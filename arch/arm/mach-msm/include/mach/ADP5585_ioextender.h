/* include/asm/mach-msm/ADP5585_ioextender.h
 *
 * Copyright (C) 2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef _LINUX_ATMEGA_MICROP_H
#define _LINUX_ATMEGA_MICROP_H

#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>


#define IOEXTENDER_I2C_NAME "ADP5585-IOExtender"

#define IOEXTENDER_I2C_RCMD_VERSION		0x00

#define IOEXTENDER_I2C_RPULL_CONFIG 		0x17
#define IOEXTENDER_I2C_GPO_DATA_OUT		0x23
#define IOEXTENDER_I2C_GPIO_DATA_OUT_L 		0x23
#define IOEXTENDER_I2C_GPIO_DATA_OUT_H 		0x24
#define IOEXTENDER_I2C_GPO_OUT_MODE		0x25
#define IOEXTENDER_I2C_GPIO_DIRECTION 		0x27
#define IOEXTENDER_I2C_GPIO_USAGE		0x38


#define IOEXT_GPIO_1	0
#define IOEXT_GPIO_2	1
#define IOEXT_GPIO_3	2
#define IOEXT_GPIO_4	3
#define IOEXT_GPIO_5	4
#define IOEXT_GPIO_6	5
#define IOEXT_GPIO_7	6
#define IOEXT_GPIO_8	7
#define IOEXT_GPIO_9	8
#define IOEXT_GPIO_10	9
#define IOEXT_GPIO_11	10

struct ioext_i2c_platform_data {
	struct platform_device *ioext_devices;
	int			num_devices;
	uint32_t		gpio_reset;
	void 			*dev_id;
	void (*setup_gpio)(void);
	void (*reset_chip)(void);
};

struct ioext_i2c_client_data {
	struct mutex ioext_i2c_rw_mutex;
	struct mutex ioext_set_gpio_mutex;
	uint16_t version;
	struct early_suspend early_suspend;

	atomic_t ioext_is_suspend;
};

struct ioext_ops {
	int (*init_ioext_func)(struct i2c_client *);
};

int ioext_i2c_read(uint8_t addr, uint8_t *data, int length);
int ioext_i2c_write(uint8_t addr, uint8_t *data, int length);
int ioext_gpio_set_value(uint8_t gpio, uint8_t value);
int ioext_gpio_get_value(uint8_t gpio);
int ioext_read_gpio_status(uint8_t *data);
void ioext_register_ops(struct ioext_ops *ops);

#endif /* _LINUX_ATMEGA_MICROP_H */
