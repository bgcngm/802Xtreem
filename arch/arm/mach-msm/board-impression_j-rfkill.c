/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009-2011 HTC Corporation.
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#include "board-impression_j.h"

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4334";

/* Add PMIC define for control 8921 start*/
struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

struct pm8xxx_gpio_init impression_j_bt_pmic_gpio[] = {
	PM8XXX_GPIO_INIT(BT_REG_ON, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(BT_WAKE, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(BT_HOST_WAKE, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_DN, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_NO, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
};

/* Add PMIC define for control 8921 end */

/* bt on configuration */
static uint32_t impression_j_GPIO_bt_on_table[] = {

	/* BT_RTS */
	GPIO_CFG(BT_UART_RTSz,
				2,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
	/* BT_CTS */
	GPIO_CFG(BT_UART_CTSz,
				2,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_4MA),
	/* BT_RX */
	GPIO_CFG(BT_UART_RX,
				2,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_4MA),
	/* BT_TX */
	GPIO_CFG(BT_UART_TX,
				2,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
};

/* bt off configuration */
static uint32_t impression_j_GPIO_bt_off_table[] = {

	/* BT_RTS */
	GPIO_CFG(BT_UART_RTSz,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_CTS */
	GPIO_CFG(BT_UART_CTSz,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_RX */
	GPIO_CFG(BT_UART_RX,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_TX */
	GPIO_CFG(BT_UART_TX,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
};

static void config_bt_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[BT]%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void impression_j_GPIO_config_bt_on(void)
{
	printk(KERN_INFO "[BT]== R ON ==\n");

	/* set bt on configuration*/
	config_bt_table(impression_j_GPIO_bt_on_table,
				ARRAY_SIZE(impression_j_GPIO_bt_on_table));
	mdelay(2);


	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON), 0);
	mdelay(5);

	/* BT_WAKE and HOST_WAKE */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_WAKE), 0); //BT wake

	mdelay(5);
	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON), 1);

	mdelay(1);

}

static void impression_j_GPIO_config_bt_off(void)
{

	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON), 0);
	mdelay(1);

	/* set bt off configuration*/
	config_bt_table(impression_j_GPIO_bt_off_table,
				ARRAY_SIZE(impression_j_GPIO_bt_off_table));
	mdelay(2);

	/* BT_RTS */
	//gpio_set_value(BT_UART_RTSz, 1);

	/* BT_CTS */

	/* BT_TX */
	//gpio_set_value(BT_UART_TX, 1);

	/* BT_RX */


	/* BT_HOST_WAKE */

	/* BT_CHIP_WAKE */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_WAKE), 0); //BT wake

	printk(KERN_INFO "[BT]== R OFF ==\n");
}

static int bluetooth_set_power(void *data, bool blocked)
{
	if (!blocked)
		impression_j_GPIO_config_bt_on();
	else
		impression_j_GPIO_config_bt_off();

	return 0;
}

static struct rfkill_ops impression_j_rfkill_ops = {
	.set_block = bluetooth_set_power,
};

static int impression_j_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true;  /* off */
	int i=0;

	/* always turn on clock? */
	/* htc_wifi_bt_sleep_clk_ctl(CLK_ON, ID_BT); */
	mdelay(2);

	for( i = 0; i < ARRAY_SIZE(impression_j_bt_pmic_gpio); i++) {
		rc = pm8xxx_gpio_config(impression_j_bt_pmic_gpio[i].gpio,
					&impression_j_bt_pmic_gpio[i].config);
		if (rc)
			pr_info("[bt] %s: Config ERROR: GPIO=%u, rc=%d\n",
				__func__, impression_j_bt_pmic_gpio[i].gpio, rc);
	}

	bluetooth_set_power(NULL, default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
				&impression_j_rfkill_ops, NULL);
	if (!bt_rfk) {
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_set_states(bt_rfk, default_state, false);

	/* userspace cannot take exclusive control */

	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	return 0;

err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_rfkill_alloc:
	return rc;
}

static int impression_j_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	return 0;
}

static struct platform_driver impression_j_rfkill_driver = {
	.probe = impression_j_rfkill_probe,
	.remove = impression_j_rfkill_remove,
	.driver = {
		.name = "impression_j_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init impression_j_rfkill_init(void)
{
	return platform_driver_register(&impression_j_rfkill_driver);
}

static void __exit impression_j_rfkill_exit(void)
{
	platform_driver_unregister(&impression_j_rfkill_driver);
}

module_init(impression_j_rfkill_init);
module_exit(impression_j_rfkill_exit);
MODULE_DESCRIPTION("impression_j rfkill");
MODULE_AUTHOR("htc_ssdbt <htc_ssdbt@htc.com>");
MODULE_LICENSE("GPL");
