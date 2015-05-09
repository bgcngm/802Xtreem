/* Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "drmP.h"
#include "drm.h"
#include <linux/android_pmem.h>

#include "kgsl.h"
#include "kgsl_device.h"
#include "kgsl_drm.h"
#include "kgsl_mmu.h"
#include "kgsl_sharedmem.h"

#define DRIVER_AUTHOR           "Qualcomm"
#define DRIVER_NAME             "kgsl"
#define DRIVER_DESC             "KGSL DRM"
#define DRIVER_DATE             "20100127"

#define DRIVER_MAJOR            2
#define DRIVER_MINOR            1
#define DRIVER_PATCHLEVEL       1

#define DRM_KGSL_GEM_FLAG_MAPPED (1 << 0)

#define DRM_KGSL_NOT_INITED -1
#define DRM_KGSL_INITED   1


#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
#define TYPE_IS_PMEM(_t) \
  (((_t & DRM_KGSL_GEM_TYPE_MEM_MASK) == DRM_KGSL_GEM_TYPE_EBI) || \
   ((_t & DRM_KGSL_GEM_TYPE_MEM_MASK) == DRM_KGSL_GEM_TYPE_SMI) || \
   ((_t) & DRM_KGSL_GEM_TYPE_PMEM))
#else
#define TYPE_IS_PMEM(_t) \
  (((_t & DRM_KGSL_GEM_TYPE_MEM_MASK) == DRM_KGSL_GEM_TYPE_EBI) || \
   ((_t) & (DRM_KGSL_GEM_TYPE_PMEM | DRM_KGSL_GEM_PMEM_EBI)))
#endif


#define TYPE_IS_MEM(_t) \
  (((_t & DRM_KGSL_GEM_TYPE_MEM_MASK) == DRM_KGSL_GEM_TYPE_KMEM) || \
   ((_t & DRM_KGSL_GEM_TYPE_MEM_MASK) == DRM_KGSL_GEM_TYPE_KMEM_NOCACHE) || \
   ((_t) & DRM_KGSL_GEM_TYPE_MEM))

#define TYPE_IS_FD(_t) ((_t) & DRM_KGSL_GEM_TYPE_FD_MASK)


#define IS_MEM_UNCACHED(_t) \
  ((_t == DRM_KGSL_GEM_TYPE_KMEM_NOCACHE) || \
   (_t == DRM_KGSL_GEM_TYPE_KMEM) || \
   (TYPE_IS_MEM(_t) && (_t & DRM_KGSL_GEM_CACHE_WCOMBINE)))

struct drm_kgsl_gem_object {
	struct drm_gem_object *obj;
	uint32_t type;
	struct kgsl_memdesc memdesc;
	struct kgsl_pagetable *pagetable;
	uint64_t mmap_offset;
	int bufcount;
	int flags;
	struct list_head list;
	int active;

	struct {
		uint32_t offset;
		uint32_t gpuaddr;
	} bufs[DRM_KGSL_GEM_MAX_BUFFERS];

	int bound;
	int lockpid;

};

static int kgsl_drm_inited = DRM_KGSL_NOT_INITED;

static struct list_head kgsl_mem_list;

static void kgsl_gem_mem_flush(struct kgsl_memdesc *memdesc, int type, int op)
{
	int cacheop = 0;

	switch (op) {
	case DRM_KGSL_GEM_CACHE_OP_TO_DEV:
		if (type & (DRM_KGSL_GEM_CACHE_WBACK |
			    DRM_KGSL_GEM_CACHE_WBACKWA))
			cacheop = KGSL_CACHE_OP_CLEAN;

		break;

	case DRM_KGSL_GEM_CACHE_OP_FROM_DEV:
		if (type & (DRM_KGSL_GEM_CACHE_WBACK |
			    DRM_KGSL_GEM_CACHE_WBACKWA |
			    DRM_KGSL_GEM_CACHE_WTHROUGH))
			cacheop = KGSL_CACHE_OP_INV;
	}

	kgsl_cache_range_op(memdesc, cacheop);
}


static int kgsl_drm_load(struct drm_device *dev, unsigned long flags)
{
	return 0;
}

static int kgsl_drm_unload(struct drm_device *dev)
{
	return 0;
}

struct kgsl_drm_device_priv {
	struct kgsl_device *device[KGSL_DEVICE_MAX];
	struct kgsl_device_private *devpriv[KGSL_DEVICE_MAX];
};

void kgsl_drm_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
}

static int kgsl_drm_suspend(struct drm_device *dev, pm_message_t state)
{
	return 0;
}

static int kgsl_drm_resume(struct drm_device *dev)
{
	return 0;
}

static void
kgsl_gem_free_mmap_offset(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	struct drm_map_list *list;

	list = &obj->map_list;
	drm_ht_remove_item(&mm->offset_hash, &list->hash);
	if (list->file_offset_node) {
		drm_mm_put_block(list->file_offset_node);
		list->file_offset_node = NULL;
	}

	kfree(list->map);
	list->map = NULL;

	priv->mmap_offset = 0;
}

static int
kgsl_gem_memory_allocated(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	return priv->memdesc.size ? 1 : 0;
}

static int
kgsl_gem_alloc_memory(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	int index;
	int result = 0;

	

	if (kgsl_gem_memory_allocated(obj) || TYPE_IS_FD(priv->type))
		return 0;

	if (priv->pagetable == NULL) {
		priv->pagetable = kgsl_mmu_getpagetable(KGSL_MMU_GLOBAL_PT);

		if (priv->pagetable == NULL) {
			DRM_ERROR("Unable to get the GPU MMU pagetable\n");
			return -EINVAL;
		}
	}

	
	priv->memdesc.priv = 0;

	if (TYPE_IS_PMEM(priv->type)) {
		if (priv->type == DRM_KGSL_GEM_TYPE_EBI ||
		    priv->type & DRM_KGSL_GEM_PMEM_EBI) {
				result = kgsl_sharedmem_ebimem_user(
						&priv->memdesc,
						priv->pagetable,
						obj->size * priv->bufcount);
				if (result) {
					DRM_ERROR(
					"Unable to allocate PMEM memory\n");
					return result;
				}
		}
		else
			return -EINVAL;

	} else if (TYPE_IS_MEM(priv->type)) {

		if (priv->type == DRM_KGSL_GEM_TYPE_KMEM ||
			priv->type & DRM_KGSL_GEM_CACHE_MASK)
				list_add(&priv->list, &kgsl_mem_list);

		result = kgsl_sharedmem_page_alloc_user(&priv->memdesc,
					priv->pagetable,
					obj->size * priv->bufcount);

		if (result != 0) {
				DRM_ERROR(
				"Unable to allocate Vmalloc user memory\n");
				return result;
		}
	} else
		return -EINVAL;

	for (index = 0; index < priv->bufcount; index++) {
		priv->bufs[index].offset = index * obj->size;
		priv->bufs[index].gpuaddr =
			priv->memdesc.gpuaddr +
			priv->bufs[index].offset;
	}
	priv->flags |= DRM_KGSL_GEM_FLAG_MAPPED;

	return 0;
}

static void
kgsl_gem_free_memory(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv = obj->driver_private;

	if (!kgsl_gem_memory_allocated(obj) || TYPE_IS_FD(priv->type))
		return;

	kgsl_gem_mem_flush(&priv->memdesc,  priv->type,
			   DRM_KGSL_GEM_CACHE_OP_FROM_DEV);

	kgsl_sharedmem_free(&priv->memdesc);

	kgsl_mmu_putpagetable(priv->pagetable);
	priv->pagetable = NULL;

	if ((priv->type == DRM_KGSL_GEM_TYPE_KMEM) ||
	    (priv->type & DRM_KGSL_GEM_CACHE_MASK))
		list_del(&priv->list);

	priv->flags &= ~DRM_KGSL_GEM_FLAG_MAPPED;

}

int
kgsl_gem_init_object(struct drm_gem_object *obj)
{
	struct drm_kgsl_gem_object *priv;
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		DRM_ERROR("Unable to create GEM object\n");
		return -ENOMEM;
	}

	obj->driver_private = priv;
	priv->obj = obj;

	return 0;
}

void
kgsl_gem_free_object(struct drm_gem_object *obj)
{
	kgsl_gem_free_memory(obj);
	kgsl_gem_free_mmap_offset(obj);
	drm_gem_object_release(obj);
	kfree(obj->driver_private);
}

static int
kgsl_gem_create_mmap_offset(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_kgsl_gem_object *priv = obj->driver_private;
	struct drm_map_list *list;
	int msize;

	list = &obj->map_list;
	list->map = kzalloc(sizeof(struct drm_map_list), GFP_KERNEL);
	if (list->map == NULL) {
		DRM_ERROR("Unable to allocate drm_map_list\n");
		return -ENOMEM;
	}

	msize = obj->size * priv->bufcount;

	list->map->type = _DRM_GEM;
	list->map->size = msize;
	list->map->handle = obj;

	
	list->file_offset_node = drm_mm_search_free(&mm->offset_manager,
						    msize / PAGE_SIZE,
						    0, 0);

	if (!list->file_offset_node) {
		DRM_ERROR("Failed to allocate offset for %d\n", obj->name);
		kfree(list->map);
		return -ENOMEM;
	}

	list->file_offset_node = drm_mm_get_block(list->file_offset_node,
						  msize / PAGE_SIZE, 0);

	if (!list->file_offset_node) {
		DRM_ERROR("Unable to create the file_offset_node\n");
		kfree(list->map);
		return -ENOMEM;
	}

	list->hash.key = list->file_offset_node->start;
	if (drm_ht_insert_item(&mm->offset_hash, &list->hash)) {
		DRM_ERROR("Failed to add to map hash\n");
		drm_mm_put_block(list->file_offset_node);
		kfree(list->map);
		return -ENOMEM;
	}

	priv->mmap_offset = ((uint64_t) list->hash.key) << PAGE_SHIFT;

	return 0;
}

int
kgsl_gem_obj_addr(int drm_fd, int handle, unsigned long *start,
			unsigned long *len)
{
	struct file *filp;
	struct drm_device *dev;
	struct drm_file *file_priv;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = 0;

	filp = fget(drm_fd);
	if (unlikely(filp == NULL)) {
		DRM_ERROR("Unable to get the DRM file descriptor\n");
		return -EINVAL;
	}
	file_priv = filp->private_data;
	if (unlikely(file_priv == NULL)) {
		DRM_ERROR("Unable to get the file private data\n");
		fput(filp);
		return -EINVAL;
	}
	dev = file_priv->minor->dev;
	if (unlikely(dev == NULL)) {
		DRM_ERROR("Unable to get the minor device\n");
		fput(filp);
		return -EINVAL;
	}

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (unlikely(obj == NULL)) {
		DRM_ERROR("Invalid GEM handle %x\n", handle);
		fput(filp);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	

	if (TYPE_IS_PMEM(priv->type)) {
		*start = priv->memdesc.physaddr +
			priv->bufs[priv->active].offset;

		*len = priv->memdesc.size;

		kgsl_gem_mem_flush(&priv->memdesc,
				   priv->type, DRM_KGSL_GEM_CACHE_OP_TO_DEV);
	} else {
		*start = 0;
		*len = 0;
		ret = -EINVAL;
	}

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	fput(filp);
	return ret;
}

static int
kgsl_gem_init_obj(struct drm_device *dev,
		  struct drm_file *file_priv,
		  struct drm_gem_object *obj,
		  int *handle)
{
	struct drm_kgsl_gem_object *priv;
	int ret;

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	memset(&priv->memdesc, 0, sizeof(priv->memdesc));
	priv->bufcount = 1;
	priv->active = 0;
	priv->bound = 0;


	priv->type = DRM_KGSL_GEM_TYPE_PMEM | DRM_KGSL_GEM_PMEM_EBI;

	ret = drm_gem_handle_create(file_priv, obj, handle);

	drm_gem_object_unreference(obj);

	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int
kgsl_gem_create_ioctl(struct drm_device *dev, void *data,
		      struct drm_file *file_priv)
{
	struct drm_kgsl_gem_create *create = data;
	struct drm_gem_object *obj;
	int ret, handle;

	
	create->size = ALIGN(create->size, 4096);

	obj = drm_gem_object_alloc(dev, create->size);

	if (obj == NULL) {
		DRM_ERROR("Unable to allocate the GEM object\n");
		return -ENOMEM;
	}

	ret = kgsl_gem_init_obj(dev, file_priv, obj, &handle);
	if (ret)
		return ret;

	create->handle = handle;
	return 0;
}

int
kgsl_gem_create_fd_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_kgsl_gem_create_fd *args = data;
	struct file *file;
	dev_t rdev;
	struct fb_info *info;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret, put_needed, handle;

	file = fget_light(args->fd, &put_needed);

	if (file == NULL) {
		DRM_ERROR("Unable to get the file object\n");
		return -EBADF;
	}

	rdev = file->f_dentry->d_inode->i_rdev;

	

	if (MAJOR(rdev) != FB_MAJOR) {
		DRM_ERROR("File descriptor is not a framebuffer\n");
		ret = -EBADF;
		goto error_fput;
	}

	info = registered_fb[MINOR(rdev)];

	if (info == NULL) {
		DRM_ERROR("Framebuffer minor %d is not registered\n",
			  MINOR(rdev));
		ret = -EBADF;
		goto error_fput;
	}

	obj = drm_gem_object_alloc(dev, info->fix.smem_len);

	if (obj == NULL) {
		DRM_ERROR("Unable to allocate GEM object\n");
		ret = -ENOMEM;
		goto error_fput;
	}

	ret = kgsl_gem_init_obj(dev, file_priv, obj, &handle);

	if (ret)
		goto error_fput;

	mutex_lock(&dev->struct_mutex);

	priv = obj->driver_private;
	priv->memdesc.physaddr = info->fix.smem_start;
	priv->type = DRM_KGSL_GEM_TYPE_FD_FBMEM;

	mutex_unlock(&dev->struct_mutex);
	args->handle = handle;

error_fput:
	fput_light(file, put_needed);

	return ret;
}

int
kgsl_gem_setmemtype_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_memtype *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = 0;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	if (TYPE_IS_FD(priv->type))
		ret = -EINVAL;
	else {
		if (TYPE_IS_PMEM(args->type) || TYPE_IS_MEM(args->type))
			priv->type = args->type;
		else
			ret = -EINVAL;
	}

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_getmemtype_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct drm_kgsl_gem_memtype *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	args->type = priv->type;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int
kgsl_gem_unbind_gpu_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	return 0;
}

int
kgsl_gem_bind_gpu_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	return 0;
}


int
kgsl_gem_alloc_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *file_priv)
{
	struct drm_kgsl_gem_alloc *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	ret = kgsl_gem_alloc_memory(obj);

	if (ret) {
		DRM_ERROR("Unable to allocate object memory\n");
	} else if (!priv->mmap_offset) {
		ret = kgsl_gem_create_mmap_offset(obj);
		if (ret)
			DRM_ERROR("Unable to create a mmap offset\n");
	}

	args->offset = priv->mmap_offset;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_mmap_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_mmap *args = data;
	struct drm_gem_object *obj;
	unsigned long addr;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	down_write(&current->mm->mmap_sem);

	addr = do_mmap(obj->filp, 0, args->size,
		       PROT_READ | PROT_WRITE, MAP_SHARED,
		       args->offset);

	up_write(&current->mm->mmap_sem);

	mutex_lock(&dev->struct_mutex);
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	if (IS_ERR((void *) addr))
		return addr;

	args->hostptr = (uint32_t) addr;
	return 0;
}


int
kgsl_gem_prep_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_kgsl_gem_prep *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	ret = kgsl_gem_alloc_memory(obj);
	if (ret) {
		DRM_ERROR("Unable to allocate object memory\n");
		drm_gem_object_unreference(obj);
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}

	if (priv->mmap_offset == 0) {
		ret = kgsl_gem_create_mmap_offset(obj);
		if (ret) {
			drm_gem_object_unreference(obj);
			mutex_unlock(&dev->struct_mutex);
			return ret;
		}
	}

	args->offset = priv->mmap_offset;
	args->phys = priv->memdesc.physaddr;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int
kgsl_gem_get_bufinfo_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	struct drm_kgsl_gem_bufinfo *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = -EINVAL;
	int index;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	if (!kgsl_gem_memory_allocated(obj)) {
		DRM_ERROR("Memory not allocated for this object\n");
		goto out;
	}

	for (index = 0; index < priv->bufcount; index++) {
		args->offset[index] = priv->bufs[index].offset;
		args->gpuaddr[index] = priv->bufs[index].gpuaddr;
	}

	args->count = priv->bufcount;
	args->active = priv->active;

	ret = 0;

out:
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_set_bufcount_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct drm_kgsl_gem_bufcount *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = -EINVAL;

	if (args->bufcount < 1 || args->bufcount > DRM_KGSL_GEM_MAX_BUFFERS)
		return -EINVAL;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;


	if (kgsl_gem_memory_allocated(obj)) {
		DRM_ERROR("Memory already allocated - cannot change"
			  "number of buffers\n");
		goto out;
	}

	priv->bufcount = args->bufcount;
	ret = 0;

out:
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
kgsl_gem_set_active_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct drm_kgsl_gem_active *args = data;
	struct drm_gem_object *obj;
	struct drm_kgsl_gem_object *priv;
	int ret = -EINVAL;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);

	if (obj == NULL) {
		DRM_ERROR("Invalid GEM handle %x\n", args->handle);
		return -EBADF;
	}

	mutex_lock(&dev->struct_mutex);
	priv = obj->driver_private;

	if (args->active < 0 || args->active >= priv->bufcount) {
		DRM_ERROR("Invalid active buffer %d\n", args->active);
		goto out;
	}

	priv->active = args->active;
	ret = 0;

out:
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int kgsl_gem_kmem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;
	struct drm_kgsl_gem_object *priv;
	unsigned long offset, pg, pfn;
	struct page *page;
	int i;

	mutex_lock(&dev->struct_mutex);

	priv = obj->driver_private;

	offset = (unsigned long) vmf->virtual_address - vma->vm_start;
	if (priv->memdesc.handle == NULL) {
		i = offset >> PAGE_SHIFT;
		page = sg_page(&(priv->memdesc.sg[i]));

		if (!page) {
			mutex_unlock(&dev->struct_mutex);
			return VM_FAULT_SIGBUS;
		}
		get_page(page);
		vmf->page = page;
	} else {
		pfn = (priv->memdesc->sg[0].dma_address >> PAGE_SHIFT) + offset;
		ret = vm_insert_pfn(vma, (unsigned long) vmf->virtual_address, pfn);
		if (ret == -ENOMEM || ret == -EAGAIN) {
			mutex_unlock(&dev->struct_mutex);
			return VM_FAULT_OOM;
		} else if (ret == -EFAULT) {
			mutex_unlock(&dev->struct_mutex);
			return VM_FAULT_SIGBUS;
		}
	}

	mutex_unlock(&dev->struct_mutex);
	return 0;
}

int kgsl_gem_phys_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;
	struct drm_kgsl_gem_object *priv;
	unsigned long offset, pfn;
	int ret = 0;

	offset = ((unsigned long) vmf->virtual_address - vma->vm_start) >>
		PAGE_SHIFT;

	mutex_lock(&dev->struct_mutex);

	priv = obj->driver_private;

	pfn = (priv->memdesc.physaddr >> PAGE_SHIFT) + offset;
	ret = vm_insert_pfn(vma,
			    (unsigned long) vmf->virtual_address, pfn);
	mutex_unlock(&dev->struct_mutex);

	switch (ret) {
	case -ENOMEM:
	case -EAGAIN:
		return VM_FAULT_OOM;
	case -EFAULT:
		return VM_FAULT_SIGBUS;
	default:
		return VM_FAULT_NOPAGE;
	}
}

static struct vm_operations_struct kgsl_gem_kmem_vm_ops = {
	.fault = kgsl_gem_kmem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static struct vm_operations_struct kgsl_gem_phys_vm_ops = {
	.fault = kgsl_gem_phys_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};


int msm_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *priv = filp->private_data;
	struct drm_device *dev = priv->minor->dev;
	struct drm_gem_mm *mm = dev->mm_private;
	struct drm_local_map *map = NULL;
	struct drm_gem_object *obj;
	struct drm_hash_item *hash;
	struct drm_kgsl_gem_object *gpriv;
	int ret = 0;

	mutex_lock(&dev->struct_mutex);

	if (drm_ht_find_item(&mm->offset_hash, vma->vm_pgoff, &hash)) {
		mutex_unlock(&dev->struct_mutex);
		return drm_mmap(filp, vma);
	}

	map = drm_hash_entry(hash, struct drm_map_list, hash)->map;
	if (!map ||
	    ((map->flags & _DRM_RESTRICTED) && !capable(CAP_SYS_ADMIN))) {
		ret =  -EPERM;
		goto out_unlock;
	}

	
	if (map->size < vma->vm_end - vma->vm_start) {
		ret = -EINVAL;
		goto out_unlock;
	}

	obj = map->handle;

	gpriv = obj->driver_private;


	if (TYPE_IS_MEM(gpriv->type)) {
		vma->vm_flags |= VM_RESERVED | VM_DONTEXPAND;
		vma->vm_ops = &kgsl_gem_kmem_vm_ops;
	} else {
		vma->vm_flags |= VM_RESERVED | VM_IO | VM_PFNMAP |
			VM_DONTEXPAND;
		vma->vm_ops = &kgsl_gem_phys_vm_ops;
	}

	vma->vm_private_data = map->handle;


	
	if (gpriv->type == DRM_KGSL_GEM_TYPE_KMEM ||
	    gpriv->type & DRM_KGSL_GEM_CACHE_MASK) {
		if (gpriv->type & DRM_KGSL_GEM_CACHE_WBACKWA)
			vma->vm_page_prot =
			pgprot_writebackwacache(vma->vm_page_prot);
		else if (gpriv->type & DRM_KGSL_GEM_CACHE_WBACK)
				vma->vm_page_prot =
				pgprot_writebackcache(vma->vm_page_prot);
		else if (gpriv->type & DRM_KGSL_GEM_CACHE_WTHROUGH)
				vma->vm_page_prot =
				pgprot_writethroughcache(vma->vm_page_prot);
		else
			vma->vm_page_prot =
			pgprot_writecombine(vma->vm_page_prot);
	} else {
		if (gpriv->type == DRM_KGSL_GEM_TYPE_KMEM_NOCACHE)
			vma->vm_page_prot =
			pgprot_noncached(vma->vm_page_prot);
		else
			
			vma->vm_page_prot =
			pgprot_writecombine(vma->vm_page_prot);
	}

	if (IS_MEM_UNCACHED(gpriv->type))
		kgsl_cache_range_op(&gpriv->memdesc,
				    KGSL_CACHE_OP_FLUSH);

	

	drm_gem_object_reference(obj);

	vma->vm_file = filp;	
	drm_vm_open_locked(vma);

out_unlock:
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

struct drm_ioctl_desc kgsl_drm_ioctls[] = {
	DRM_IOCTL_DEF_DRV(KGSL_GEM_CREATE, kgsl_gem_create_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_PREP, kgsl_gem_prep_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_SETMEMTYPE, kgsl_gem_setmemtype_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_GETMEMTYPE, kgsl_gem_getmemtype_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_BIND_GPU, kgsl_gem_bind_gpu_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_UNBIND_GPU, kgsl_gem_unbind_gpu_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_ALLOC, kgsl_gem_alloc_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_MMAP, kgsl_gem_mmap_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_GET_BUFINFO, kgsl_gem_get_bufinfo_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_SET_BUFCOUNT,
		      kgsl_gem_set_bufcount_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_SET_ACTIVE, kgsl_gem_set_active_ioctl, 0),
	DRM_IOCTL_DEF_DRV(KGSL_GEM_CREATE_FD, kgsl_gem_create_fd_ioctl,
		      DRM_MASTER),
};

static struct drm_driver driver = {
	.driver_features = DRIVER_GEM,
	.load = kgsl_drm_load,
	.unload = kgsl_drm_unload,
	.preclose = kgsl_drm_preclose,
	.suspend = kgsl_drm_suspend,
	.resume = kgsl_drm_resume,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.gem_init_object = kgsl_gem_init_object,
	.gem_free_object = kgsl_gem_free_object,
	.ioctls = kgsl_drm_ioctls,

	.fops = {
		 .owner = THIS_MODULE,
		 .open = drm_open,
		 .release = drm_release,
		 .unlocked_ioctl = drm_ioctl,
		 .mmap = msm_drm_gem_mmap,
		 .poll = drm_poll,
		 .fasync = drm_fasync,
		 },

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

int kgsl_drm_init(struct platform_device *dev)
{
	
	if (kgsl_drm_inited == DRM_KGSL_INITED)
		return 0;

	kgsl_drm_inited = DRM_KGSL_INITED;

	driver.num_ioctls = DRM_ARRAY_SIZE(kgsl_drm_ioctls);

	INIT_LIST_HEAD(&kgsl_mem_list);

	return drm_platform_init(&driver, dev);
}

void kgsl_drm_exit(void)
{
	kgsl_drm_inited = DRM_KGSL_NOT_INITED;
	drm_platform_exit(&driver, driver.kdriver.platform_device);
}
