/*! @file vfsSpiDrv.h
*******************************************************************************
**  SPI Driver Interface Functions
**
**  This file contains the SPI driver interface functions.
**
**  Copyright 2011-2012 Validity Sensors, Inc. All Rights Reserved.
*/

#ifndef VFSSPIDRV_H_
#define VFSSPIDRV_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <linux/irq.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#if 0
#define DPRINTK(fmt, args...) printk(KERN_ERR "vfsspi:"fmt, ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/* Major number of device ID.
 * A device ID consists of two parts: a major number, identifying the class of
 * the device, and a minor ID, identifying a specific instance of a device in
 * that class. A device ID is represented using the type dev_t. The minor number
 * of the Validity device is 0. */
#define VFSSPI_MAJOR         (221)

/* Maximum transfer size */
#define DEFAULT_BUFFER_SIZE  (4096 * 5)

#if VCS_FEATURE_SENSOR_WINDSOR
#define DRDY_ACTIVE_STATUS      1
#define BITS_PER_WORD           8
#define DRDY_IRQ_FLAG           IRQF_TRIGGER_RISING
#else /* VCS_FEATURE_SENSOR_WINDSOR */
#define DRDY_ACTIVE_STATUS      0
#if defined(CONFIG_FPR_SPI_DMA_GSBI1) || defined(CONFIG_FPR_SPI_DMA_GSBI5)
#define BITS_PER_WORD           8
#else
#define BITS_PER_WORD           16
#endif
#define DRDY_IRQ_FLAG           IRQF_TRIGGER_FALLING
#endif /* VCS_FEATURE_SENSOR_WINDSOR */

/* Timeout value for polling DRDY signal assertion */
#define DRDY_TIMEOUT_MS      40

/* Magic number of IOCTL command */
#define VFSSPI_IOCTL_MAGIC    'k'

/*
 * IOCTL commands definitions
 */

/* Transmit data to the device and retrieve data from it simultaneously */
#define VFSSPI_IOCTL_RW_SPI_MESSAGE         _IOWR(VFSSPI_IOCTL_MAGIC,   \
							1, unsigned int)

/* Hard reset the device */
#define VFSSPI_IOCTL_DEVICE_RESET           _IO(VFSSPI_IOCTL_MAGIC,   2)

/* Set the baud rate of SPI master clock */
#define VFSSPI_IOCTL_SET_CLK                _IOW(VFSSPI_IOCTL_MAGIC,    \
							3, unsigned int)

/* Get level state of DRDY GPIO */
#define VFSSPI_IOCTL_CHECK_DRDY             _IO(VFSSPI_IOCTL_MAGIC,   4)

/* Register DRDY signal. It is used by SPI driver for indicating host
 * that DRDY signal is asserted. */
#define VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL   _IOW(VFSSPI_IOCTL_MAGIC,    \
							5, unsigned int)

/* Store the user data into the SPI driver. Currently user data is a
 * device info data, which is obtained from announce packet. */
#define VFSSPI_IOCTL_SET_USER_DATA          _IOW(VFSSPI_IOCTL_MAGIC,    \
							6, unsigned int)

/* Retrieve user data from the SPI driver*/
#define VFSSPI_IOCTL_GET_USER_DATA          _IOWR(VFSSPI_IOCTL_MAGIC,   \
							7, unsigned int)

/* Enable/disable DRDY interrupt handling in the SPI driver */
#define VFSSPI_IOCTL_SET_DRDY_INT           _IOW(VFSSPI_IOCTL_MAGIC,    \
							8, unsigned int)

/* Put device in Low power mode */
#define VFSSPI_IOCTL_DEVICE_SUSPEND         _IO(VFSSPI_IOCTL_MAGIC,   9)

/* Indicate the fingerprint buffer size for read */
#define VFSSPI_IOCTL_STREAM_READ_START      _IOW(VFSSPI_IOCTL_MAGIC,	\
							10, unsigned int)
