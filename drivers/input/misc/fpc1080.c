/* FPC1080 Swipe Sensor Driver
 *
 * Copyright (c) 2011 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/earlysuspend.h> //EK 20130131 Add

#include <linux/spi/fpc1080.h>
#include <mach/ADP5585_ioextender.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fingerprint Cards AB <tech@fingerprints.com>");
MODULE_DESCRIPTION("FPC1080 swipe sensor driver.");

/* -------------------------------------------------------------------- */
/* fpc1080 sensor commands and registers				*/
/* -------------------------------------------------------------------- */
#define FPC1080_ACTIVATE_SLEEP			0x28
#define FPC1080_ACTIVATE_DEEP_SLEEP		0x2C
#define FPC1080_ACTIVATE_NAVIGATION		0x30
#define FPC1080_ACTIVATE_IDLE			0x34
#define FPC1080_RD_INTERRUPT_WITH_CLEAR		0x64
#define FPC1080_RD_INTERRUPT_WITH_NO_CLEAR	0x18
#define FPC1080_WAIT_FOR_FINGER_PRESENT		0x24
#define FPC1080_READ_AND_CAPTURE_IMAGE		0xCC
#define FPC1080_READ_IMAGE_DATA			0xC4
#define FPC1080_CAPTURE_IMAGE			0xC0
#define FPC1080_SET_SMART_REF			0x48

#define FPC1080_FNGR_DWN_MIN			0x38
#define FPC1080_FNGR_DWN_MAX			0x3C
#define FPC1080_FNGR_DWN_MID			0x4C
#define FPC1080_DX_THRESHOLD			0x54
#define FPC1080_DY_THRESHOLD			0x58
#define FPC1080_FNGR_DET_THRESHOLD		0x5C
#define FPC1080_FNGR_LOST_THRESHOLD		0x60
#define FPC1080_ADC_OFFSET			0xA0
#define FPC1080_ADC_GAIN			0xA4
#define FPC1080_PXL_SETUP			0xA8
#define FPC1080_NAV_CNTR			0x80
#define FPC1080_FNGR_DRIVE_CONF			0x1C
#define FPC1080_SMRT_DATA			0x10
#define FPC1080_STATUS				0x14
#define FPC1080_REG_HW_ID			0x9C

#define FPC1080_STATUS_IRQ			(1 << 0u)
#define FPC1080_STATUS_FSM_IDLE			(1 << 1u)

#define FPC1080_SMRT_MOTION_EST_BIT_8		(1 << 0u)
#define FPC1080_SMRT_MOTION_EST_BIT_9		(1 << 1u)
#define FPC1080_SMRT_SHORT_CLICK		(1 << 2u)
#define FPC1080_SMRT_LONG_CLICK			(1 << 3u)
#define FPC1080_SMRT_X_SIGN			(1 << 4u)
#define FPC1080_SMRT_Y_SIGN			(1 << 5u)
#define FPC1080_SMRT_X_BYTE			4
#define FPC1080_SMRT_Y_BYTE			3
#define FPC1080_SMRT_MO_CNTR_BYTE		2
#define FPC1080_SMRT_MO_EST_BYTE		1
#define FPC1080_SMRT_BITS			0

#define FPC1080_SPI_FNGR_DRV_TST		(1 << 2u)
#define FPC1080_SPI_FNGR_DRV_EXT		(1 << 1u)
#define FPC1080_SPI_SMRT_SENS_EN		(1 << 0u)

#define	FPC1080_PATTERN1_XREG			0x78
#define FPC1080_PATTERN2_XREG			0x7C

#define FPC1080_IRQ_REBOOT			0xFF
#define FPC1080_IRQ_CMD_DONE			(1 << 7u)
#define FPC1080_IRQ_DY				(1 << 6u)
#define FPC1080_IRQ_DX				(1 << 5u)
#define FPC1080_IRQ_FING_LOST			(1 << 4u)
#define FPC1080_IRQ_SHORT_CLICK			(1 << 3u)
#define FPC1080_IRQ_LONG_CLICK			(1 << 2u)
#define FPC1080_IRQ_FING_UP			(1 << 1u)
#define FPC1080_IRQ_FING_DOWN			(1 << 0u)


/* -------------------------------------------------------------------- */
/* fpc1080 driver constants						*/
/* -------------------------------------------------------------------- */
#define FPC1080_HARDWARE_ID			0x1A

#define FPC1080_SYNCED_REG_SIZE			2
#define FPC1080_MOTION_THRESHOLD		25
#define FPC1080_MOTON_FRAMES_THERSHOLD		50

#define FPC1080_MAJOR				235

#define FPC1080_DEFAULT_IRQ_TIMEOUT		(100 * HZ / 1000)
#define FPC1080_FRAME_SIZE			(128 * 8)
#define FPC1080_MAX_FRAMES			256
#define FPC1080_IMAGE_BUFFER_SIZE		(FPC1080_MAX_FRAMES * \
						 FPC1080_FRAME_SIZE)

#define FPC1080_SPI_CLOCK_SPEED			(11 * 1000 * 1000)

#define FPC1080_DEV_NAME                        "fpc1080"
#define FPC1080_CLASS_NAME                      "fpsensor"
#define FPC1080_WORKER_THREAD_NAME		"fpc1080worker"
#define FPC1080_MOUSE_DEV_NAME                        "fpc1080Mouse"
#define FPC1080_TOUCH_PAD_DEV_NAME              "fpc1080TouchPad"


//#define DEBUG_FPC1080

#ifdef DEBUG_FPC1080
#define DEBUG_PRINT(fmt,...) printk(fmt,##__VA_ARGS__);
#else
#define DEBUG_PRINT(fmt,...) do{}while(0)
#endif

#define FPC1080_IOCTL_MAGIC_NO			0xFC

#define FPC1080_IOCTL_START_CAPTURE	_IO(FPC1080_IOCTL_MAGIC_NO, 0)
#define FPC1080_IOCTL_ABORT_CAPTURE	_IO(FPC1080_IOCTL_MAGIC_NO, 1)
#define FPC1080_IOCTL_CAPTURE_SINGLE	_IOW(FPC1080_IOCTL_MAGIC_NO, 2, int)
#define FPC1080_IOCTL_TEST_SENSOR		_IO(FPC1080_IOCTL_MAGIC_NO, 3)

#define FPC1080_KEY_FINGER_PRESENT		188

#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
//#define SUPPORT_MOUSE_DRAGGING
////#define IMAGE_NAVIGATION
//#define SUPPORT_DOUBLE_TAP
//#define SUPPORT_VOLUME_CONTROL

#define FPC1080_POLL_INTERVAL 15000
#define FPC1080_POLL_INTERVAL_TOL 2000
#define FPC1080_POLL_INTERVAL_TAP 150000
#define FPC1080_POLL_INTERVAL_VOLUME 3000000

#define FLOAT_MAX 100
////#define PTR_NORMALIZE

#define FNGR_ST_MOVING		0
#define FNGR_ST_DETECTED		1
#define FNGR_ST_LOST			2
#define FNGR_ST_TAP			3
#define FNGR_ST_HOLD			4
#define FNGR_ST_L_HOLD		5
#define FNGR_ST_DOUBLE_TAP	6

#define MOUSE_SCROLL_NONE	0
#define MOUSE_SCROLL_NORMAL	1
#define MOUSE_SCROLL_REVERSED	2
#define MOUSE_SCROLL_LAUNCHER	3  //EK add 20130218 for NEC

#endif


/*#define CONFIG_FPC1080_TOUCH_PAD*/
#define DEVICE_WIDTH 720
#define DEVICE_HEIGHT 1280

enum {
	FPC1080_THREAD_IDLE_MODE = 0,
	FPC1080_THREAD_CAPTURE_MODE,
	FPC1080_THREAD_NAV_MODE,
	FPC1080_THREAD_EXIT
};

/* -------------------------------------------------------------------- */
/* global variables							*/
/* -------------------------------------------------------------------- */
static int fpc1080_device_count = 8;

/* -------------------------------------------------------------------- */
/* fpc1080 data types							*/
/* -------------------------------------------------------------------- */
struct fpc1080_thread_task {
	int mode;
	int should_stop;
	struct semaphore sem_idle;
	wait_queue_head_t wait_job;
	struct task_struct *thread;
};

struct fpc1080_diag {
	u8 selftest;
	u32 capture_time;
	u32 frames_captured;
	u32 frames_stored;
	u32 get_id;
};

struct fpc1080_data {
	struct spi_device *spi;
	struct class *class;
	struct device *device;
	struct cdev cdev;
	dev_t devno;
	u32 reset_gpio;
	u32 irq_gpio;
	u32 irq;
	u32 data_offset;
	u32 avail_data;
	wait_queue_head_t waiting_data_avail;
	int interrupt_done;
	wait_queue_head_t waiting_interrupt_return;
	struct semaphore mutex;
	u8 *huge_buffer;
	u32 current_frame;
	int capture_done;
	struct fpc1080_thread_task thread_task;
	struct fpc1080_adc_setup adc_setup;
	struct fpc1080_diag diag;
	struct early_suspend		early_suspend;

#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
	struct fpc1080_nav_settings nav_settings;
	int nav_sum_x;
	int nav_sum_y;
	u8* tx_buf;
	u8* prev_img_buf;
	u8* cur_img_buf;
        bool l_button_down;
	int tap_status;
	unsigned long volume_time;
	unsigned long time;
	bool volume_enable;
	struct task_struct *nav_task;
	struct input_dev *nav_dev;
	u8 *rx_buf;
	struct input_dev *mouse_dev;
#endif


#if defined(CONFIG_FPC1080_TOUCH_PAD)
				struct input_dev *touch_pad_dev;
#endif



};

struct fpc1080_attribute {
	struct device_attribute attr;
	size_t offset;
};

/* -------------------------------------------------------------------- */
/* function prototypes							*/
/* -------------------------------------------------------------------- */
static int __init fpc1080_init(void);

static void __exit fpc1080_exit(void);

static int __devinit fpc1080_probe(struct spi_device *spi);

static int __devexit fpc1080_remove(struct spi_device *spi);

static int fpc1080_suspend(struct device *dev);

static int fpc1080_resume(struct device *dev);

static int fpc1080_open(struct inode *inode, struct file *file);

static ssize_t fpc1080_write(struct file *file, const char *buff,
					size_t count, loff_t *ppos);

static ssize_t fpc1080_read(struct file *file, char *buff,
				size_t count, loff_t *ppos);

static int fpc1080_release(struct inode *inode, struct file *file);

static unsigned int fpc1080_poll(struct file *file, poll_table *wait);

static long fpc1080_ioctl(struct file *filp,
			  unsigned int cmd,
			  unsigned long arg);

static int fpc1080_reset(struct fpc1080_data *fpc1080);

