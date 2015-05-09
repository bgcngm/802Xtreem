/* arch/arm/mach-msm/board-fighter-keypad.c
 * Copyright (C) 2010 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>
#include <mach/board_htc.h>
#include <mach/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "board-monarudo.h"

/* static char *keycaps = "--qwerty"; */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_monarudo."

/* module_param_named(keycaps, keycaps, charp, 0); */

static struct gpio_event_direct_entry monarudo_keypad_map_xc[] = {
	{
		.gpio = PWR_KEY_MSMz,
		.code = KEY_POWER,
	},
	{
		.gpio = VOL_DOWNz_XC,
		.code = KEY_VOLUMEDOWN,
	},
	{
		.gpio = VOL_UPz_XC,
		.code = KEY_VOLUMEUP,
	},
};

static struct gpio_event_direct_entry monarudo_keypad_map[] = {
	{
		.gpio = PWR_KEY_MSMz,
		.code = KEY_POWER,
	},
	{
		.gpio = VOL_DOWNz_XA_XB,
		.code = KEY_VOLUMEDOWN,
	},
	{
		.gpio = VOL_UPz_XA_XB,
		.code = KEY_VOLUMEUP,
	},
};

static uint32_t matirx_inputs_gpio_table_xc[] = {
	GPIO_CFG(PWR_KEY_MSMz, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
	GPIO_CFG(VOL_DOWNz_XC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
	GPIO_CFG(VOL_UPz_XC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
};

static uint32_t matirx_inputs_gpio_table[] = {
	GPIO_CFG(PWR_KEY_MSMz, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
	GPIO_CFG(VOL_DOWNz_XA_XB, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
	GPIO_CFG(VOL_UPz_XA_XB, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		 GPIO_CFG_2MA),
};

static void monarudo_direct_inputs_gpio(void)
{

	if (system_rev >= 2) {
		gpio_tlmm_config(matirx_inputs_gpio_table_xc[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(matirx_inputs_gpio_table_xc[1], GPIO_CFG_ENABLE);
		gpio_tlmm_config(matirx_inputs_gpio_table_xc[2], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(matirx_inputs_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(matirx_inputs_gpio_table[1], GPIO_CFG_ENABLE);
		gpio_tlmm_config(matirx_inputs_gpio_table[2], GPIO_CFG_ENABLE);
	}

	return;
}

uint32_t hw_clr_gpio_table[] = {
	GPIO_CFG(RESET_EN_CLRz_XC, 0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(RESET_EN_CLRz_XC, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static void monarudo_clear_hw_reset(void)
{
	if (system_rev >= PVT ) { /* Function Ready after A01 */
		printk(KERN_INFO "[KEY] %s ++++++\n", __func__);
		gpio_tlmm_config(hw_clr_gpio_table[1], GPIO_CFG_ENABLE);
		gpio_set_value(RESET_EN_CLRz_XC, 0);
		msleep(100);
		gpio_tlmm_config(hw_clr_gpio_table[0], GPIO_CFG_ENABLE);
		printk(KERN_INFO "[KEY] %s ------\n", __func__);
	}
	else {
		printk(KERN_INFO "[KEY] %s: No Clear_HW_Reset pin\n", __func__);
	}

}

static struct gpio_event_input_info monarudo_keypad_power_info = {
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_PRINT_KEYS,
	.type = EV_KEY,
#if BITS_PER_LONG != 64 && !defined(CONFIG_KTIME_SCALAR)
	.debounce_time.tv.nsec = 20 * NSEC_PER_MSEC,
# else
	.debounce_time.tv64 = 20 * NSEC_PER_MSEC,
# endif
	.keymap = monarudo_keypad_map_xc,
	.keymap_size = ARRAY_SIZE(monarudo_keypad_map_xc),
	.setup_input_gpio = monarudo_direct_inputs_gpio,
	.clear_hw_reset = monarudo_clear_hw_reset,
};

static struct gpio_event_info *monarudo_keypad_info[] = {
	&monarudo_keypad_power_info.info,
};

static struct gpio_event_platform_data monarudo_keypad_data = {
	.name = "keypad_8960",
	.info = monarudo_keypad_info,
	.info_count = ARRAY_SIZE(monarudo_keypad_info),
};

static struct platform_device monarudo_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &monarudo_keypad_data,
	},
};

static struct keyreset_platform_data monarudo_reset_keys_pdata = {
	/*.keys_up = 8064_reset_keys_up,*/
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		KEY_VOLUMEUP,
		0
	},
};

static struct platform_device monarudo_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &monarudo_reset_keys_pdata,
};

int __init monarudo_init_keypad(void)
{
	if (platform_device_register(&monarudo_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);


	if (system_rev < 2) { /* XA and XB load different keymap*/
		monarudo_keypad_power_info.keymap = monarudo_keypad_map;
		monarudo_keypad_power_info.keymap_size =
			ARRAY_SIZE(monarudo_keypad_map);
        }


	return platform_device_register(&monarudo_keypad_device);
}

