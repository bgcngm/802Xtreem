/* linux/drivers/misc/nm32x_62x-tsi.c
 *
 * Driver file for NM32x/62x Transport Stream Interface Device
 *
 *  Copyright (c) 2011 Newport Media Inc.
 * 	http://www.newportmediainc.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/err.h>          /* IS_ERR etc. */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>  

#include <linux/uaccess.h>        /* copy_to_user */
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/tsif_api.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/segment.h>

struct tsif_dev {
#if defined(CONFIG_PM)
	int last_running_state;
#endif
	struct device *dev;
	wait_queue_head_t wq_read;
	void *cookie;
	/* mirror for tsif data */
	void *data_buffer;
	unsigned buf_size_packets; /**< buffer size in packets */
	unsigned ri, wi;
	enum tsif_state state;
	unsigned rptr;
};

static struct platform_device *nm32x_62x_tsi_dev;

static void nm32x_62x_tsi_rx_notify(void *data)
{
	struct tsif_dev *the_dev = data;
	tsif_get_state(the_dev->cookie, &the_dev->ri, &the_dev->wi,
		       &the_dev->state);
	wake_up_interruptible(&the_dev->wq_read);
}

static int nm32x_62x_tsi_release(struct inode *inode, struct file *file)
{
	struct tsif_dev *the_dev = file->private_data;
	if(the_dev->state == tsif_state_running)
		tsif_stop(the_dev->cookie);
	return 0;
}
#if 1//MTF
static unsigned int nm32x_62x_tsi_poll(struct file *file, poll_table *wait)
{
 unsigned int mask = 0;
 struct tsif_dev *the_dev = file->private_data;

 poll_wait(file, &the_dev->wq_read, wait);

 if(the_dev->state != tsif_state_running)
 {
   mask |= (POLLHUP | POLLNVAL| POLLERR);
 }
 else if ((the_dev->ri != the_dev->wi))
 {
	mask |= (POLLIN | POLLRDNORM);
 }

 return mask;
}
#else
static unsigned int	nm32x_62x_tsi_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	struct tsif_dev *the_dev = file->private_data;

	poll_wait(file, &the_dev->wq_read, wait);

	if((the_dev->ri != the_dev->wi) || (the_dev->state != tsif_state_running))
	{
		mask |= (POLLIN | POLLRDNORM);
	}

	return mask;
}
#endif
static ssize_t nm32x_62x_tsi_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
//	void *data_buffer = NULL;
//	int i;
	int avail = 0;
	int wi;
//	unsigned char *ptr;
	struct tsif_dev *the_dev = file->private_data;
	tsif_get_state(the_dev->cookie, &the_dev->ri, &the_dev->wi,
		       &the_dev->state);
   
    if (the_dev->state != tsif_state_running)
        return 0;

	/* consistency check */
	if (the_dev->ri != (the_dev->rptr / TSIF_PKT_SIZE)) {
		dev_err(the_dev->dev,
			"%s: inconsistent read pointers: ri %d rptr %d\n",
			__func__, the_dev->ri, the_dev->rptr);
		the_dev->rptr = the_dev->ri * TSIF_PKT_SIZE;
	}
	/* ri == wi if no data */
	if (the_dev->ri == the_dev->wi) {
		/* shall I block waiting for data? */
		if (file->f_flags & O_NONBLOCK) {
			if (the_dev->state == tsif_state_running) {
				return -EAGAIN;
			} else {
				/* not running -> EOF */
				return 0;
			}
		}
		if (wait_event_interruptible(the_dev->wq_read,
		      (the_dev->ri != the_dev->wi) ||
		      (the_dev->state != tsif_state_running))) {
			/* got signal -> tell FS to handle it */
			return -ERESTARTSYS;
		}
		if (the_dev->ri == the_dev->wi) {
			/* still no data -> EOF */
			return 0;
		}
	}
	/* contiguous chunk last up to wi or end of buffer */
	wi = (the_dev->wi > the_dev->ri) ?
		the_dev->wi : the_dev->buf_size_packets;