static ssize_t fpc1080_show_attr_adc_setup(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t fpc1080_store_attr_adc_setup(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

static ssize_t fpc1080_show_attr_diag(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t fpc1080_store_attr_diag(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);


#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
static ssize_t fpc1080_show_attr_nav_settings(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t fpc1080_store_attr_nav_settings(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

static int fpc1080_stop_navigation(struct fpc1080_data *fpc1080);
static int fpc1080_start_navigation(struct fpc1080_data *fpc1080);
static int fpc1080_nav_task(struct fpc1080_data *fpc1080);
#endif

/*static int fpc1080_stop_navigation(struct fpc1080_data *fpc1080);*/
/*static int fpc1080_start_navigation(struct fpc1080_data *fpc1080);*/

static int fpc1080_wreg(struct fpc1080_data *fpc1080, u8 addr, u8 value);
static int fpc1080_write_adc_setup(struct fpc1080_data *fpc1080);
static int fpc1080_cmd_wait_irq(struct fpc1080_data *fpc1080, u8 cmd, u8 *irq);
static int fpc1080_spi_rd_image(struct fpc1080_data *fpc1080, bool capture);
static int fpc1080_selftest_short(struct fpc1080_data *fpc1080);
static int fpc1080_get_id_short(struct fpc1080_data *fpc1080);

static void fpc1080_early_suspend(struct early_suspend *h);
static void fpc1080_late_resume(struct early_suspend *h);

#if defined CONFIG_FPC1080_TOUCH_PAD
static void dispatch_touch_pad_event( struct fpc1080_data *fpc1080, int x, int y, int finger_status );
#endif
/* -------------------------------------------------------------------- */
/* External interface							*/
/* -------------------------------------------------------------------- */
module_init(fpc1080_init);
module_exit(fpc1080_exit);

static const struct dev_pm_ops fpc1080_pm = {
	.suspend = fpc1080_suspend,
	.resume = fpc1080_resume
};

static struct spi_driver fpc1080_driver = {
	.driver = {
		.name	= FPC1080_DEV_NAME,
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
		.pm = &fpc1080_pm,
	},
	.probe	= fpc1080_probe,
	.remove	= __devexit_p(fpc1080_remove)
};

static const struct file_operations fpc1080_fops = {
	.owner = THIS_MODULE,
	.open  = fpc1080_open,
	.write = fpc1080_write,
	.read  = fpc1080_read,
	.release = fpc1080_release,
	.poll = fpc1080_poll,
	.unlocked_ioctl = fpc1080_ioctl
};

/* -------------------------------------------------------------------- */
/* devfs								*/
/* -------------------------------------------------------------------- */
#define FPC1080_ATTR(__grp, __field, __mode)				\
{									\
	.attr = __ATTR(__field, (__mode),				\
	fpc1080_show_attr_##__grp,					\
	fpc1080_store_attr_##__grp),					\
	.offset = offsetof(struct fpc1080_##__grp, __field)		\
}

#define FPC1080_DEV_ATTR(_grp, _field, _mode)				\
struct fpc1080_attribute fpc1080_attr_##_field =			\
					FPC1080_ATTR(_grp, _field, (_mode))

#define ADC_SETUP_MODE (S_IWUSR | S_IWGRP | S_IWOTH)

static FPC1080_DEV_ATTR(adc_setup, gain,	ADC_SETUP_MODE);
static FPC1080_DEV_ATTR(adc_setup, offset,	ADC_SETUP_MODE);
static FPC1080_DEV_ATTR(adc_setup, pxl_setup,	ADC_SETUP_MODE);

static struct attribute *fpc1080_adc_attrs[] = {
	&fpc1080_attr_gain.attr.attr,
	&fpc1080_attr_offset.attr.attr,
	&fpc1080_attr_pxl_setup.attr.attr,
	NULL
};

static const struct attribute_group fpc1080_adc_attr_group = {
	.attrs = fpc1080_adc_attrs,
	.name = "adc_setup"
};

#define DIAG_MODE (S_IRUSR | S_IRGRP | S_IROTH)

static FPC1080_DEV_ATTR(diag, selftest,		DIAG_MODE);
static FPC1080_DEV_ATTR(diag, capture_time,	DIAG_MODE);
static FPC1080_DEV_ATTR(diag, frames_captured,	DIAG_MODE);
static FPC1080_DEV_ATTR(diag, frames_stored,	DIAG_MODE);
static FPC1080_DEV_ATTR(diag, get_id,	DIAG_MODE); ////

static struct attribute *fpc1080_diag_attrs[] = {
	&fpc1080_attr_selftest.attr.attr,
	&fpc1080_attr_capture_time.attr.attr,
	&fpc1080_attr_frames_captured.attr.attr,
	&fpc1080_attr_frames_stored.attr.attr,
	&fpc1080_attr_get_id.attr.attr,
	NULL
};

static const struct attribute_group fpc1080_diag_attr_group = {
	.attrs = fpc1080_diag_attrs,
	.name = "diag"
};


#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
#define NAV_SETTINGS_MODE ( S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

static FPC1080_DEV_ATTR(nav_settings, p_sensitivity_key,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, p_sensitivity_ptr,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, p_multiplier_x,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, p_multiplier_y,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, multiplier_key_accel,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, multiplier_ptr_accel,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, threshold_key_start,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, threshold_key_accel,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, threshold_key_dispatch,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, threshold_ptr_start,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, threshold_ptr_accel,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, duration_key_clear,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, duration_ptr_clear,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, mouse_enable,	NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, mouse_scroll,		NAV_SETTINGS_MODE);
static FPC1080_DEV_ATTR(nav_settings, mouse_scroll_skip_frame,	NAV_SETTINGS_MODE);

static struct attribute *fpc1080_nav_settings_attrs[] = {
	&fpc1080_attr_p_sensitivity_key.attr.attr,
	&fpc1080_attr_p_sensitivity_ptr.attr.attr,
	&fpc1080_attr_p_multiplier_x.attr.attr,
	&fpc1080_attr_p_multiplier_y.attr.attr,
	&fpc1080_attr_multiplier_key_accel.attr.attr,
	&fpc1080_attr_multiplier_ptr_accel.attr.attr,
	&fpc1080_attr_threshold_key_start.attr.attr,
	&fpc1080_attr_threshold_key_accel.attr.attr,
	&fpc1080_attr_threshold_key_dispatch.attr.attr,
	&fpc1080_attr_threshold_ptr_start.attr.attr,
	&fpc1080_attr_threshold_ptr_accel.attr.attr,
	&fpc1080_attr_duration_key_clear.attr.attr,
	&fpc1080_attr_duration_ptr_clear.attr.attr,
	&fpc1080_attr_mouse_enable.attr.attr,
	&fpc1080_attr_mouse_scroll.attr.attr,
	&fpc1080_attr_mouse_scroll_skip_frame.attr.attr,
	NULL
};

static const struct attribute_group fpc1080_nav_settings_attr_group = {
	.attrs = fpc1080_nav_settings_attrs,
	.name = "nav_settings"
};
#endif


/* -------------------------------------------------------------------- */
/* function definitions							*/
/* -------------------------------------------------------------------- */
static ssize_t fpc1080_show_attr_adc_setup(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return -ENOTTY;
	/*u8 *target;
	struct fpc1080_data *fpc1080;
	struct fpc1080_attribute *fpc_attr;
	fpc1080 = dev_get_drvdata(dev);
	fpc_attr = container_of(attr, struct fpc1080_attribute, attr);

	target = ((u8 *)&fpc1080->nav_settings) + fpc_attr->offset;
	return scnprintf(buf, PAGE_SIZE, "%s: %i\n", attr->attr.name,
				*target);*/

}

/* -------------------------------------------------------------------- */
static ssize_t fpc1080_store_attr_adc_setup(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	u8 *target;
	u8 tmp;
	struct fpc1080_data *fpc1080;
	struct fpc1080_attribute *fpc_attr;
	fpc1080 = dev_get_drvdata(dev);
	fpc_attr = container_of(attr, struct fpc1080_attribute, attr);

	if ((sscanf(buf, "%hhu", &tmp)) <= 0)
		return -EINVAL;

	target = ((u8 *)&fpc1080->adc_setup) + fpc_attr->offset;
	*target = tmp;

	return strnlen(buf, count);
}

/* -------------------------------------------------------------------- */
static ssize_t fpc1080_show_attr_diag(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct fpc1080_data *fpc1080;
	struct fpc1080_attribute *fpc_attr;

	fpc1080 = dev_get_drvdata(dev);
	fpc_attr = container_of(attr, struct fpc1080_attribute, attr);

	if(fpc_attr->offset == offsetof(struct fpc1080_diag, selftest)) {

		fpc1080_selftest_short(fpc1080);

		return scnprintf(buf, PAGE_SIZE, "%i\n",
				 fpc1080->diag.selftest);
	}

	if(fpc_attr->offset == offsetof(struct fpc1080_diag, capture_time)) {
		return scnprintf(buf, PAGE_SIZE, "%i\n",
				 fpc1080->diag.capture_time);
	}

	if(fpc_attr->offset == offsetof(struct fpc1080_diag, frames_captured)) {
		return scnprintf(buf, PAGE_SIZE, "%i\n",
				 fpc1080->diag.frames_captured);
	}

	if(fpc_attr->offset == offsetof(struct fpc1080_diag, frames_stored)) {
		return scnprintf(buf, PAGE_SIZE, "%i\n",
				 fpc1080->diag.frames_stored);
	}

	if(fpc_attr->offset == offsetof(struct fpc1080_diag, get_id)) {

		fpc1080_get_id_short(fpc1080);

		return scnprintf(buf, PAGE_SIZE, "%i\n",
				 fpc1080->diag.get_id);
	}
	return -ENOENT;
}

/* -------------------------------------------------------------------- */
static ssize_t fpc1080_store_attr_diag(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	return -EPERM;
}


#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
/* -------------------------------------------------------------------- */
static ssize_t fpc1080_show_attr_nav_settings(struct device *dev,
								struct device_attribute *attr, char *buf)
{
	u8 *target;
	struct fpc1080_data *fpc1080;
	struct fpc1080_attribute *fpc_attr;
	fpc1080 = dev_get_drvdata(dev);
	fpc_attr = container_of(attr, struct fpc1080_attribute, attr);

	target = ((u8 *)&fpc1080->nav_settings) + fpc_attr->offset;
	return scnprintf(buf, PAGE_SIZE, "%s: %i\n", attr->attr.name,
									*target);

}

static ssize_t fpc1080_store_attr_nav_settings(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t count)
{
	u8 *target;
	u8 tmp;
	struct fpc1080_data *fpc1080;
	struct fpc1080_attribute *fpc_attr;
	fpc1080 = dev_get_drvdata(dev);
	fpc_attr = container_of(attr, struct fpc1080_attribute, attr);

	if ((sscanf(buf, "%hhu", &tmp)) <= 0)
					return -EINVAL;

	target = ((u8 *)&fpc1080->nav_settings) + fpc_attr->offset;
	*target = tmp;

	return strnlen(buf, count);

}

/* -------------------------------------------------------------------- */

static void init_enhanced_navi_setting( struct fpc1080_data *fpc1080 )
{
	fpc1080->nav_settings.p_sensitivity_key = 200; //originally 20 in navigation event
	fpc1080->nav_settings.p_sensitivity_ptr = 200;
	fpc1080->nav_settings.p_multiplier_x = 110;
	fpc1080->nav_settings.p_multiplier_y = 110;
	fpc1080->nav_settings.multiplier_key_accel = 2;
	fpc1080->nav_settings.multiplier_ptr_accel = 1;
	fpc1080->nav_settings.threshold_key_start = 10;
	fpc1080->nav_settings.threshold_key_accel = 55;
	fpc1080->nav_settings.threshold_key_dispatch = 130;
	fpc1080->nav_settings.threshold_ptr_start = 5;
	fpc1080->nav_settings.threshold_ptr_accel = 15;
	fpc1080->nav_settings.duration_key_clear = 100;
	fpc1080->nav_settings.duration_ptr_clear = 100;
	fpc1080->nav_settings.mouse_scroll_skip_frame = 5; //originally 5

	// for debugging
	fpc1080->nav_settings.mouse_enable = 0;
	fpc1080->nav_settings.mouse_scroll = MOUSE_SCROLL_REVERSED;

	printk("[fp] %s: end \n", __func__);
}

/* -------------------------------------------------------------------- */

#ifdef PTR_NORMALIZE
static int div_rate( int remain_data ) {

     int check_data = abs( remain_data );

     DEBUG_PRINT("remain_data : %d\n", remain_data );

     if(check_data > 500)
	  return 5;
     else if(check_data > 40)
	  return 5;
     else if(check_data > 20)
	  return 4;
     else if(check_data > 10)
	  return 3;
     else
	  return 2;

}
#endif

/* -------------------------------------------------------------------- */
#if 0
static void dispatch_mouse_event( struct fpc1080_data *fpc1080, int x, int y, int finger_status )
{
	 int abs_x, abs_y;
	 int sign_x, sign_y;

#ifdef PTR_NORMALIZE
	 int div_x;
	 int div_y;
#endif

	 const int SCROLL_ACTIVATION_RANGE = 20;
	 static int active = 0;
	 static unsigned long prev_time = 0;
	 static int scroll_frame = 0;

	 unsigned long current_time = jiffies * 1000 / HZ;
	 int mouse_scroll = fpc1080->nav_settings.mouse_scroll;

	 switch ( finger_status )
	 {
		 case FNGR_ST_TAP:

		  input_report_key( fpc1080->mouse_dev, BTN_LEFT, 1 );
		  input_sync(fpc1080->mouse_dev);
		  input_report_key( fpc1080->mouse_dev, BTN_LEFT, 0 );
		  input_sync(fpc1080->mouse_dev);
		  return;
		 case FNGR_ST_HOLD:

		  input_report_key( fpc1080->mouse_dev, BTN_RIGHT, 1 );
		  input_sync(fpc1080->mouse_dev);
		  input_report_key( fpc1080->mouse_dev, BTN_RIGHT, 0 );
		  input_sync(fpc1080->mouse_dev);
		  return;

#ifdef SUPPORT_MOUSE_DRAGGING
		 case FNGR_ST_L_HOLD:

		  fpc1080->l_button_down = true;
		  input_report_key( fpc1080->mouse_dev, BTN_LEFT, 1 );
		  input_sync(fpc1080->mouse_dev);
		  break;
#endif
#if defined (SUPPORT_DOUBLE_TAP)
		 case FNGR_ST_DOUBLE_TAP:
		  input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 1 );
		  input_sync(fpc1080->mouse_dev);
		  input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 0 );
		  input_sync(fpc1080->mouse_dev);
		  fpc1080->volume_enable = true;
		  fpc1080->volume_time = jiffies;
		 break;
#endif
		 case FNGR_ST_LOST:
		  if ( fpc1080->l_button_down )
		  {
			   input_report_key( fpc1080->mouse_dev, BTN_LEFT, 0 );
			   input_sync(fpc1080->mouse_dev);
			   fpc1080->l_button_down = false;
		  }
		  /* Here we don't use break on purpose */
		 case FNGR_ST_DETECTED:
		  fpc1080->nav_settings.sum_x = fpc1080->nav_settings.sum_y = 0;
		  scroll_frame = 0;
		  active = 0;
		  break;
	 }

	 if ( mouse_scroll != MOUSE_SCROLL_NONE )
	 {
		  int direction = 0;


#if defined ENABLE_TOUCH_EVENT_IN_MOUSE
		if(mouse_scroll == MOUSE_SCROLL_LAUNCHER)
		{

			dispatch_touch_pad_event(fpc1080,  x,  y, finger_status );
			return;

		}
		else
		{
#endif
			  fpc1080->nav_settings.sum_y += y;

			  if ( abs( fpc1080->nav_settings.sum_y ) > SCROLL_ACTIVATION_RANGE )
			  {
				   direction = fpc1080->nav_settings.sum_y > 0 ? 1 : -1;
				   if ( mouse_scroll == MOUSE_SCROLL_REVERSED ) direction *= -1;
			  }

			  if ( direction )
			  {
				   scroll_frame++;
				   if ( scroll_frame % fpc1080->nav_settings.mouse_scroll_skip_frame == 0 )
				   {
					input_report_rel( fpc1080->mouse_dev, REL_WHEEL, direction );
					input_sync( fpc1080->mouse_dev );
					scroll_frame = 0;
				   }
			  }


#if defined ENABLE_TOUCH_EVENT_IN_MOUSE
		}
 #endif
	 }
	 else
	 {
		  sign_x = x > 0 ? 1 : -1;
		  sign_y = y > 0 ? 1 : -1;
		  abs_x = x * sign_x;
		  abs_y = y * sign_y;

		  if ( prev_time != 0 && ( current_time - prev_time > fpc1080->nav_settings.duration_ptr_clear ))
		  {

			   fpc1080->nav_settings.sum_x = fpc1080->nav_settings.sum_y = 0;
			   active = 0;
		  }
		  prev_time = current_time;

		  if ( !active && abs_x < fpc1080->nav_settings.threshold_ptr_start &&
			   abs_y < fpc1080->nav_settings.threshold_ptr_start )
		  {

			   return;
		  }
		  active = 1;

		  if ( abs_x > fpc1080->nav_settings.threshold_ptr_accel )
			   x = ( fpc1080->nav_settings.threshold_ptr_accel + ( abs_x - fpc1080->nav_settings.threshold_ptr_accel ) * fpc1080->nav_settings.multiplier_ptr_accel ) * sign_x;
		  if ( abs_y > fpc1080->nav_settings.threshold_ptr_accel )
			   y = ( fpc1080->nav_settings.threshold_ptr_accel + ( abs_y - fpc1080->nav_settings.threshold_ptr_accel ) * fpc1080->nav_settings.multiplier_ptr_accel ) * sign_y;

		  // Correct axis factor
		  x = x * fpc1080->nav_settings.p_multiplier_x / FLOAT_MAX;
		  y = y * fpc1080->nav_settings.p_multiplier_y / FLOAT_MAX;

		  // Adjust Sensitivity
		  x = x * fpc1080->nav_settings.p_sensitivity_ptr / FLOAT_MAX;
		  y = y * fpc1080->nav_settings.p_sensitivity_ptr / FLOAT_MAX;

#ifdef PTR_NORMALIZE

		  if ( x * fpc1080->nav_settings.sum_x < 0 )
		  {

			   fpc1080->nav_settings.sum_x = 0;
		  }

		  if ( y * fpc1080->nav_settings.sum_y < 0 )
		  {

			   fpc1080->nav_settings.sum_y = 0;
		  }

		  // Normalize
		  fpc1080->nav_settings.sum_x += x;
		  fpc1080->nav_settings.sum_y += y;

	//		   do {
		  div_x = div_rate( fpc1080->nav_settings.sum_x );
		  div_y = div_rate( fpc1080->nav_settings.sum_y );
	//	   int div_M = div_x > div_y ? div_x : div_y;

		  x = fpc1080->nav_settings.sum_x / div_x;
		  y = fpc1080->nav_settings.sum_y / div_y;

		  if ( x == 0 ) {
			   if ( fpc1080->nav_settings.sum_x > 0 )
				x = 1;
			   else
				x = -1;
		  }

		  if ( y == 0 ) {
			   if ( fpc1080->nav_settings.sum_y > 0 )
				y = 1;
			   else
				y = -1;
		  }

		  fpc1080->nav_settings.sum_x -= x;
		  fpc1080->nav_settings.sum_y -= y;
#endif
		  input_report_rel( fpc1080->mouse_dev, REL_X,	x );
		  input_report_rel( fpc1080->mouse_dev, REL_Y,	y );
		  input_sync(fpc1080->mouse_dev );
	 }
}

/* -------------------------------------------------------------------- */

static void dispatch_trackpad_event( struct fpc1080_data *fpc1080, int x, int y, int finger_status )
{
	 int abs_x, abs_y;
	 int sign_x, sign_y;

	 if ( finger_status == FNGR_ST_TAP )
	 {

	  input_report_key( fpc1080->nav_dev, KEY_ENTER, 1 );
	  input_sync(fpc1080->nav_dev);
	  input_report_key( fpc1080->nav_dev, KEY_ENTER, 0 );
	  input_sync(fpc1080->nav_dev);
	  return;
	 }
#if defined (SUPPORT_DOUBLE_TAP)
	 else if( finger_status == FNGR_ST_DOUBLE_TAP ){
		 input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 1 );
		 input_sync(fpc1080->mouse_dev);
		 input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 0 );
		 input_sync(fpc1080->mouse_dev);
		  fpc1080->volume_enable = true;
		  fpc1080->volume_time = jiffies;
	   return;
	 }
#endif

	 else if(finger_status == FNGR_ST_HOLD)
	{

		input_report_key(fpc1080->nav_dev, KEY_SLEEP, 1);
		input_sync(fpc1080->nav_dev);

		input_report_key(fpc1080->nav_dev, KEY_SLEEP, 0);
		input_sync(fpc1080->nav_dev);


		 return;
	}

	 // Handle Acceleration
	 sign_x = x > 0 ? 1 : -1;
	 sign_y = y > 0 ? 1 : -1;
	 abs_x = x * sign_x;
	 abs_y = y * sign_y;

	 abs_x = x > 0 ? x : -x;
	 abs_y = y > 0 ? y : -y;

	 if ( abs_x > fpc1080->nav_settings.threshold_key_accel )
	  x = ( fpc1080->nav_settings.threshold_key_accel + ( abs_x - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_x;
	 if ( abs_y > fpc1080->nav_settings.threshold_key_accel )
	  y = ( fpc1080->nav_settings.threshold_key_accel + ( abs_y - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_y;


	 // Correct axis factor
	 x = x * fpc1080->nav_settings.p_multiplier_x / FLOAT_MAX;
	 y = y * fpc1080->nav_settings.p_multiplier_y / FLOAT_MAX;


	 // Adjust Sensitivity
	 x = x * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;
	 y = y * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;

	 input_report_rel(fpc1080->nav_dev, REL_X, x);
	 input_report_rel(fpc1080->nav_dev, REL_Y, y);

	 input_sync(fpc1080->nav_dev);


}
#endif
/* -------------------------------------------------------------------- */
#if defined (SUPPORT_VOLUME_CONTROL) ////
static void dispatch_dpad_event( struct fpc1080_data *fpc1080, int x, int y, bool finger_lost )
{
	 int abs_x, abs_y;
	 int sign_x, sign_y;
	 int key_code;
	 int reset = 0;
	 static int active = 0;
	 static int issued_key = 0;
	 static unsigned long prev_time = 0;
	 unsigned long current_time = jiffies * 1000 / HZ;

	 // Handle Acceleration
	 sign_x = x > 0 ? 1 : -1;
	 sign_y = y > 0 ? 1 : -1;
	 abs_x = x * sign_x;
	 abs_y = y * sign_y;

	 if ( prev_time != 0 && ( current_time - prev_time > fpc1080->nav_settings.duration_key_clear )) {

	  reset = 1;
	 }
	 prev_time = current_time;

	 abs_x = x > 0 ? x : -x;
	 abs_y = y > 0 ? y : -y;

	 if ( abs_x > fpc1080->nav_settings.threshold_key_accel )
	  x = ( fpc1080->nav_settings.threshold_key_accel + ( abs_x - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_x;
	 if ( abs_y > fpc1080->nav_settings.threshold_key_accel )
	  y = ( fpc1080->nav_settings.threshold_key_accel + ( abs_y - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_y;

	 // Correct axis factor
	 x = x * fpc1080->nav_settings.p_multiplier_x / FLOAT_MAX;
	 y = y * fpc1080->nav_settings.p_multiplier_y / FLOAT_MAX;

	 // Adjust Sensitivity
	 x = x * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;
	 y = y * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;

	 if ( abs_x > abs_y )
	 {
	  fpc1080->nav_settings.sum_x += x;

	  if ( issued_key ) {
		   if ( x < 0 && issued_key != KEY_LEFT )
			reset = 1;
		   if ( x > 0 && issued_key != KEY_RIGHT )
			reset = 1;
	  }
	 }
	 else
	 {
	  fpc1080->nav_settings.sum_y += y;

	  if ( issued_key ) {
		   if ( y < 0 && issued_key != KEY_UP )
			reset = 1;
		   if ( y > 0 && issued_key != KEY_DOWN )
			reset = 1;
	  }
	 }

	 if ( reset ) {

	  fpc1080->nav_settings.sum_x = fpc1080->nav_settings.sum_y = 0;
	  active = 0;
	  issued_key = 0;
	 }

	 if ( !active ) {
	  if ( fpc1080->nav_settings.sum_x > fpc1080->nav_settings.threshold_key_start ) {
		   key_code = KEY_RIGHT;
		   fpc1080->nav_settings.sum_x -= fpc1080->nav_settings.threshold_key_start;
		   fpc1080->nav_settings.sum_y = 0;
	  } else if ( fpc1080->nav_settings.sum_x < -fpc1080->nav_settings.threshold_key_start ) {
		   key_code = KEY_LEFT;
		   fpc1080->nav_settings.sum_x += fpc1080->nav_settings.threshold_key_start;
		   fpc1080->nav_settings.sum_y = 0;
	  } else if ( fpc1080->nav_settings.sum_y < -fpc1080->nav_settings.threshold_key_start ) {
		   key_code = KEY_UP;
		   fpc1080->nav_settings.sum_y += fpc1080->nav_settings.threshold_key_start;
		   fpc1080->nav_settings.sum_x = 0;
	  } else if ( fpc1080->nav_settings.sum_y > fpc1080->nav_settings.threshold_key_start ) {
		   key_code = KEY_DOWN;
		   fpc1080->nav_settings.sum_y -= fpc1080->nav_settings.threshold_key_start;
		   fpc1080->nav_settings.sum_x = 0;
	  } else {
		   key_code = 0;
	  }

	  if ( key_code ) {
		   active = 1;

#if defined (SUPPORT_VOLUME_CONTROL)
		if(key_code == KEY_UP ||key_code == KEY_RIGHT){
			key_code	= KEY_VOLUMEUP;
		}
		else if(key_code == KEY_DOWN ||key_code == KEY_LEFT){
			key_code	= KEY_VOLUMEDOWN;
		}

		fpc1080->volume_time = jiffies;

		   input_report_key(fpc1080->nav_dev, key_code, 1);
		   input_sync(fpc1080->nav_dev);
		   input_report_key(fpc1080->nav_dev, key_code, 0);
		   input_sync(fpc1080->nav_dev);
#else
		   input_report_key(fpc1080->nav_dev, key_code, 1);
		   input_report_key(fpc1080->nav_dev, key_code, 0);
#endif

		   issued_key = key_code;

	  } else {
		   return;
	  }
	 }

	 //do
	 {
	  if ( fpc1080->nav_settings.sum_x > fpc1080->nav_settings.threshold_key_dispatch ) {
		   key_code = KEY_RIGHT;
		   fpc1080->nav_settings.sum_x -= fpc1080->nav_settings.threshold_key_dispatch;
		   fpc1080->nav_settings.sum_y = 0;
	  } else if ( fpc1080->nav_settings.sum_x < -fpc1080->nav_settings.threshold_key_dispatch ) {
		   key_code = KEY_LEFT;
		   fpc1080->nav_settings.sum_x += fpc1080->nav_settings.threshold_key_dispatch;
		   fpc1080->nav_settings.sum_y = 0;
	  } else if ( fpc1080->nav_settings.sum_y < -fpc1080->nav_settings.threshold_key_dispatch ) {
		   key_code = KEY_UP;
		   fpc1080->nav_settings.sum_y += fpc1080->nav_settings.threshold_key_dispatch;
		   fpc1080->nav_settings.sum_x = 0;
	  } else if ( fpc1080->nav_settings.sum_y > fpc1080->nav_settings.threshold_key_dispatch ) {
		   key_code = KEY_DOWN;
		   fpc1080->nav_settings.sum_y -= fpc1080->nav_settings.threshold_key_dispatch;
		   fpc1080->nav_settings.sum_x = 0;
	  } else {
		   key_code = 0;
	  }

	  if ( key_code ) {
#if defined (SUPPORT_VOLUME_CONTROL)
		if(key_code == KEY_UP ||key_code == KEY_RIGHT){
			key_code	= KEY_VOLUMEUP;
		}
		else if(key_code == KEY_DOWN ||key_code == KEY_LEFT){
			key_code	= KEY_VOLUMEDOWN;
		}

		fpc1080->volume_time = jiffies;
		input_report_key(fpc1080->nav_dev, key_code, 1);
		input_sync(fpc1080->nav_dev);
		input_report_key(fpc1080->nav_dev, key_code, 0);
		input_sync(fpc1080->nav_dev);
#else
		input_report_key(fpc1080->nav_dev, key_code, 1);
		input_report_key(fpc1080->nav_dev, key_code, 0);
#endif

		   issued_key = key_code;

	  }
	 }
	 //while ( key_code );
}
#endif////
#endif





#if defined CONFIG_FPC1080_TOUCH_PAD

static void dispatch_touch_pad_event( struct fpc1080_data *fpc1080, int x, int y, int finger_status ){
	int sign_x, sign_y;
	int abs_x, abs_y;


	switch(finger_status){
	case FNGR_ST_DETECTED:
		fpc1080->nav_sum_x = 360;
		fpc1080->nav_sum_y = 640;
		input_report_abs(fpc1080->touch_pad_dev, ABS_X, fpc1080->nav_sum_x);
		input_report_abs(fpc1080->touch_pad_dev, ABS_Y, fpc1080->nav_sum_y);
		input_report_abs(fpc1080->touch_pad_dev, ABS_Z, 0);
		input_report_key(fpc1080->touch_pad_dev, BTN_TOUCH, 1);
		input_sync(fpc1080->touch_pad_dev);

		break;
	case FNGR_ST_LOST:
	case FNGR_ST_TAP:
		input_report_abs(fpc1080->touch_pad_dev, ABS_X, fpc1080->nav_sum_x);
		input_report_abs(fpc1080->touch_pad_dev, ABS_Y, fpc1080->nav_sum_y);
		input_report_abs(fpc1080->touch_pad_dev, ABS_Z, 0);
		input_report_key(fpc1080->touch_pad_dev, BTN_TOUCH, 0);
		input_sync(fpc1080->touch_pad_dev);

		break;


#if defined (SUPPORT_DOUBLE_TAP)
    case FNGR_ST_DOUBLE_TAP:
        input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 1 );
        input_sync(fpc1080->mouse_dev);
        input_report_key( fpc1080->mouse_dev, KEY_VOLUMEUP, 0 );
        input_sync(fpc1080->mouse_dev);
        fpc1080->volume_enable = true;
        fpc1080->volume_time = jiffies;
        return;
#endif



    case FNGR_ST_HOLD:


	   input_report_key(fpc1080->nav_dev, KEY_SLEEP, 1);
	   input_sync(fpc1080->nav_dev);

	   input_report_key(fpc1080->nav_dev, KEY_SLEEP, 0);
	   input_sync(fpc1080->nav_dev);


	return;

	case FNGR_ST_MOVING:
		 sign_x = x > 0 ? 1 : -1;
		 sign_y = y > 0 ? 1 : -1; //reverse direction
		 abs_x = x > 0 ? x : -x;
		 abs_y = y > 0 ? y : -y;

		 if ( abs_x > fpc1080->nav_settings.threshold_key_accel )
		  x = ( fpc1080->nav_settings.threshold_key_accel + ( abs_x - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_x;
		 if ( abs_y > fpc1080->nav_settings.threshold_key_accel )
		  y = ( fpc1080->nav_settings.threshold_key_accel + ( abs_y - fpc1080->nav_settings.threshold_key_accel ) * fpc1080->nav_settings.multiplier_key_accel ) * sign_y;

		 x = x * fpc1080->nav_settings.p_multiplier_x / FLOAT_MAX;
		 y = y * fpc1080->nav_settings.p_multiplier_y / FLOAT_MAX;

		 x = x * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;
		 y = y * fpc1080->nav_settings.p_sensitivity_key / FLOAT_MAX;


		 if(x != 0 || y != 0){
			 if(abs(x) > abs(y)){
				int newX = fpc1080->nav_sum_x;
				newX += x;
				if(newX < 0){
					newX = 0;
				}
				else if(newX >= DEVICE_WIDTH){
					newX = DEVICE_WIDTH -1;
				}

				if(newX != fpc1080->nav_sum_x){

					fpc1080->nav_sum_x = newX;
					input_report_abs(fpc1080->touch_pad_dev, ABS_X, fpc1080->nav_sum_x);
					input_report_abs(fpc1080->touch_pad_dev, ABS_Y, fpc1080->nav_sum_y);
					input_report_abs(fpc1080->touch_pad_dev, ABS_Z, 0);
					input_report_key(fpc1080->touch_pad_dev, BTN_TOUCH, 1);
					input_sync(fpc1080->touch_pad_dev);
				}
			 }
			 else{
				int newY = fpc1080->nav_sum_y;
				newY -= y;
				if(newY < 0){
					newY = 0;
				}
				else if(newY >= DEVICE_HEIGHT){
					newY = DEVICE_HEIGHT -1;
				}

				if(newY != fpc1080->nav_sum_y){

					fpc1080->nav_sum_y = newY;

					input_report_abs(fpc1080->touch_pad_dev, ABS_X, fpc1080->nav_sum_x);
					input_report_abs(fpc1080->touch_pad_dev, ABS_Y, fpc1080->nav_sum_y);
					input_report_abs(fpc1080->touch_pad_dev, ABS_Z, 0);
					input_report_key(fpc1080->touch_pad_dev, BTN_TOUCH, 1);
					input_sync(fpc1080->touch_pad_dev);
				}
			 }
		 }
		break;
	}
}
#endif



/* -------------------------------------------------------------------- */
static int fpc1080_wait_for_irq(struct fpc1080_data *fpc1080,
							int timeout)
{
	int result;

	if (!timeout) {
		result = wait_event_interruptible(
				fpc1080->waiting_interrupt_return,
				fpc1080->interrupt_done);
	} else {
		result = wait_event_interruptible_timeout(
				fpc1080->waiting_interrupt_return,
				fpc1080->interrupt_done, timeout);
	}

	if (result < 0) {
		dev_err(&fpc1080->spi->dev, "wait_event_interruptible "
				"interrupted by signal.\n");
		return result;
	}

	if (result || !timeout) {
		fpc1080->interrupt_done = 0;
		return 0;
	}

	return -ETIMEDOUT;
}

/* -------------------------------------------------------------------- */
static int fpc1080_spi_wr_rd(struct fpc1080_data *fpc1080, u8 *tx,
					u8 *rx, unsigned int length)
{
	int error;
	struct spi_message m;
	struct spi_transfer t = {
		.cs_change = 0,
		.delay_usecs = 0,
/*		.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
		.tx_buf = tx,
		.rx_buf = rx,
		.len = length,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 0,
	};

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}

	fpc1080->avail_data = length;
	fpc1080->data_offset = 0;
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_spi_wr_reg(struct fpc1080_data *fpc1080, u8 addr,
						u8 value, unsigned int size)
{
	int error;
	fpc1080->huge_buffer[0] = addr;
	fpc1080->huge_buffer[1] = value;
	error = fpc1080_spi_wr_rd(fpc1080, fpc1080->huge_buffer,
				fpc1080->huge_buffer + size, size);
	if (error)
		return error;

	fpc1080->data_offset = size > FPC1080_SYNCED_REG_SIZE ?
				size + FPC1080_SYNCED_REG_SIZE : size + 1;
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_reset(struct fpc1080_data *fpc1080)
{
	int error = 0;
	int ret = 0;
	int gpio;

	printk("[fp] %s: fpc1080->reset_gpio =%d\n", __func__, fpc1080->reset_gpio);
	gpio = fpc1080->reset_gpio;

	ret = ioext_gpio_set_value(gpio, 0);
	if (ret < 0) {
		pr_err("[fpc] %s: io extender low failed %d\n", __func__, ret);
	}

	udelay(1000);

	ret = ioext_gpio_set_value(gpio, 1);
	if (ret < 0) {
		pr_err("[fpc] %s: io_extender high failed %d\n", __func__, ret);
	}
	udelay(1250);

	error = gpio_get_value(fpc1080->irq_gpio) ? 0 : -EIO;
	if (error) {
		printk(KERN_INFO "[fpc] reset timed out, waiting again..\n");
		udelay(1250);
		error = gpio_get_value(fpc1080->irq_gpio) ? 0 : -EIO;
	}

	disable_irq(fpc1080->irq);
	fpc1080->interrupt_done = 0;
	enable_irq(fpc1080->irq);

	if (error) {
		dev_err(&fpc1080->spi->dev, "irq after reset timed out\n");
		return -EIO;
	}

	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_RD_INTERRUPT_WITH_CLEAR,
									0, 2);


	if (error) {
		pr_err("[fpc] %s: fpc1080_spi_wr_reg failed %d\n", __func__, error);
		return error;
	}

	if (fpc1080->huge_buffer[fpc1080->data_offset] != FPC1080_IRQ_REBOOT) {
		dev_err(&fpc1080->spi->dev, "unexpected response at reset.\n");
		return -EIO;
	}
	fpc1080->data_offset = 0;
	fpc1080->avail_data = 0;
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_capture_single(struct fpc1080_data *fpc1080, int mode)
{
	int error;
	u8 pat1, pat2, irq;
	pat1 = 0x55;
	pat2 = 0xAA;
	printk("%s:\n", __func__);
	switch (mode) {
	case 0:
		break;
	case 1:
		pat1 = 0xAA;
		pat2 = 0x55;

	case 2:
		error = fpc1080_wreg(fpc1080, FPC1080_FNGR_DRIVE_CONF,
				     FPC1080_SPI_FNGR_DRV_TST);
		if (error)
			return error;
		error = fpc1080_wreg(fpc1080, FPC1080_PATTERN1_XREG, pat1);
		if (error)
			return error;
		error = fpc1080_wreg(fpc1080, FPC1080_PATTERN2_XREG, pat2);
		if (error)
			return error;
		break;
	default:
		return -EINVAL;

	}

	error = fpc1080_write_adc_setup(fpc1080);
	if (error)
		return error;
	error = fpc1080_cmd_wait_irq(fpc1080, FPC1080_CAPTURE_IMAGE, &irq);
	if (error)
		return error;
	return fpc1080_spi_rd_image(fpc1080, 0);
}

/* -------------------------------------------------------------------- */
static int fpc1080_spi_rd_image(struct fpc1080_data *fpc1080, bool capture)
{
		int error;
	u8 tmp[2] = {0};
	struct spi_message spi_mess;
	struct spi_transfer trans_rd_cap1;
	int rx_off=0;
	int tx_off=0;

	memset(&trans_rd_cap1, 0 , sizeof(struct spi_transfer));
	if (fpc1080->current_frame >= (FPC1080_MAX_FRAMES-1))
		return -ENOBUFS;

	rx_off = fpc1080->current_frame * FPC1080_FRAME_SIZE;
	if (fpc1080->current_frame > 1) {
		tx_off = 0;
	} else {
		tx_off = FPC1080_MAX_FRAMES * 1024 - 1026;
	}

	memset(fpc1080->huge_buffer + tx_off, 0, 1026);
	fpc1080->huge_buffer[tx_off] = capture ? FPC1080_READ_AND_CAPTURE_IMAGE :
			  FPC1080_READ_IMAGE_DATA;

	if (fpc1080->current_frame > 0) {
		tmp[0] = fpc1080->huge_buffer[rx_off];
		tmp[1] = fpc1080->huge_buffer[rx_off + 1];
	}

	trans_rd_cap1.cs_change = 0;
	trans_rd_cap1.delay_usecs = 0;
/*	trans_rd_cap1.speed_hz = FPC1080_SPI_CLOCK_SPEED;*/
	trans_rd_cap1.tx_buf = fpc1080->huge_buffer + tx_off;
	trans_rd_cap1.rx_buf = fpc1080->huge_buffer + rx_off;
	trans_rd_cap1.len = 1026;

	spi_message_init(&spi_mess);
	spi_message_add_tail(&trans_rd_cap1, &spi_mess);

	error = spi_sync(fpc1080->spi, &spi_mess);

	if (fpc1080->current_frame > 0) {
		fpc1080->huge_buffer[rx_off] = tmp[0];
		fpc1080->huge_buffer[rx_off + 1] = tmp[1];
	}

	if (error)
		return error;

	fpc1080->current_frame++;
	fpc1080->avail_data += FPC1080_FRAME_SIZE;
	wake_up_interruptible(&fpc1080->waiting_data_avail);

	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_spi_read_wait_irq(struct fpc1080_data *fpc1080, u8 *irq)
{
	int error;
	u8 buf[4];
	struct spi_message m;

	struct spi_transfer t = {
		.cs_change = 0,
		.delay_usecs = 0,
/*		.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
		.tx_buf = buf,
		.rx_buf = buf + 2,
		.len = 2,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 0,
	};
	buf[0] = FPC1080_RD_INTERRUPT_WITH_CLEAR;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	while (1) {
		error = fpc1080_wait_for_irq(fpc1080,
					     FPC1080_DEFAULT_IRQ_TIMEOUT);
		if (error == 0)
			break;
		if (fpc1080->thread_task.should_stop)
			return -EINTR;
		if (error != -ETIMEDOUT)
			return error;
	}
	if (error)
		return error;

	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}

	*irq = buf[3];
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_cmd_wait_irq(struct fpc1080_data *fpc1080, u8 cmd, u8 *irq)
{
	int error;
	struct spi_message m;
	struct spi_transfer t = {
		.cs_change = 0,
		.delay_usecs = 0,
/*		.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
		.tx_buf = &cmd,
		.rx_buf = NULL,
		.len = 1,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 0,
	};
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}

	return fpc1080_spi_read_wait_irq(fpc1080, irq);
}

/* -------------------------------------------------------------------- */
static int fpc1080_spi_is_motion(struct fpc1080_data *fpc1080)
{
	int error;
	u8 tx[7];
	u8 rx[7];

	struct spi_message m;

	struct spi_transfer t = {
		.cs_change = 0,
		.delay_usecs = 0,
/*		.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
		.tx_buf = tx,
		.rx_buf = rx,
		.len = 7,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 0,
	};
	tx[0] = FPC1080_SMRT_DATA;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);


	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}
	return ((rx[2 + FPC1080_SMRT_X_BYTE] > 0) ||
		(rx[2 + FPC1080_SMRT_Y_BYTE] >= 3));
}

/* -------------------------------------------------------------------- */
static int fpc1080_wreg(struct fpc1080_data *fpc1080, u8 addr, u8 value)
{
	int error;
	u8 tx[2];

	struct spi_message m;

	struct spi_transfer t = {
		.cs_change = 0,
		.delay_usecs = 0,
/*		.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
		.tx_buf = tx,
		.rx_buf = NULL,
		.len = 2,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 0,
	};
	tx[0] = addr;
	tx[1] = value;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	error = spi_sync(fpc1080->spi, &m);

	if (error)
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");

	dev_dbg(&fpc1080->spi->dev, "wrote %X to register %X\n", value, addr);
	return error;
}

/* -------------------------------------------------------------------- */
static int fpc1080_thread_goto_idle(struct fpc1080_data *fpc1080)
{
	fpc1080->thread_task.should_stop = 1;
	fpc1080->thread_task.mode = FPC1080_THREAD_IDLE_MODE;
	printk("%s: mode=%d\n", __func__, fpc1080->thread_task.mode);
	if (down_interruptible(&fpc1080->thread_task.sem_idle))
		return -ERESTARTSYS;

	up(&fpc1080->thread_task.sem_idle);

	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_start_thread(struct fpc1080_data *fpc1080, int mode)
{
	 fpc1080->thread_task.should_stop = 0;
	 fpc1080->thread_task.mode = mode;
	 printk("[fp] %s: mode=%d\n", __func__, fpc1080->thread_task.mode);
	 wake_up_interruptible(&fpc1080->thread_task.wait_job);

	 return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_write_adc_setup(struct fpc1080_data *fpc1080)
{
	int error;
	printk("%s:\n", __func__);
	error = fpc1080_wreg(fpc1080, FPC1080_ADC_GAIN,
			     fpc1080->adc_setup.gain);
	if (error)
		return error;

	error = fpc1080_wreg(fpc1080, FPC1080_ADC_OFFSET,
			     fpc1080->adc_setup.offset);
	if (error)
		return error;

	return fpc1080_wreg(fpc1080, FPC1080_PXL_SETUP,
			    fpc1080->adc_setup.pxl_setup);
}

/* -------------------------------------------------------------------- */
static int fpc1080_capture_task(struct fpc1080_data *fpc1080)
{
	int error;
	int keep_image;

	u8 irq;
/*	u32 stored_captures = 0;*/
	u32 total_captures;

	struct timespec ts_start, ts_end, ts_delta;
	printk("%s: starting\n", __func__);
	error = fpc1080_write_adc_setup(fpc1080);
	if (error)
		goto out;

	error = fpc1080_wreg(fpc1080, FPC1080_FNGR_DRIVE_CONF,
			     (FPC1080_SPI_FNGR_DRV_EXT |
			      FPC1080_SPI_SMRT_SENS_EN));
	if (error)
		goto out;

	error = fpc1080_cmd_wait_irq(fpc1080,
				     FPC1080_WAIT_FOR_FINGER_PRESENT,
				     &irq);
	if (error)
		goto out;

	getnstimeofday(&ts_start);

	error = fpc1080_cmd_wait_irq(fpc1080, FPC1080_CAPTURE_IMAGE, &irq);
	if (error)
		goto out;

	keep_image = 1;
	total_captures = 0;

	while (1) {

		if (fpc1080->thread_task.should_stop) {
			error = -EINTR;
			break;
		}
		total_captures++;
		if (keep_image) {
			error = fpc1080_cmd_wait_irq(fpc1080,
						     FPC1080_SET_SMART_REF,
						     &irq);
			if (error)
				goto out;

			error = fpc1080_spi_rd_image(fpc1080, 1);
			if (error)
				break;

			fpc1080_spi_read_wait_irq(fpc1080, &irq);
		} else {
			error = fpc1080_cmd_wait_irq(fpc1080,
						     FPC1080_CAPTURE_IMAGE,
						     &irq);
			if (error)
				break;
		}

		if (irq & FPC1080_IRQ_FING_UP)
			break;

		error = fpc1080_spi_is_motion(fpc1080);
		if (error < 0)
			break;

		keep_image = error;
	}

	getnstimeofday(&ts_end);
	ts_delta = timespec_sub(ts_end, ts_start);

	fpc1080->diag.capture_time = ts_delta.tv_nsec / NSEC_PER_MSEC;
	fpc1080->diag.capture_time += (ts_delta.tv_sec * MSEC_PER_SEC);

	fpc1080->diag.frames_stored = fpc1080->current_frame;
	fpc1080->diag.frames_captured = total_captures;

	if (fpc1080->diag.capture_time > 0) {
		dev_dbg(&fpc1080->spi->dev,
			"captured %lu frames (%lu kept) in %lu  ms (%lu fps)\n",
			(long unsigned int)fpc1080->diag.frames_captured,
			(long unsigned int)fpc1080->diag.frames_stored,
			(long unsigned int)fpc1080->diag.capture_time,
			(long unsigned int)(total_captures * MSEC_PER_SEC /
					    fpc1080->diag.capture_time));
	}

out:
	if (error) {
		fpc1080->avail_data = 0;
		dev_err(&fpc1080->spi->dev,
			"capture_task failed with error %i\n", error);
	}
	fpc1080->capture_done = 1;
	wake_up_interruptible(&fpc1080->waiting_data_avail);
	return error;
}

/* -------------------------------------------------------------------- */
static int threadfn(void *_fpc1080)
{
	struct fpc1080_data *fpc1080 = _fpc1080;
	printk("%s: starting\n", __func__);
	while (!kthread_should_stop()) {

		up(&fpc1080->thread_task.sem_idle);
		wait_event_interruptible(fpc1080->thread_task.wait_job,
			fpc1080->thread_task.mode != FPC1080_THREAD_IDLE_MODE);

		down(&fpc1080->thread_task.sem_idle);
	printk("%s: fpc1080->thread_task.mode=%d\n", __func__, fpc1080->thread_task.mode);
		switch (fpc1080->thread_task.mode) {
		case FPC1080_THREAD_CAPTURE_MODE:
			fpc1080_capture_task(fpc1080);
			break;

#ifdef CONFIG_FPC1080_NAVIGATION
		case FPC1080_THREAD_NAV_MODE:
			fpc1080_nav_task(fpc1080);
			break;
#endif
		default:
			break;
		}

		if(fpc1080->thread_task.mode != FPC1080_THREAD_EXIT) {
			fpc1080->thread_task.mode = FPC1080_THREAD_IDLE_MODE;
		}
	}
	printk("[fpc]%s: end\n", __func__);
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_abort_capture(struct fpc1080_data *fpc1080)
{
	printk("%s:\n", __func__);
	fpc1080_thread_goto_idle(fpc1080);
	fpc1080->avail_data = 0;
	fpc1080->current_frame = 0;
	fpc1080->data_offset = 0;
	return fpc1080_reset(fpc1080);
}

/* -------------------------------------------------------------------- */
static int fpc1080_start_capture(struct fpc1080_data *fpc1080)
{
	int error = 0;
	printk("%s:\n", __func__);

	error = fpc1080_abort_capture(fpc1080);

	if (error)
		return error;

	fpc1080_start_thread(fpc1080, FPC1080_THREAD_CAPTURE_MODE);
	fpc1080->capture_done = 0;

	return error;
}

/* -------------------------------------------------------------------- */
irqreturn_t fpc1080_interrupt(int irq, void *_fpc1080)
{
	struct fpc1080_data *fpc1080 = _fpc1080;
	if (gpio_get_value(fpc1080->irq_gpio)) {
		fpc1080->interrupt_done = 1;
		wake_up_interruptible(&fpc1080->waiting_interrupt_return);
		return IRQ_HANDLED;
	}
	printk("%s:IRQ_NONE\n", __func__);
	return IRQ_NONE;
}

/* -------------------------------------------------------------------- */
static int fpc1080_selftest_short(struct fpc1080_data *fpc1080)
{
	int error;
	printk("[fp]%s:\n", __func__);
	error = fpc1080_abort_capture(fpc1080);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"reset fail on entry.\n");
		goto err;
	}

	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_REG_HW_ID, 0, 2);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"read HW-ID fail.\n");
		goto err;
	}

	if (fpc1080->huge_buffer[fpc1080->data_offset] != FPC1080_HARDWARE_ID) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"HW-ID mismatch.\n");
		error = -EIO;
		goto err;
	}

	error= fpc1080_wreg(fpc1080, FPC1080_CAPTURE_IMAGE, 0);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"capture cmd fail.\n");
		goto err;
	}

	error = fpc1080_wait_for_irq(fpc1080, FPC1080_DEFAULT_IRQ_TIMEOUT);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"irq timeout.\n");
		goto err;
	}

	error = fpc1080_abort_capture(fpc1080);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 selftest, "
						"reset fail on exit.\n");
		goto err;
	}

err:
	fpc1080->diag.selftest = (error == 0)? 1 : 0;
	printk("[fp] %s:fpc1080->diag.selftest=%d\n", __func__, fpc1080->diag.selftest);
#ifdef CONFIG_FPC1080_NAVIGATION
	fpc1080_start_navigation(fpc1080);
#endif

	return error;
}

static int fpc1080_get_id_short(struct fpc1080_data *fpc1080)
{
	int error;
	printk("[fp] %s:\n", __func__);
	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_RD_INTERRUPT_WITH_NO_CLEAR, 0, 2);

	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_REG_HW_ID, 0, 2);
	if (error) {
		dev_err(&fpc1080->spi->dev, "fpc1080 get id, "
						"read HW-ID fail.\n");
		goto err;
	}

	if (fpc1080->huge_buffer[fpc1080->data_offset] != FPC1080_HARDWARE_ID) {
		dev_err(&fpc1080->spi->dev, "fpc1080 get id, "
						"HW-ID mismatch.\n");
		error = -EIO;
		goto err;
	}
	printk("%s:HW-ID = 0x%x\n", __func__, fpc1080->huge_buffer[fpc1080->data_offset]);
err:
	fpc1080->diag.get_id = (error == 0)? 1 : 0;
	printk("[fp] %s:fpc1080->diag.get_id=%d\n", __func__, fpc1080->diag.get_id);
#ifdef CONFIG_FPC1080_NAVIGATION
	fpc1080_start_navigation(fpc1080);
#endif
	return error;
}
#if  defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)

#ifdef IMAGE_NAVIGATION

#define IMAGE_WIDTH 128
#define IMAGE_ROW 8
#define IMAGE_PADDING 24
#define BEST_IMAGE_WIDTH  4
#define BEST_IMAGE_HEIGHT  6

#define EFINGER_UP 50000

/* -------------------------------------------------------------------- */

static void process_navi_event( struct fpc1080_data *fpc1080, int dx, int dy, int finger_status )
{
	 const int THRESHOLD_RANGE_TAP = 100;
	 const unsigned long THRESHOLD_DURATION_TAP = 350;

	 int isMouseMode = 0;
	 int filtered_finger_status = finger_status;

	 static int deviation_x = 0;
	 static int deviation_y = 0;
	 int deviation;
	 static unsigned long tick_down = 0;
	 unsigned long tick_curr = jiffies * 1000 / HZ;
	 unsigned long duration = 0;
	 const unsigned long THRESHOLD_DURATION_HOLD = 900;
printk("%s:\n", __func__);
#ifdef CONFIG_INPUT_CRUCIAL_MISC
	 if ( finger_status == FNGR_ST_LOST )
	  set_crucial_otp_info( 0, 0, 1000, 1000 );
	 else if ( dx || dy )
	  set_crucial_otp_info( -dx, -dy, 0, 0 );
#endif

	 if ( finger_status == FNGR_ST_DETECTED )
	 {
	  tick_down = tick_curr;
	 }

	 if ( tick_down > 0 )
	 {
	  duration = tick_curr - tick_down;
	  deviation_x += dx;
	  deviation_y += dy;
	  deviation =  deviation_x * deviation_x + deviation_y * deviation_y;

	  if ( deviation > THRESHOLD_RANGE_TAP )
	  {
		   deviation_x = 0;
		   deviation_y = 0;
		   tick_down = 0;
		   fpc1080->tap_status = -1;

	  }
#ifdef SUPPORT_MOUSE_DRAGGING
	  else
	  {
		   if ( duration < THRESHOLD_DURATION_TAP )
		   {
			if ( finger_status == FNGR_ST_LOST )
			{

#if defined (SUPPORT_DOUBLE_TAP)
			if(fpc1080->tap_status == FNGR_ST_TAP)
				 fpc1080->tap_status = FNGR_ST_DOUBLE_TAP;
			else
				 fpc1080->tap_status = FNGR_ST_TAP;

			return;
#else
			 filtered_finger_status = FNGR_ST_TAP;
#endif


			 tick_down = 0;
			 deviation_x = 0;
			 deviation_y = 0;
			}
		   }

		   else
		   {
			filtered_finger_status = FNGR_ST_L_HOLD;
			fpc1080->tap_status = -1;
			tick_down = 0;
			deviation_x = 0;
			deviation_y = 0;
		   }
	  }
#else
	  else if ( finger_status == FNGR_ST_LOST && duration < THRESHOLD_DURATION_TAP )
	  {
		   filtered_finger_status = FNGR_ST_TAP;
		   tick_down = 0;
		   deviation_x = 0;
		   deviation_y = 0;
	  }
	  else if ( duration > THRESHOLD_DURATION_HOLD )
	  {
		   filtered_finger_status = FNGR_ST_HOLD;
		   tick_down = 0;
		   deviation_x = 0;
		   deviation_y = 0;
	  }

#endif
	 }



#if defined(CONFIG_FPC1080_MOUSE)
	 isMouseMode = fpc1080->nav_settings.mouse_enable;


#endif

#if defined (SUPPORT_VOLUME_CONTROL)
	 if(fpc1080->volume_enable == true){
	  dispatch_dpad_event( fpc1080, dx, dy, filtered_finger_status );
	  return;
	 }
#endif



#if defined CONFIG_FPC1080_TOUCH_PAD
		  dispatch_touch_pad_event( fpc1080, dx, dy, filtered_finger_status );
#else
		  if (isMouseMode == 0)
		  dispatch_trackpad_event( fpc1080, dx, dy, filtered_finger_status );
		 else
		  dispatch_mouse_event( fpc1080, dx, dy, filtered_finger_status );
#endif


}

/* -------------------------------------------------------------------- */

static int getSliceDiff(unsigned char* a, unsigned char* b, unsigned int columns, unsigned int rows) {
	int diff = 0;
	int x;
	int y;
	int i;

	for (y = 0; y < rows; y++) {
		for (x = 0; x < columns; x++) {
			i = x + y * IMAGE_WIDTH;
			diff += abs(a[i] - b[i]);
		}
	}
	return diff;
}

/* -------------------------------------------------------------------- */

static void get_movement(unsigned char* prev, unsigned char* cur, int* mx, int* my){
	int compare_width = IMAGE_WIDTH - 2 * IMAGE_PADDING;
	int diff = 0;
	int min_diff = 255 * 40;
	int x;
	int y;
	int cr1 = 0;
	int cr2 = 0;
	const int THRESHOLD_CR = 1400;
	*mx = 0;
	*my = 0;
	for(y = 0 ; y < BEST_IMAGE_HEIGHT ; y++){
		for(x = -BEST_IMAGE_WIDTH ; x <= BEST_IMAGE_WIDTH; x++){
			diff = getSliceDiff(prev + IMAGE_PADDING, cur + IMAGE_PADDING + x + y*IMAGE_WIDTH, compare_width, IMAGE_ROW - BEST_IMAGE_HEIGHT);
			if(diff < min_diff){
				min_diff = diff;
				*mx = x;
				*my = y;
			}
			if ( x == 0 && y == 0 )
			     cr1 = diff;

			diff = getSliceDiff(cur + IMAGE_PADDING, prev + IMAGE_PADDING + x + y*IMAGE_WIDTH, compare_width, IMAGE_ROW - BEST_IMAGE_HEIGHT);
			if(diff < min_diff){
				min_diff = diff;
				*mx = -x;
				*my = -y;
			}
			if ( x == 0 && y == 0 )
			     cr2 = diff;
		}
	}

	if ( *mx || *my )
	{
	     if ( max(cr1, cr2) * 1000 / min_diff < THRESHOLD_CR )
	     {
		  *mx = 0;
		  *my = 0;
	     }
	}
}

/* -------------------------------------------------------------------- */

static int fpc1080_wait_fp_event(struct fpc1080_data *fpc1080, u8 cmd, u8 *irq )
{
/*	unsigned long diffVolumeTime = 0;
	unsigned long diffTime = 0;*/

	int error;
	struct spi_message m;
	u8 buf[4];

	struct spi_transfer t;
printk("%s:+++\n", __func__);
	memset(&t, 0 , sizeof(struct spi_transfer));

	t.cs_change = 0,
	t.delay_usecs = 0,
/*	t.speed_hz = FPC1080_SPI_CLOCK_SPEED,*/
	t.tx_buf = &cmd,
	t.rx_buf = NULL,
	t.len = 1,
	t.tx_dma = 0,
	t.rx_dma = 0,
	t.bits_per_word = 0,

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}

	memset(&t, 0 , sizeof(struct spi_transfer));

	t.cs_change = 0;
	t.delay_usecs = 0;
/*	t.speed_hz = FPC1080_SPI_CLOCK_SPEED;*/
	t.tx_buf = buf;
	t.rx_buf = buf + 2;
	t.len = 2;
	t.tx_dma = 0;
	t.rx_dma = 0;
	t.bits_per_word = 0;

	buf[0] = FPC1080_RD_INTERRUPT_WITH_CLEAR;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	while (1) {
		error = fpc1080_wait_for_irq(fpc1080, FPC1080_DEFAULT_IRQ_TIMEOUT);
		if (error == 0)
			break;
		if (fpc1080->thread_task.should_stop)
			return -EINTR;
		if (error != -ETIMEDOUT)
			return error;


#ifdef SUPPORT_VOLUME_CONTROL
		if(fpc1080->volume_enable == true){
			diffVolumeTime = abs(jiffies - fpc1080->volume_time);
			if(diffVolumeTime > 0){
				diffVolumeTime = diffVolumeTime * 1000000 / HZ;
				if (diffVolumeTime >= FPC1080_POLL_INTERVAL_VOLUME) {
					fpc1080->volume_enable = false;
				}
			}
		}
#endif

#ifdef SUPPORT_DOUBLE_TAP
		if(fpc1080->tap_status == FNGR_ST_TAP || fpc1080->tap_status == FNGR_ST_DOUBLE_TAP){
			diffTime = abs(jiffies - fpc1080->time);
			if(diffTime > 0){
				diffTime = diffTime * 1000000 / HZ;
				if (diffTime >= FPC1080_POLL_INTERVAL_TAP) {
					if(fpc1080->tap_status == FNGR_ST_TAP){
						process_navi_event(fpc1080, 0, 0, FNGR_ST_TAP);
					}
					else{
						process_navi_event(fpc1080, 0, 0, FNGR_ST_DOUBLE_TAP);
					}
					fpc1080->tap_status = -1;
				}
			}
		}
#endif

	}
	if (error)
		return error;

	error = spi_sync(fpc1080->spi, &m);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_sync failed.\n");
		return error;
	}

	*irq = buf[3];
	return 0;
}

/* -------------------------------------------------------------------- */

static int fpc1080_nav_task(struct fpc1080_data *fpc1080){
	bool isReverse = false;
	int dx = 0;
	int dy = 0;
	int sumX = 0;
	int sumY = 0;
/*	unsigned long diffVolumeTime = 0;*/
	unsigned long diffTime = 0;
	int status = 0;
	int keep_image;
	u8 irq;
	unsigned char* prevBuffer;
	unsigned char* curBuffer;

	status = fpc1080_write_adc_setup(fpc1080);
	if (status)
		goto out;

	status = fpc1080_wreg(fpc1080, FPC1080_FNGR_DRIVE_CONF,
				     FPC1080_SPI_FNGR_DRV_EXT | FPC1080_SPI_SMRT_SENS_EN);
	if (status)
		goto out;

	while (!kthread_should_stop()) {
restart:
		status = fpc1080_wait_fp_event(fpc1080, FPC1080_WAIT_FOR_FINGER_PRESENT, &irq);
		if (status){
			goto out;
		}

		status = fpc1080_cmd_wait_irq(fpc1080, FPC1080_CAPTURE_IMAGE, &irq);
		if (status)
			goto out;

		keep_image = 1;
		fpc1080->time = jiffies;
		process_navi_event(fpc1080, 0, 0, FNGR_ST_DETECTED);

		while (1) {

			if (fpc1080->thread_task.should_stop) {
				status = -EINTR;
				break;
			}

			if(isReverse){
				prevBuffer = fpc1080->cur_img_buf;
				curBuffer = fpc1080->prev_img_buf;
			}
			else{
				prevBuffer = fpc1080->prev_img_buf;
				curBuffer = fpc1080->cur_img_buf;
			}

			if (keep_image) {

				status = fpc1080_cmd_wait_irq(fpc1080, FPC1080_SET_SMART_REF, &irq);

				if (status){
					goto out;
				}

				status = fpc1080_spi_rd_image(fpc1080, 1);
				if (status){
					goto out;
				}

				fpc1080_spi_read_wait_irq(fpc1080, &irq);
			} else {
				status = fpc1080_cmd_wait_irq(fpc1080, FPC1080_CAPTURE_IMAGE, &irq);

				if (status){
					goto out;
				}
			}

#if defined (SUPPORT_VOLUME_CONTROL)
			if(fpc1080->volume_enable == true){
				diffVolumeTime = abs(jiffies - fpc1080->volume_time);
				if(diffVolumeTime > 0){
					diffVolumeTime = diffVolumeTime * 1000000 / HZ;
					if (diffVolumeTime >= FPC1080_POLL_INTERVAL_VOLUME) {
						fpc1080->volume_enable = false;
					}
				}
			}
#endif

			if (irq & FPC1080_IRQ_FING_UP){
				process_navi_event(fpc1080, 0, 0, FNGR_ST_LOST);
				goto restart;
			}
			status = fpc1080_spi_is_motion(fpc1080);
			if (status < 0){
				goto out;
			}

			if (fpc1080->avail_data ) {
				 memcpy(curBuffer,
					&fpc1080->huge_buffer[fpc1080->data_offset], FPC1080_FRAME_SIZE);

				fpc1080->data_offset += FPC1080_FRAME_SIZE;
				fpc1080->avail_data -= FPC1080_FRAME_SIZE;
				if(fpc1080->data_offset >= FPC1080_IMAGE_BUFFER_SIZE)
					fpc1080->data_offset = 0;

				get_movement(prevBuffer, curBuffer, &dx, &dy);
				isReverse = !isReverse;
				sumX += dx;
				sumY += dy;

				diffTime = abs(jiffies - fpc1080->time);
				if(diffTime > 0){
					diffTime = diffTime * 1000000 / HZ;
					if (diffTime >= FPC1080_POLL_INTERVAL) {
						//printk(KERN_INFO "nav_thread() send dx : %d dy : %d\n", sumX, sumY);
						process_navi_event(fpc1080, sumX, sumY, FNGR_ST_MOVING);
						sumX = 0;
						sumY = 0;
						fpc1080->time = jiffies;
					}
				}
			}

			keep_image = status;

		}
	}
	goto done;
out:
	if (status) {
		fpc1080->avail_data = 0;
		if(status == -EINTR)
			dev_dbg(&fpc1080->spi->dev, "capture_task cancel\n");
		else
			dev_err(&fpc1080->spi->dev, "capture_task failed with error %i\n", status);
	}
	fpc1080->capture_done = 1;
	wake_up_interruptible(&fpc1080->waiting_data_avail);
	return status;
done:
	return 0;
}
#else
/* -------------------------------------------------------------------- */

static int fpc1080_write_nav_setup(struct fpc1080_data *fpc1080)
{
	printk("%s:\n", __func__);
	if (fpc1080_wreg(fpc1080, FPC1080_FNGR_DWN_MIN, 50))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_FNGR_DWN_MID, 120))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_FNGR_DWN_MAX, 255))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_FNGR_DET_THRESHOLD, 15))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_FNGR_LOST_THRESHOLD, 10))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_DX_THRESHOLD, 25))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_DY_THRESHOLD, 25))
		goto err;
	if (fpc1080_wreg(fpc1080, FPC1080_NAV_CNTR, 20))
		goto err;

	return 0;
