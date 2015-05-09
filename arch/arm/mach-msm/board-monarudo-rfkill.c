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
#include <asm/setup.h>
#include <linux/mfd/pm8xxx/pm8921.h>

/* CXO workaround for DLX_XC start*/
#include <mach/msm_xo.h>
/* CXO workaround for DLX_XC end */

#include "board-monarudo.h"

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4334";

/* Add PMIC define for control 8921 start*/
struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};
/* CXO workaround for DLX_XC start*/
/* add xo handle for DLX_XC workaround */
struct msm_xo_voter *xo_handle; /*handle to vote for TCXO D1 buffer \kernel\include\linux\usb\msm_hsusb.h */
/* CXO workaround for DLX_XC end*/

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

struct pm8xxx_gpio_init monarudo_bt_pmic_gpio_xc[] = {
	PM8XXX_GPIO_INIT(BT_REG_ON_XC, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(BT_WAKE_XC, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(BT_HOST_WAKE_XC, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_DN, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_NO, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
};

/* Add PMIC define for control 8921 end */

/* bt on configuration */
static uint32_t monarudo_GPIO_bt_on_table[] = {

	/* BT_RTS */
	GPIO_CFG(BT_UART_RTSz_XC,
				2,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
	/* BT_CTS */
	GPIO_CFG(BT_UART_CTSz_XC,
				2,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_4MA),
	/* BT_RX */
	GPIO_CFG(BT_UART_RX_XC,
				2,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_4MA),
	/* BT_TX */
	GPIO_CFG(BT_UART_TX_XC,
				2,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
};

/* bt off configuration */
static uint32_t monarudo_GPIO_bt_off_table[] = {

	/* BT_RTS */
	GPIO_CFG(BT_UART_RTSz_XC,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_CTS */
	GPIO_CFG(BT_UART_CTSz_XC,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_RX */
	GPIO_CFG(BT_UART_RX_XC,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_4MA),
	/* BT_TX */
	GPIO_CFG(BT_UART_TX_XC,
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

static void monarudo_GPIO_config_bt_on(void)
{
	printk(KERN_INFO "[BT]== R ON ==\n");

/* CXO workaround for DLX_XC start*/
	if(system_rev == XC)
	{
		msm_xo_mode_vote(xo_handle, MSM_XO_MODE_ON);
		pr_info("[BT] %s: msm_xo_mode_vote MSM_XO_MODE_ON\n", __func__);
	}
/* CXO workaround for DLX_XC end*/
	/* set bt on configuration*/
	config_bt_table(monarudo_GPIO_bt_on_table,
				ARRAY_SIZE(monarudo_GPIO_bt_on_table));
	mdelay(2);


	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON_XC), 0);
	mdelay(5);

	/* BT_WAKE and HOST_WAKE */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_WAKE_XC), 0); //BT wake

	mdelay(5);
	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON_XC), 1);

	mdelay(1);

}

static void monarudo_GPIO_config_bt_off(void)
{

	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON_XC), 0);
	mdelay(1);

	/* set bt off configuration*/
	config_bt_table(monarudo_GPIO_bt_off_table,
				ARRAY_SIZE(monarudo_GPIO_bt_off_table));
	mdelay(2);

	/* BT_RTS */
	//gpio_set_value(BT_UART_RTSz_XC, 1);

	/* BT_CTS */

	/* BT_TX */
	//gpio_set_value(BT_UART_TX_XC, 1);

	/* BT_RX */


	/* BT_HOST_WAKE */

	/* BT_CHIP_WAKE */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_WAKE_XC), 0); //BT wake

	printk(KERN_INFO "[BT]== R OFF ==\n");
/* CXO workaround for DLX_XC start*/
	if(system_rev == XC)
	{
		msm_xo_mode_vote(xo_handle, MSM_XO_MODE_OFF);
		pr_info("[BT] %s: msm_xo_mode_vote MSM_XO_MODE_OFF\n",	__func__);
	}
/* CXO workaround for DLX_XC end*/
}

static int bluetooth_set_power(void *data, bool blocked)
{
	if (!blocked)
		monarudo_GPIO_config_bt_on();
	else
		monarudo_GPIO_config_bt_off();

	return 0;
}

static struct rfkill_ops monarudo_rfkill_ops = {
	.set_block = bluetooth_set_power,
};

static int monarudo_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true;  /* off */
	int i=0;

	/* always turn on clock? */
	/* htc_wifi_bt_sleep_clk_ctl(CLK_ON, ID_BT); */
	mdelay(2);

	for( i = 0; i < ARRAY_SIZE(monarudo_bt_pmic_gpio_xc); i++) {
		rc = pm8xxx_gpio_config(monarudo_bt_pmic_gpio_xc[i].gpio,
					&monarudo_bt_pmic_gpio_xc[i].config);
		if (rc)
			pr_info("[BT] %s: Config ERROR: GPIO=%u, rc=%d\n",
				__func__, monarudo_bt_pmic_gpio_xc[i].gpio, rc);
	}

	bluetooth_set_power(NULL, default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
				&monarudo_rfkill_ops, NULL);
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

static int monarudo_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	return 0;
}

static struct platform_driver monarudo_rfkill_driver = {
	.probe = monarudo_rfkill_probe,
	.remove = monarudo_rfkill_remove,
	.driver = {
		.name = "monarudo_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init monarudo_rfkill_init(void)
{
	if (system_rev < XC) {
		pr_err("[BT]%s: cannot run BT under DLX XA and XB \n", __func__);
		return 0;
	}
/* CXO workaround for DLX_XC start*/
	if(system_rev == XC) {
		xo_handle = msm_xo_get(MSM_XO_TCXO_D0, "bt");
		if (IS_ERR(xo_handle)) {
			pr_err("[BT] %s not able to get the handle to vote for TCXO D0 buffer\n", __func__);
		}
	}
/* CXO workaround for DLX_XC end*/
	return platform_driver_register(&monarudo_rfkill_driver);
}

static void __exit monarudo_rfkill_exit(void)
{
/* CXO workaround for DLX_XC start*/
	if(system_rev == XC)
		 msm_xo_put(xo_handle);
/* CXO workaround for DLX_XC end*/
	platform_driver_unregister(&monarudo_rfkill_driver);
}

module_init(monarudo_rfkill_init);
module_exit(monarudo_rfkill_exit);
MODULE_DESCRIPTION("monarudo rfkill");
MODULE_AUTHOR("htc_ssdbt <htc_ssdbt@htc.com>");
MODULE_LICENSE("GPL");
