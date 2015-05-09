/* arch/arm/mach-msm/board-k2_ul-keypad.c
 *
 * Copyright (C) 2008 Google, Inc.
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#include "board-k2_ul.h"

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_k2_ul."

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[keypad]%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static struct gpio_event_direct_entry k2_ul_keypad_input_map[] = {
	{
		.gpio = MSM_PWR_KEYz,
		.code = KEY_POWER,
	},
	{
		.gpio = MSM_VOL_UPz,
		.code = KEY_VOLUMEUP,
	},
	{
		.gpio = MSM_VOL_DOWNz,
		.code = KEY_VOLUMEDOWN,
	 },
};

static void k2_ul_setup_input_gpio(void)
{
	uint32_t inputs_gpio_table[] = {
		GPIO_CFG(MSM_PWR_KEYz, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG(MSM_VOL_UPz, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG(MSM_VOL_DOWNz, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	};

	config_gpio_table(inputs_gpio_table, ARRAY_SIZE(inputs_gpio_table));
}

static void k2_ul_clear_hw_reset(void)
{
	uint32_t hw_clr_gpio_table[] = {
		GPIO_CFG(MSM_HW_RST_CLR, 0, GPIO_CFG_INPUT,
			 GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG(MSM_HW_RST_CLR, 0, GPIO_CFG_OUTPUT,
			 GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	};

	pr_info("[KEY] %s ++++++\n", __func__);
	gpio_tlmm_config(hw_clr_gpio_table[1], GPIO_CFG_ENABLE);
	gpio_set_value(MSM_HW_RST_CLR, 0);
	msleep(100);
	gpio_tlmm_config(hw_clr_gpio_table[0], GPIO_CFG_ENABLE);
	pr_info("[KEY] %s ------\n", __func__);
}

static struct gpio_event_input_info k2_ul_keypad_input_info = {
	.info.func             = gpio_event_input_func,
	.flags                 = GPIOEDF_PRINT_KEYS,
	.type                  = EV_KEY,
#if BITS_PER_LONG != 64 && !defined(CONFIG_KTIME_SCALAR)
	.debounce_time.tv.nsec = 20 * NSEC_PER_MSEC,
# else
	.debounce_time.tv64    = 20 * NSEC_PER_MSEC,
# endif
	.keymap                = k2_ul_keypad_input_map,
	.keymap_size           = ARRAY_SIZE(k2_ul_keypad_input_map),
	.setup_input_gpio      = k2_ul_setup_input_gpio,
	.clear_hw_reset        = k2_ul_clear_hw_reset,
};

static struct gpio_event_info *k2_ul_keypad_info[] = {
	&k2_ul_keypad_input_info.info,
};

static struct gpio_event_platform_data k2_ul_keypad_data = {
	.names = {
		"device-keypad",
		NULL,
	},
	.info = k2_ul_keypad_info,
	.info_count = ARRAY_SIZE(k2_ul_keypad_info),
};

static struct platform_device k2_ul_keypad_input_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &k2_ul_keypad_data,
	},
};
static struct keyreset_platform_data k2_ul_reset_keys_pdata = {
	/*.keys_up = k2_ul_reset_keys_up,*/
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		KEY_VOLUMEUP,
		0
	},
};

struct platform_device k2_ul_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &k2_ul_reset_keys_pdata,
};

int __init k2_ul_init_keypad(void)
{
	printk(KERN_DEBUG "[KEY]%s\n", __func__);

	if (platform_device_register(&k2_ul_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);

	return platform_device_register(&k2_ul_keypad_input_device);
}