err:
	return -EIO;
}

/* -------------------------------------------------------------------- */

static int fpc1080_nav_thread_wait_fp_event(struct fpc1080_data *fpc1080)
{
	int status;
	status = fpc1080_spi_wr_reg(fpc1080,
				    FPC1080_WAIT_FOR_FINGER_PRESENT, 0, 1);

	if (status)
		return status;

	while (1) {
		status = fpc1080_wait_for_irq(fpc1080,
					FPC1080_DEFAULT_IRQ_TIMEOUT);
		if (status == 0)
			break;
		if (fpc1080->thread_task.should_stop)
			return 0;
		if (status != -ETIMEDOUT)
			return status;
	}

	status = fpc1080_spi_wr_reg(fpc1080,
			FPC1080_RD_INTERRUPT_WITH_CLEAR, 0, 2);
	if (status)
		return status;

	input_report_key(fpc1080->nav_dev, FPC1080_KEY_FINGER_PRESENT, 1);
	input_report_key(fpc1080->nav_dev, FPC1080_KEY_FINGER_PRESENT, 0);
	input_sync(fpc1080->nav_dev);
	printk(KERN_INFO "[fp]fpc1080: event synced\n");
	return 1;
}

/* -------------------------------------------------------------------- */

static int fpc1080_nav_task(struct fpc1080_data *fpc1080)
{
	u8 irq;
	int status;
	u8 *data;
	u8 dx;
	u8 dy;
	u8 key;
	int x, y;
	printk("[fp] %s:\n", __func__);
	if (fpc1080_spi_wr_reg(fpc1080, FPC1080_FNGR_DRIVE_CONF, 2, 2))
		goto err;

	if (fpc1080_write_adc_setup(fpc1080))
		goto err;

	if (fpc1080_write_nav_setup(fpc1080))
		goto err;

	status = fpc1080_nav_thread_wait_fp_event(fpc1080);

	if (status < 0)
		goto err;
	else if (status == 0)
		goto done;

	if (fpc1080_spi_wr_reg(fpc1080, FPC1080_ACTIVATE_NAVIGATION, 0, 1))
		goto err;

	while (!fpc1080->thread_task.should_stop) {
		while (1) {
			status = fpc1080_wait_for_irq(fpc1080,
						FPC1080_DEFAULT_IRQ_TIMEOUT);
			if (status == 0)
				break;
			if (fpc1080->thread_task.should_stop)
				goto done;
			if (status != -ETIMEDOUT)
				goto err;
		}
		if (fpc1080_spi_wr_reg(fpc1080,
				FPC1080_RD_INTERRUPT_WITH_NO_CLEAR, 0, 2))
			goto err;
		irq = fpc1080->huge_buffer[fpc1080->data_offset];

		if (irq & FPC1080_IRQ_FING_LOST) {
			if (fpc1080_spi_wr_reg(fpc1080,
					FPC1080_RD_INTERRUPT_WITH_CLEAR, 0, 2))
					goto err;

			status = fpc1080_nav_thread_wait_fp_event(fpc1080);
			if (status < 0)
				goto err;
			else if (status == 0)
				goto done;

			if (fpc1080_spi_wr_reg(fpc1080,
				FPC1080_ACTIVATE_NAVIGATION, 0, 1))
					goto err;
		} else {
			if (fpc1080_spi_wr_reg(fpc1080, FPC1080_SMRT_DATA,
									0, 7))
				goto err;
			data = fpc1080->huge_buffer + fpc1080->data_offset;
			dx = data[FPC1080_SMRT_X_BYTE];
			dy = data[FPC1080_SMRT_Y_BYTE];
			key = 0;

			switch (irq) {
			case FPC1080_IRQ_DX:
				key = (data[FPC1080_SMRT_BITS] &
				FPC1080_SMRT_X_SIGN ? KEY_LEFT : KEY_RIGHT);
			break;
			case FPC1080_IRQ_DY:
				key = (data[FPC1080_SMRT_BITS] &
				FPC1080_SMRT_Y_SIGN ? KEY_UP : KEY_DOWN);
			break;
			case FPC1080_IRQ_SHORT_CLICK:
			case FPC1080_IRQ_LONG_CLICK:
				if (dx + dy < FPC1080_MOTION_THRESHOLD &&
					data[FPC1080_SMRT_MO_CNTR_BYTE]  <
					FPC1080_MOTON_FRAMES_THERSHOLD) {
					key = KEY_ENTER;
				}
			default:
			break;
			}

			if (key) {
				if ( KEY_ENTER == key ) {
					x = 0;
					y = 0;
				} else if ( KEY_UP == key ) {
					x = 0;
					y = (dy/20 + 1);
				} else if ( KEY_DOWN == key ) {
					x = 0;
					y = -(dy/20 + 1);
				} else if ( KEY_RIGHT == key ) {
					x = -(dx/20 + 1);
					y = 0;
				} else if ( KEY_LEFT == key ) {
					x = (dx/20 + 1);
					y = 0;
				} else {
					x = 0;
					y = 0;
				}

				if ( (FPC1080_KEY_FINGER_PRESENT == key) || (KEY_ENTER == key) ) {
					input_report_key(fpc1080->nav_dev, key, 1);
					input_report_key(fpc1080->nav_dev, key, 0);
					input_sync(fpc1080->nav_dev);
					/*
					if ( KEY_ENTER == key ) {
						input_report_key(fpc1080->mouse_dev, BTN_MIDDLE, 1);
						input_sync(fpc1080->mouse_dev );
						input_report_key(fpc1080->mouse_dev, BTN_MIDDLE, 0);
						input_sync(fpc1080->mouse_dev );
					} else if ( FPC1080_KEY_FINGER_PRESENT == key ) {
						input_report_key(fpc1080->nav_dev, key, 1);
						input_sync(fpc1080->nav_dev);
						input_report_key(fpc1080->nav_dev, key, 0);
						input_sync(fpc1080->nav_dev);
					}
					*/
				} else {
					printk(KERN_INFO "[fp] fpc1080_nav_task() x = %d, y = %d\n", x, y);
					input_report_rel(fpc1080->mouse_dev, REL_HWHEEL, x);
					input_report_rel(fpc1080->mouse_dev, REL_WHEEL, y);
					input_sync(fpc1080->mouse_dev );
				}
			}

			if (fpc1080_spi_wr_reg(fpc1080,
					FPC1080_RD_INTERRUPT_WITH_CLEAR, 0, 2))
				goto err;
			if (fpc1080_spi_wr_reg(fpc1080, FPC1080_SMRT_DATA,
									0, 7))
				goto err;
		}

	}
	goto done;
err:
	dev_err(&fpc1080->spi->dev, "fpc1080nav task stopped, "
					"due to previous error.\n");

	while (!fpc1080->thread_task.should_stop)
		; /* Note: semicolon goes here to satisfy style checker */

done:
	return 0;
}
#endif
/* -------------------------------------------------------------------- */