/* Indicate that fingerprint acquisition is completed */
#define VFSSPI_IOCTL_STREAM_READ_STOP       _IO(VFSSPI_IOCTL_MAGIC,   11)

/* Retrieve supported SPI baud rate table */
#define VFSSPI_IOCTL_GET_FREQ_TABLE         _IOWR(VFSSPI_IOCTL_MAGIC,	\
							12, unsigned int)

/* Turn on the power to the sensor */
#define VFSSPI_IOCTL_POWER_ON               _IO(VFSSPI_IOCTL_MAGIC,   13)

/* Turn off the power to the sensor */
#define VFSSPI_IOCTL_POWER_OFF              _IO(VFSSPI_IOCTL_MAGIC,   14)

/* Register SUSPENDRESUME signal. It is used by SPI driver for indicating host that
 * SUSPEND/RESUME signal is asserted. */
#define VFSSPI_IOCTL_REGISTER_SUSPENDRESUME_SIGNAL    _IOW(VFSSPI_IOCTL_MAGIC,	\
							 19, unsigned int)

/* Retrieive System status:
 * 0 - Awake;
 * 1 - Suspend
*/
#define VFSSPI_IOCTL_GET_SYSTEM_STATUS  _IOR(VFSSPI_IOCTL_MAGIC,	\
							 20, unsigned int)

#define VFSSPI_DRDY_PIN     55 /* irq */
#define VFSSPI_SLEEP_PIN    2  /* reset */

#define SLOW_BAUD_RATE  5400000  /* 1100000/4800000/5400000 */
#define MAX_BAUD_RATE   15060000 /* 1100000/9600000/10800000 */

#define BAUD_RATE_COEF  1000

/*
 * Definitions of structures which are used by IOCTL commands
 */

/* Pass to VFSSPI_IOCTL_SET_USER_DATA and VFSSPI_IOCTL_GET_USER_DATA commands */
struct vfsspi_iocUserData {
	void *buffer;
	unsigned int len;
};

/* Pass to VFSSPI_IOCTL_RW_SPI_MESSAGE command */
struct vfsspi_iocTransfer {
	unsigned char *rxBuffer;    /* pointer to retrieved data */
	unsigned char *txBuffer;    /* pointer to transmitted data */
	unsigned int len;   /* transmitted/retrieved data size */
};

/* Pass to VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL command */
struct vfsspi_iocRegSignal {
	int userPID;        /* Process ID to which SPI driver sends
				signal indicating that DRDY is asserted */
	int signalID;       /* Signal number */
};

/* Pass to VFSSPI_IOCTL_GET_FREQ_TABLE command */
/**
* vfsspi_iocFreqTable - structure to get supported SPI baud rates
*
* @table:table which contains supported SPI baud rates
* @tblSize:table size
*/
typedef struct vfsspi_iocFreqTable {
	unsigned int *table;
	unsigned int  tblSize;
} vfsspi_iocFreqTable_t;

struct vfsspi_devData {
	dev_t devt;
	spinlock_t vfsSpiLock;
	struct spi_device *spi;
	struct list_head deviceEntry;
	struct mutex bufferMutex;
	unsigned int isOpened;
	unsigned char *buffer;
	unsigned char *nullBuffer;
	unsigned char *streamBuffer;
	unsigned int *freqTable;
	unsigned int freqTableSize;
	size_t streamBufSize;
	struct vfsspi_iocUserData userInfoData;
	unsigned int drdyPin;
	unsigned int sleepPin;
	int userPID;
	int signalID;
	int eUserPID;
	int eSignalID;
	unsigned int curSpiSpeed;
};


#if 0
GSBI to be used : GSBI5
GPIO assignment:
	GPIO 49 - SPI_FP_MOSI
	GPIO 50 - SPI_FP_MISO
	GPIO 51 - SPI_FP_CS_N
	GPIO 52 - SPI_FP_CLK
	GPIO 154 - FP_DRDY_N
	GPIO 131 - FP_SLEEP
#endif
#endif              /* VFSSPIDRV_H_ */