//	data_buffer = kzalloc(188*16, GFP_KERNEL);
//	if ((wi * TSIF_PKT_SIZE - the_dev->rptr) == TSIF_PKT_SIZE*16){
//		int i;
//		for (i=0; i < 16 ; i++)
//			memcpy(data_buffer + 188*i, the_dev->data_buffer + the_dev->rptr + 192*i, 188);
//	}

	avail = min(wi * TSIF_PKT_SIZE - the_dev->rptr, count);
//	printk("size to be read out ==> wi * TSIF_PKT_SIZE - the_dev->rptr: %d or count: %d\r\n",(int) (wi * TSIF_PKT_SIZE - the_dev->rptr), (int) count);

//suggest by NMI to fix kernel panic bug, Valente Wx ITS#537
    if (NULL == the_dev->data_buffer)
        return 0;

#if 0
	if (copy_to_user(buf, data_buffer, 188*16))
		return -EFAULT;
#else
	if (copy_to_user(buf, the_dev->data_buffer + the_dev->rptr, avail))
		return -EFAULT;
#endif
#if 0
ptr = (unsigned char *)(the_dev->data_buffer + the_dev->rptr);
for (i = 0; i < avail; i += 2)
{
if (i % 192 == 0)
	printk("[TUNER] cnt = %d, Val = %d, Val2 = %d\r\n", i, *(ptr + i), *(ptr + i + 1));
}
#endif
	the_dev->rptr = (the_dev->rptr + avail) %
		(TSIF_PKT_SIZE * the_dev->buf_size_packets);
	the_dev->ri = the_dev->rptr / TSIF_PKT_SIZE;
	*pos += avail;
	tsif_reclaim_packets(the_dev->cookie, the_dev->ri);

//	kfree(data_buffer);

	return avail;
}

#define TSI_TRIGGER 0xAABB
#define TSI_STOP     0xAACC	

static long nm32x_62x_tsi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct tsif_dev *the_dev = platform_get_drvdata(nm32x_62x_tsi_dev);

//currently only two ioctl for tigger and stop are provided..
	printk(KERN_DEBUG"<ISDB-T> TSI cmd is %x\n",cmd);
	switch(cmd)
	{
		case TSI_TRIGGER:
			if(the_dev->state == tsif_state_running)
				return -EBUSY;
		printk(KERN_DEBUG"<ISDB-T> the_dev->cookie is  %x\n", (unsigned int)the_dev->cookie);
		printk(KERN_DEBUG"<ISDB-T> the_dev->cookie\n");
		ret = tsif_start(the_dev->cookie);
		if (ret)
			return ret;
		tsif_get_info(the_dev->cookie, &the_dev->data_buffer,
			      &the_dev->buf_size_packets);
		the_dev->rptr = 0;
		break;

		case TSI_STOP:
			tsif_stop(the_dev->cookie);	

		break;
		default:
		break;



	}

 	return ret;
}


static int nm32x_62x_tsi_open(struct inode *inode, struct file *file)
{
	struct tsif_dev *the_dev = platform_get_drvdata(nm32x_62x_tsi_dev);
	if (!the_dev->cookie)  /* not bound yet */
		return -ENODEV;
	file->private_data = the_dev;
	return nonseekable_open(inode, file);
}

static struct file_operations tsi_fops = {
	owner:		THIS_MODULE,
	open:		nm32x_62x_tsi_open,
	release:	nm32x_62x_tsi_release,
	unlocked_ioctl:		nm32x_62x_tsi_ioctl,
	read:		nm32x_62x_tsi_read,
	poll:		nm32x_62x_tsi_poll,
};


static struct miscdevice nm32x_62x_tsi_miscdev = {
	minor:		MISC_DYNAMIC_MINOR,
	name:		"nm32x_62x-tsi",
	fops:		&tsi_fops
};