static int fpc1080_stop_navigation(struct fpc1080_data *fpc1080)
{
	printk("[fp] %s:\n", __func__);
	fpc1080_thread_goto_idle(fpc1080);
	return fpc1080_reset(fpc1080);
}

/* -------------------------------------------------------------------- */

static int fpc1080_start_navigation(struct fpc1080_data *fpc1080)
{
	printk("[fp] %s:\n", __func__);
	fpc1080_stop_navigation(fpc1080);
	fpc1080_start_thread(fpc1080, FPC1080_THREAD_NAV_MODE);
	return 0;
}

#endif



/* -------------------------------------------------------------------- */
static int fpc1080_open(struct inode *inode, struct file *file)
{
	struct fpc1080_data *fpc1080;
	int error;
	fpc1080 = container_of(inode->i_cdev, struct fpc1080_data, cdev);
	printk("[fp] %s:\n", __func__);
	if (down_interruptible(&fpc1080->mutex))
		return -ERESTARTSYS;

	file->private_data = fpc1080;

#ifdef CONFIG_FPC1080_NAVIGATION
	error = fpc1080_stop_navigation(fpc1080);
#else
	error = fpc1080_reset(fpc1080);
#endif
	up(&fpc1080->mutex);
	return error;
}

/* -------------------------------------------------------------------- */
static int fpc1080_release(struct inode *inode, struct file *file)
{
	int status;
	struct fpc1080_data *fpc1080 = file->private_data;
	status = 0;
	if (down_interruptible(&fpc1080->mutex))
		return -ERESTARTSYS;
	printk("[fp] %s:\n", __func__);
	fpc1080_abort_capture(fpc1080);

	if (!atomic_read(&file->f_count)) {
#ifdef CONFIG_FPC1080_NAVIGATION
		status = fpc1080_start_navigation(fpc1080);
#else
		status = fpc1080_spi_wr_reg(fpc1080,
				FPC1080_ACTIVATE_DEEP_SLEEP, 0, 1);
#endif
	}
	up(&fpc1080->mutex);
	return status;
}

