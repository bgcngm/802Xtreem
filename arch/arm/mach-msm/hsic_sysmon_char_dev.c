/* Copyright (c) 2013, HTC Corporation. All rights reserved.
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

/* add additional information to our printk's */
#define pr_fmt(fmt) "%s: " fmt "\n", __func__

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/cdev.h>

#include "hsic_sysmon.h"
#include "sysmon.h"

#define DRIVER_DESC	"HSIC System monitor driver character device"

#define RD_BUF_SIZE	4096

#define HTC_SYSMON_DEVICE_NAME		"htc_hsicsysmon"
static dev_t				sysmon_dev_num;
struct cdev				hsic_sysmon_cdev;
struct class				*hsicdev_classp;
struct device				*devicep;

struct sysmon_char_dev {
	int			buflen;
	char			buf[RD_BUF_SIZE];
};
static struct sysmon_char_dev *sysmon_dev;

static ssize_t hsic_sysmon_char_read(struct file *file, char __user *ubuf,
				 size_t count, loff_t *ppos)
{
	struct sysmon_char_dev *dev = sysmon_dev;
	int ret;

	if (!dev)
		return -ENODEV;

	ret = hsic_sysmon_read(HSIC_SYSMON_DEV_EXT_MODEM, dev->buf, RD_BUF_SIZE,
				&dev->buflen, 0);
	if (!ret)
		return simple_read_from_buffer(ubuf, count, ppos,
					dev->buf, dev->buflen);

	return 0;
}

static ssize_t hsic_sysmon_char_write(struct file *file, const char __user *ubuf,
				 size_t count, loff_t *ppos)
{
	struct sysmon_char_dev	*dev = sysmon_dev;
	int ret;
	int i = 0;

	if (!dev)
		return -ENODEV;

	if (copy_from_user(dev->buf, ubuf, count)) {
		pr_err("error copying for writing");
		return 0;
	}
	for (i = 0 ; i < count; i++)
		pr_err("%c", dev->buf[i]);

	ret = hsic_sysmon_write(HSIC_SYSMON_DEV_EXT_MODEM,
				dev->buf, count, 1000);
	if (ret < 0) {
		pr_err("error writing to hsic_sysmon");
		return ret;
	}

	return count;
}

static int hsic_sysmon_char_open(struct inode *inode, struct file *file)
{
	pr_info("open device");
	return hsic_sysmon_open(HSIC_SYSMON_DEV_EXT_MODEM);
}

static int hsic_sysmon_char_release(struct inode *inode, struct file *file)
{
	pr_info("release device");
	hsic_sysmon_close(HSIC_SYSMON_DEV_EXT_MODEM);
	return 0;
}

static const struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.read  = hsic_sysmon_char_read,
        .write = hsic_sysmon_char_write,
        .open  = hsic_sysmon_char_open,
	.release = hsic_sysmon_char_release
};


static int __init sysmon_char_dev_init(void)
{
	int status;

	sysmon_dev = kzalloc(sizeof(*sysmon_dev), GFP_KERNEL);
	if (!sysmon_dev)
		return -ENOMEM;



	status = alloc_chrdev_region(&sysmon_dev_num, 0, 1, HTC_SYSMON_DEVICE_NAME);
	if (IS_ERR_VALUE(status)) {
		pr_err("ERROR:%s: alloc_chrdev_region() ret %i.\n",
			__func__, status);
		goto error0;
	}
	hsicdev_classp = class_create(THIS_MODULE, HTC_SYSMON_DEVICE_NAME);
	if (IS_ERR(hsicdev_classp)) {
		pr_err("ERROR:%s: class_create() ENOMEM\n", __func__);
		status = -ENOMEM;
		goto error1;
	}
	cdev_init(&hsic_sysmon_cdev, &dev_fops);
	hsic_sysmon_cdev.owner = THIS_MODULE;

	status = cdev_add(&hsic_sysmon_cdev, sysmon_dev_num, 1);
	if (IS_ERR_VALUE(status)) {
		pr_err("%s: cdev_add() ret%i\n", __func__, status);
		goto error1;
	}
	devicep = device_create(hsicdev_classp, NULL, sysmon_dev_num, NULL, HTC_SYSMON_DEVICE_NAME);
	if (IS_ERR(devicep)) {
		pr_err("%s: device_create() ENOMEM\n", __func__);
		status = -ENOMEM;
		cdev_del(&hsic_sysmon_cdev);
		goto error1;
	}
	return 0;

error1:
	cdev_del(&hsic_sysmon_cdev);
	device_destroy(hsicdev_classp,
		MKDEV(MAJOR(sysmon_dev_num), 1));

	class_destroy(hsicdev_classp);
error0:
	unregister_chrdev_region(MAJOR(sysmon_dev_num), 1);
	return 0;
}

static void __exit sysmon_char_dev_exit(void)
{
	kfree(sysmon_dev);
        cdev_del(&hsic_sysmon_cdev);
        device_destroy(hsicdev_classp,
                MKDEV(MAJOR(sysmon_dev_num), 1));

        class_destroy(hsicdev_classp);
        unregister_chrdev_region(MAJOR(sysmon_dev_num), 1);
}

module_init(sysmon_char_dev_init);
module_exit(sysmon_char_dev_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