static int tsif_init_one(struct tsif_dev *the_dev, int index)
{
	int rc;
	pr_info("%s[%d]\n", __func__, index);
	init_waitqueue_head(&the_dev->wq_read);
	the_dev->cookie = tsif_attach(index, nm32x_62x_tsi_rx_notify, the_dev);
	if (IS_ERR(the_dev->cookie)) {
		rc = PTR_ERR(the_dev->cookie);
		pr_err("tsif_attach failed: %d\n", rc);
		goto err_attach;
	}
	/* now data buffer is not allocated yet */
	tsif_get_info(the_dev->cookie, &the_dev->data_buffer, NULL);
	dev_info(the_dev->dev,
		 "Device attached to TSIF, buffer size %d\n", the_dev->buf_size_packets);
	return 0;
err_attach:
	return rc;
}

static void tsif_exit_one(struct tsif_dev *the_dev)
{
	dev_info(the_dev->dev, "%s\n", __func__);
	tsif_detach(the_dev->cookie);
}

#define TSIF_NUM_DEVS 1 /**< support this many devices */

struct tsif_dev the_devices[TSIF_NUM_DEVS];

static int nm32x_62x_tsi_probe(struct platform_device *pdev)
{
	int rc;
        int instance;
	the_devices[0].dev = &pdev->dev;

	platform_set_drvdata(pdev, &the_devices[0]);
	nm32x_62x_tsi_dev= pdev;
	rc = misc_register(&nm32x_62x_tsi_miscdev);
	if(rc){
		printk(KERN_ERR "Unable to register the nm32x_62x-tsi driver\n");
		goto err_init1;
	}	
#if 0
	rc = tsif_init_one(&the_devices[0], 0);
	if (rc)
		goto err_init1;
#else
        instance = tsif_get_active();
        if (instance >= 0)
                rc = tsif_init_one(&the_devices[0], instance);
        else
                rc = instance;
        if (rc)
                goto err_init1;
#endif
	return 0;

err_init1:
	return rc;
}

static int nm32x_62x_tsi_remove(struct platform_device *dev)
{
	struct tsif_dev *the_dev = platform_get_drvdata(dev);
	tsif_exit_one(the_dev);
	
	return 0;

}


#if defined(CONFIG_PM)
static int nm32x_62x_tsi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct tsif_dev *the_dev = platform_get_drvdata(pdev);

	the_dev->last_running_state = the_dev->state;
	if(the_dev->state == tsif_state_running)
		tsif_stop(the_dev->cookie);

	return 0;
}

static int nm32x_62x_tsi_resume(struct platform_device *pdev)
{
	struct tsif_dev *the_dev = platform_get_drvdata(pdev);

	if(the_dev->last_running_state == tsif_state_running){
		tsif_start(the_dev->cookie);
		tsif_get_state(the_dev->cookie, &the_dev->ri, &the_dev->wi,
			       &the_dev->state);
		wake_up_interruptible(&the_dev->wq_read);
	}

	return 0;
}
#endif  //defined(CONFIG_PM)

static struct platform_driver nm32x_62x_tsi_driver = {
	.probe		= nm32x_62x_tsi_probe,
	.remove		= nm32x_62x_tsi_remove,
	.shutdown	= NULL,
#if defined(CONFIG_PM)
	.suspend	= nm32x_62x_tsi_suspend,
	.resume		= nm32x_62x_tsi_resume,
#else
	.suspend	= NULL,
	.resume		= NULL,
#endif
	.driver		= {
			.owner	= THIS_MODULE,
			.name	= "nm32x_62x-tsi",
	},
};



const char nm32x_62x[] __initdata =  "NM32x/62x TSI Driver Version 1.0\n";

static int __init nm32x_62x_tsi_init(void)
{
	printk(nm32x_62x);
	return platform_driver_register(&nm32x_62x_tsi_driver);
}




static void __exit nm32x_62x_tsi_exit(void)
{

	platform_driver_unregister(&nm32x_62x_tsi_driver);
}



module_init(nm32x_62x_tsi_init);
module_exit(nm32x_62x_tsi_exit);

MODULE_AUTHOR("Newport Media Inc.");
MODULE_DESCRIPTION("NM32X/62X TSI Device Driver");
MODULE_LICENSE("GPL");