/* -------------------------------------------------------------------- */
static ssize_t fpc1080_read(struct file *file, char *buff,
				size_t count, loff_t *ppos)
{
	int error;
	unsigned int max_dat;

	struct fpc1080_data *fpc1080 = file->private_data;
	error = 0;
	if (down_interruptible(&fpc1080->mutex))
		return -ERESTARTSYS;

	if (!fpc1080->capture_done) {
		error = wait_event_interruptible(
				fpc1080->waiting_data_avail,
				(fpc1080->capture_done || fpc1080->avail_data));
	}

	max_dat = (count > fpc1080->avail_data) ? fpc1080->avail_data : count;
	if (max_dat) {
		error = copy_to_user(buff,
			&fpc1080->huge_buffer[fpc1080->data_offset+2], max_dat);
		if (error)
			goto out;

		fpc1080->data_offset += max_dat;
		fpc1080->avail_data -= max_dat;
		if(fpc1080->data_offset >= FPC1080_IMAGE_BUFFER_SIZE)
			fpc1080->data_offset = 0;
		error = max_dat;
	}
out:
	up(&fpc1080->mutex);
	return error;
}

/* -------------------------------------------------------------------- */
static ssize_t fpc1080_write(struct file *file, const char *buff,
					size_t count, loff_t *ppos)
{
	return -ENOTTY;
}

/* -------------------------------------------------------------------- */
static unsigned int fpc1080_poll(struct file *file, poll_table *wait)
{
	unsigned int ret = 0;
	struct fpc1080_data *fpc1080 = file->private_data;
	if (down_interruptible(&fpc1080->mutex))
		return -ERESTARTSYS;

	if (fpc1080->avail_data == 0 && !fpc1080->capture_done)
		poll_wait(file, &fpc1080->waiting_data_avail, wait);

	if (fpc1080->avail_data > 0)
		ret |= 	(POLLIN | POLLRDNORM);
	else if (fpc1080->capture_done)
		ret |= POLLHUP;

	up(&fpc1080->mutex);

	return ret;
}

/* -------------------------------------------------------------------- */
static long fpc1080_ioctl(struct file *filp, unsigned int cmd,
					unsigned long arg) {
	int error;
	struct fpc1080_data *fpc1080 = filp->private_data;
	error = 0;
	if (down_interruptible(&fpc1080->mutex))
		return -ERESTARTSYS;
	printk("[fp] %s: cmd = %d\n", __func__, cmd);
	switch (cmd) {
	case FPC1080_IOCTL_START_CAPTURE:
		error = fpc1080_start_capture(fpc1080);
		break;
	case FPC1080_IOCTL_ABORT_CAPTURE:
		error = fpc1080_abort_capture(fpc1080);
		break;
	case FPC1080_IOCTL_CAPTURE_SINGLE:
		error = fpc1080_abort_capture(fpc1080);
		if (error)
			break;

		error = fpc1080_capture_single(fpc1080, arg);
		break;
	default:
		error = -ENOTTY;
		break;
	}
	up(&fpc1080->mutex);
	return error;
}

/* -------------------------------------------------------------------- */
static int fpc1080_cleanup(struct fpc1080_data *fpc1080)
{

#ifdef CONFIG_FPC1080_NAVIGATION
	if (fpc1080->nav_dev)
		input_unregister_device(fpc1080->nav_dev);
#endif


#ifdef CONFIG_FPC1080_TOUCH_PAD
					if (fpc1080->touch_pad_dev)
									input_unregister_device(fpc1080->touch_pad_dev);
#endif


	if (fpc1080->thread_task.thread) {
		fpc1080->thread_task.should_stop = 1;
		fpc1080->thread_task.mode = FPC1080_THREAD_EXIT;
		wake_up_interruptible(&fpc1080->thread_task.wait_job);
		kthread_stop(fpc1080->thread_task.thread);
	}

	if (!IS_ERR_OR_NULL(fpc1080->device))
		device_destroy(fpc1080->class, fpc1080->devno);

	class_destroy(fpc1080->class);

	if (fpc1080->irq >= 0)
		free_irq(fpc1080->irq, fpc1080);

	if (gpio_is_valid(fpc1080->irq_gpio))
		gpio_free(fpc1080->irq_gpio);

	if (gpio_is_valid(fpc1080->reset_gpio))
		gpio_free(fpc1080->reset_gpio);

	if (fpc1080->huge_buffer) {
		free_pages((unsigned long)fpc1080->huge_buffer,
			   get_order(FPC1080_IMAGE_BUFFER_SIZE));
	}
#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
	kfree(fpc1080->prev_img_buf);
	kfree(fpc1080->cur_img_buf);
#endif
	kfree(fpc1080);
	return 0;
}

/* -------------------------------------------------------------------- */
static int __devinit fpc1080_probe(struct spi_device *spi)
{
	struct fpc1080_platform_data *fpc1080_pdata;
	int error;
	struct fpc1080_data *fpc1080 = NULL;

	error = 0;

	printk("[fp] %s enter\n", __func__);

	fpc1080 = kzalloc(sizeof(*fpc1080), GFP_KERNEL);
	if (!fpc1080) {
		dev_err(&spi->dev,
		"failed to allocate memory for struct fpc1080_data\n");
		return -ENOMEM;
	}

	fpc1080->huge_buffer = (u8 *)__get_free_pages(GFP_KERNEL, get_order(FPC1080_IMAGE_BUFFER_SIZE));

	if (!fpc1080->huge_buffer) {
		dev_err(&fpc1080->spi->dev, "failed to get free pages\n");
		return -ENOMEM;
	}
#ifdef IMAGE_NAVIGATION
	fpc1080->prev_img_buf = kzalloc(IMAGE_WIDTH*IMAGE_ROW * sizeof(u8), GFP_KERNEL);

	if (!fpc1080->prev_img_buf) {
		dev_err(&fpc1080->spi->dev, "failed allocating image buffer memory.\n");
		error = -ENOMEM;
		goto err;
	}

	fpc1080->cur_img_buf = kzalloc(IMAGE_WIDTH*IMAGE_ROW * sizeof(u8), GFP_KERNEL);

	if (!fpc1080->cur_img_buf) {
		dev_err(&fpc1080->spi->dev, "failed allocating image buffer memory.\n");
		error = -ENOMEM;
		goto err;
	}
#endif

	spi_set_drvdata(spi, fpc1080);
	fpc1080->spi = spi;
	fpc1080->reset_gpio = -EINVAL;
	fpc1080->irq_gpio = -EINVAL;
	fpc1080->irq = -EINVAL;

	init_waitqueue_head(&fpc1080->waiting_interrupt_return);
	init_waitqueue_head(&fpc1080->waiting_data_avail);
	fpc1080_pdata = spi->dev.platform_data;

	memset(&(fpc1080->diag), 0, sizeof(fpc1080->diag));

	if (!fpc1080_pdata) {
		dev_err(&fpc1080->spi->dev,
				"spi->dev.platform_data is NULL.\n");
		error = -EINVAL;
		goto err;
	}

	fpc1080->reset_gpio = fpc1080_pdata->reset_gpio;
	error = ioext_gpio_set_value(fpc1080->reset_gpio, 1);
	if (error < 0) {
		pr_err("[fpc] %s: io_extender high failed %d\n", __func__, error);
	}

	fpc1080->irq_gpio = fpc1080_pdata->irq_gpio;
	fpc1080->irq = gpio_to_irq(fpc1080->irq_gpio);

	if (fpc1080->irq < 0) {
		dev_err(&fpc1080->spi->dev, "gpio_to_irq failed.\n");
		error = fpc1080->irq;
		goto err;
	}

	error = request_irq(fpc1080->irq, fpc1080_interrupt,
			IRQF_TRIGGER_RISING, "fpc1080", fpc1080);

	if (error) {
		dev_err(&fpc1080->spi->dev, "request_irq %i failed.\n",
			fpc1080->irq);

		fpc1080->irq = -EINVAL;
		goto err;
	}

/*
	if (fpc1080->irq)
		irq_set_irq_wake(fpc1080->irq, 1);
*/
	fpc1080->spi->mode = SPI_MODE_0;
	fpc1080->spi->bits_per_word = 8;

	error = spi_setup(fpc1080->spi);

	if (error) {
		dev_err(&fpc1080->spi->dev, "spi_setup failed\n");
		goto err;
	}

	error = fpc1080_reset(fpc1080);

	if (error)
		goto err;

	printk("[fpc] %s: fpc1080->irq_gpio (%d) = %d.\n", __func__, fpc1080->irq_gpio , gpio_get_value(fpc1080->irq_gpio));
	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_REG_HW_ID, 0, 2);
	if (error)
		goto err;

	if (fpc1080->huge_buffer[fpc1080->data_offset] != FPC1080_HARDWARE_ID) {
		dev_err(&fpc1080->spi->dev,
			"hardware id mismatch: %x expected %x\n",
			fpc1080->huge_buffer[fpc1080->data_offset],
			FPC1080_HARDWARE_ID);

		error = -EIO;
		goto err;
	}

	dev_info(&fpc1080->spi->dev, "hardware id: %x\n",
		 fpc1080->huge_buffer[fpc1080->data_offset]);

	fpc1080->class = class_create(THIS_MODULE, FPC1080_CLASS_NAME);

	if (IS_ERR(fpc1080->class)) {
		dev_err(&fpc1080->spi->dev, "failed to create class.\n");
		error = PTR_ERR(fpc1080->class);
		goto err;
	}

	fpc1080->devno = MKDEV(FPC1080_MAJOR, fpc1080_device_count++);

	fpc1080->device = device_create(fpc1080->class, NULL, fpc1080->devno,
						NULL, "%s", FPC1080_DEV_NAME);

	if (IS_ERR(fpc1080->device)) {
		dev_err(&fpc1080->spi->dev, "device_create failed.\n");
		error = PTR_ERR(fpc1080->device);
		goto err;
	}

	fpc1080->adc_setup.gain = fpc1080_pdata->adc_setup.gain;
	fpc1080->adc_setup.offset = fpc1080_pdata->adc_setup.offset;
	fpc1080->adc_setup.pxl_setup = fpc1080_pdata->adc_setup.pxl_setup;

#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
	memcpy(&fpc1080->nav_settings, &fpc1080_pdata->nav, sizeof(fpc1080->nav_settings));
	fpc1080->nav_dev = input_allocate_device();

	if (!fpc1080->nav_dev) {
		dev_err(&fpc1080->spi->dev, "input_allocate_device failed.\n");
		error  = -ENOMEM;
		goto err;
	}

	fpc1080->nav_dev->name = FPC1080_DEV_NAME;
	set_bit(EV_KEY, fpc1080->nav_dev->evbit);
	set_bit(EV_REL, fpc1080->nav_dev->evbit);
	input_set_capability(fpc1080->nav_dev, EV_REL, REL_X);
	input_set_capability(fpc1080->nav_dev, EV_REL, REL_Y);
	input_set_capability(fpc1080->nav_dev, EV_KEY, BTN_MOUSE);
	input_set_capability(fpc1080->nav_dev, EV_KEY, KEY_ENTER);

#if defined (SUPPORT_DOUBLE_TAP)
	input_set_capability(fpc1080->nav_dev, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(fpc1080->nav_dev, EV_KEY, KEY_VOLUMEDOWN);
#endif



	input_set_capability(fpc1080->nav_dev, EV_KEY, KEY_POWER);
	input_set_capability(fpc1080->nav_dev, EV_KEY, KEY_SLEEP);
/* add for wake up */
	set_bit(EV_KEY, fpc1080->nav_dev->evbit);
	set_bit(KEY_ENTER, fpc1080->nav_dev->keybit);
	set_bit(KEY_UP, fpc1080->nav_dev->keybit);
	set_bit(KEY_DOWN, fpc1080->nav_dev->keybit);
	set_bit(KEY_RIGHT, fpc1080->nav_dev->keybit);
	set_bit(KEY_LEFT, fpc1080->nav_dev->keybit);
	set_bit(FPC1080_KEY_FINGER_PRESENT, fpc1080->nav_dev->keybit);

	error = input_register_device(fpc1080->nav_dev);

	if (error) {
		dev_err(&fpc1080->spi->dev, "input_register_device failed.\n");
		input_free_device(fpc1080->nav_dev);
		fpc1080->nav_dev = NULL;
		goto err;
	}

	fpc1080->mouse_dev = input_allocate_device();

	if (!fpc1080->mouse_dev) {
		dev_err(&fpc1080->spi->dev, "input_allocate_device failed.\n");
		error  = -ENOMEM;
		goto err;
	}

	fpc1080->mouse_dev->name = FPC1080_MOUSE_DEV_NAME;
	set_bit(EV_KEY, fpc1080->mouse_dev->evbit);
	set_bit(EV_REL, fpc1080->mouse_dev->evbit);
	input_set_capability(fpc1080->mouse_dev, EV_REL, REL_X);
	input_set_capability(fpc1080->mouse_dev, EV_REL, REL_Y);
	input_set_capability(fpc1080->mouse_dev, EV_KEY, BTN_LEFT);
	input_set_capability(fpc1080->mouse_dev, EV_KEY, BTN_RIGHT);
	input_set_capability(fpc1080->mouse_dev, EV_KEY, BTN_MIDDLE);
	input_set_capability(fpc1080->mouse_dev, EV_REL, REL_WHEEL);
	input_set_capability(fpc1080->mouse_dev, EV_REL, REL_HWHEEL);

#if defined (SUPPORT_DOUBLE_TAP)
	input_set_capability(fpc1080->mouse_dev, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(fpc1080->mouse_dev, EV_KEY, KEY_VOLUMEDOWN);
#endif

	input_set_capability(fpc1080->mouse_dev, EV_KEY, KEY_POWER);
	input_set_capability(fpc1080->mouse_dev, EV_KEY, KEY_SLEEP);
/* add for wake up */
	set_bit(EV_KEY, fpc1080->mouse_dev->evbit);
	set_bit(KEY_ENTER, fpc1080->mouse_dev->keybit);
	set_bit(KEY_UP, fpc1080->mouse_dev->keybit);
	set_bit(KEY_DOWN, fpc1080->mouse_dev->keybit);
	set_bit(KEY_RIGHT, fpc1080->mouse_dev->keybit);
	set_bit(KEY_LEFT, fpc1080->mouse_dev->keybit);
	set_bit(FPC1080_KEY_FINGER_PRESENT, fpc1080->mouse_dev->keybit);

	error = input_register_device(fpc1080->mouse_dev);

	if (error) {
		dev_err(&fpc1080->spi->dev, "input_register_device failed.\n");
		input_free_device(fpc1080->mouse_dev);
		fpc1080->mouse_dev = NULL;
		goto err;
	}

#endif


#ifdef CONFIG_FPC1080_TOUCH_PAD
					fpc1080->touch_pad_dev = input_allocate_device();

					if (!fpc1080->touch_pad_dev) {
						dev_err(&fpc1080->spi->dev, "input_allocate_device failed.\n");
						error  = -ENOMEM;
						goto err;
					}

					fpc1080->touch_pad_dev->name = FPC1080_TOUCH_PAD_DEV_NAME;
					set_bit(EV_KEY, fpc1080->touch_pad_dev->evbit);
					set_bit(EV_ABS, fpc1080->touch_pad_dev->evbit);
					set_bit(BTN_TOUCH, fpc1080->touch_pad_dev->keybit);
					set_bit(ABS_X, fpc1080->touch_pad_dev->absbit);
					set_bit(ABS_Y, fpc1080->touch_pad_dev->absbit);
					set_bit(ABS_Z, fpc1080->touch_pad_dev->absbit);
					input_set_abs_params(fpc1080->touch_pad_dev, ABS_X, 0, DEVICE_WIDTH, 0, 0);
					input_set_abs_params(fpc1080->touch_pad_dev, ABS_Y, 0, DEVICE_HEIGHT, 0, 0);

					error = input_register_device(fpc1080->touch_pad_dev);

					if (error) {
						 dev_err(&fpc1080->spi->dev, "input_register_device failed.\n");
						 input_free_device(fpc1080->touch_pad_dev);
						 fpc1080->nav_dev = NULL;
						 goto err;
					}
#endif



	sema_init(&fpc1080->mutex, 0);
	error = sysfs_create_group(&spi->dev.kobj, &fpc1080_adc_attr_group);
	if (error) {
		dev_err(&fpc1080->spi->dev, "sysf_create_group failed.\n");
		goto err;
	}

	error = sysfs_create_group(&spi->dev.kobj, &fpc1080_diag_attr_group);
	if (error) {
		dev_err(&fpc1080->spi->dev, "sysf_create_group failed.\n");
		goto err_sysf_1;
	}

#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
	error = sysfs_create_group(&spi->dev.kobj, &fpc1080_nav_settings_attr_group);
	if (error) {
		dev_err(&fpc1080->spi->dev, "sysf_create_group failed.\n");
		goto err_sysf_1;
	}
#endif

	error = register_chrdev_region(fpc1080->devno, 1, FPC1080_DEV_NAME);

	if (error) {
		dev_err(&fpc1080->spi->dev,
		  "fpc1080_probe - register_chrdev_region failed.\n");

		goto err_sysf_2;
	}

	cdev_init(&fpc1080->cdev, &fpc1080_fops);
	fpc1080->cdev.owner = THIS_MODULE;

	error = cdev_add(&fpc1080->cdev, fpc1080->devno, 1);

	if (error) {
		dev_err(&fpc1080->spi->dev, "cdev_add failed.\n");
		goto err_chrdev;
	}

	init_waitqueue_head(&fpc1080->thread_task.wait_job);
	sema_init(&fpc1080->thread_task.sem_idle, 0);
	fpc1080->thread_task.mode = FPC1080_THREAD_IDLE_MODE;
	fpc1080->thread_task.thread = kthread_run(threadfn, fpc1080, "%s",
						 FPC1080_WORKER_THREAD_NAME);
#ifdef CONFIG_HAS_EARLYSUSPEND
	fpc1080->early_suspend.suspend = fpc1080_early_suspend,
	fpc1080->early_suspend.resume = fpc1080_late_resume,
	fpc1080->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	register_early_suspend(&fpc1080->early_suspend);
#endif

	if (IS_ERR(fpc1080->thread_task.thread)) {
		dev_err(&fpc1080->spi->dev, "kthread_run failed.\n");
		goto err_chrdev;
	}

#if defined(CONFIG_FPC1080_NAVIGATION) || defined(CONFIG_FPC1080_MOUSE)
	init_enhanced_navi_setting(fpc1080);
	error = fpc1080_start_navigation(fpc1080);
	if (error)
		goto err_cdev;
#else

	error = fpc1080_spi_wr_reg(fpc1080, FPC1080_ACTIVATE_DEEP_SLEEP, 0, 1);
	if (error)
		goto err_cdev;

#endif

	up(&fpc1080->mutex);
	printk("[fp] %s: end \n", __func__);
	return 0;
err_cdev:
	cdev_del(&fpc1080->cdev);

err_chrdev:
	unregister_chrdev_region(fpc1080->devno, 1);

err_sysf_2:
	sysfs_remove_group(&spi->dev.kobj, &fpc1080_diag_attr_group);

err_sysf_1:
	sysfs_remove_group(&spi->dev.kobj, &fpc1080_adc_attr_group);

err:
	fpc1080_cleanup(fpc1080);
	spi_set_drvdata(spi, NULL);
	return error;
}

/* -------------------------------------------------------------------- */
static int __devexit fpc1080_remove(struct spi_device *spi)
{
	struct fpc1080_data *fpc1080 = spi_get_drvdata(spi);
#ifdef CONFIG_FPC1080_NAVIGATION
	fpc1080_stop_navigation(fpc1080);
	sysfs_remove_group(&fpc1080->spi->dev.kobj, &fpc1080_nav_settings_attr_group);
#endif
	sysfs_remove_group(&fpc1080->spi->dev.kobj, &fpc1080_adc_attr_group);
	sysfs_remove_group(&fpc1080->spi->dev.kobj, &fpc1080_diag_attr_group);

	fpc1080_spi_wr_reg(fpc1080, FPC1080_ACTIVATE_DEEP_SLEEP, 0, 1);

	cdev_del(&fpc1080->cdev);
	unregister_chrdev_region(fpc1080->devno, 1);
	fpc1080_cleanup(fpc1080);
	spi_set_drvdata(spi, NULL);
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_suspend(struct device *dev)
{
#ifdef CONFIG_FPC1080_NAVIGATION
	struct fpc1080_data *fpc1080 = dev_get_drvdata(dev);
	fpc1080_stop_navigation(fpc1080);
	fpc1080_spi_wr_reg(fpc1080, FPC1080_ACTIVATE_SLEEP, 0, 1);
#endif
	return 0;
}

/* -------------------------------------------------------------------- */
static int fpc1080_resume(struct device *dev)
{
#ifdef CONFIG_FPC1080_NAVIGATION
	struct fpc1080_data *fpc1080 = dev_get_drvdata(dev);
	fpc1080_start_navigation(fpc1080);
#endif
	return 0;
}
/* -------------------------------------------------------------------- */

static void fpc1080_early_suspend(struct early_suspend *h)
{
}

static void fpc1080_late_resume(struct early_suspend *h)
{
}


/* -------------------------------------------------------------------- */
static int __init fpc1080_init(void)
{
	if (spi_register_driver(&fpc1080_driver))
		return -EINVAL;

	return 0;
}

/* -------------------------------------------------------------------- */
static void __exit fpc1080_exit(void)
{
	spi_unregister_driver(&fpc1080_driver);
}

/* -------------------------------------------------------------------- */
