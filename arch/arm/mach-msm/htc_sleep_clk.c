/* arch/arm/mach-msm/include/mach/htc_sleep_clk.c
 *
 * Copyright (C) 2010 HTC, Inc.
 * Author: assd bt <assd_bt@htc.com>
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

/* Control bluetooth power for glacier platform */

#include <asm/mach-types.h>

#include <mach/htc_sleep_clk.h>
#include <linux/mfd/pm8xxx/pm8038.h>
#include "board-8930.h"

static int htc_sleep_clk_pin = -2;
static int htc_sleep_clk_state_wifi;
static int htc_sleep_clk_state_bt;
static DEFINE_MUTEX(htc_w_b_mutex);

static struct regulator *reg_l13 = NULL;
static struct regulator *reg_l25 = NULL;

/* pmic config */
static struct pm_gpio pmic_gpio_sleep_clk_output = {
	.direction      = PM_GPIO_DIR_OUT,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 0,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = PM8038_GPIO_VIN_L11,
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_1,
	.inv_int_pol    = 0,
	.disable_pin    = 0,
};

int pmic_regulator_enable(struct regulator **preg, char *name, unsigned int volt)
{
	int err = 0;

	if (preg == NULL)
		return -EINVAL;

	if (*preg == NULL) { /* If the reg had not been initialized */
		if (strlen(name) == 0)
			return -EINVAL;

		*preg = regulator_get(NULL, name);
		if (IS_ERR(*preg)) {
			printk(KERN_DEBUG "Unable to get %s\n", name);
			err = -ENODEV;
			goto err_reg_get;
		}

		err = regulator_set_voltage(*preg, volt, volt);
		if (err) {
			printk(KERN_DEBUG "Unable to set %s voltage to %d, err=%d\n", name, volt, err);
			goto err_reg_set;
		}

		printk(KERN_DEBUG "HTC sleep clk: %s initialized\n", name);
	}

	err = regulator_enable(*preg);
	if (err) {
		printk(KERN_DEBUG "Unable to enable %s, err=%d\n", name, err);
		goto err_reg_set;
	}

	printk(KERN_DEBUG "HTC sleep clk: %s enabled\n", name);
	return 0;

err_reg_set:
	regulator_put(*preg);
err_reg_get:
	*preg = NULL;
	return err;
}

int pmic_regulator_disable(struct regulator **preg)
{
	int err = 0;

	if (preg == NULL)
		return -EINVAL;

	if (*preg == NULL) {
		printk(KERN_DEBUG "HTC sleep clk: reg not initialized\n");
		return -EINVAL;
	}

	err = regulator_disable(*preg);
	if (err) {
		printk(KERN_DEBUG "Unable to disable reg, err=%d\n", err);
		regulator_put(*preg);
		*preg = NULL;
	}

	return err;
}

int set_wifi_bt_sleep_clk(int on)
{
	int err = 0;

	if (on) {
		printk(KERN_DEBUG "EN SLEEP CLK\n");

		pmic_regulator_enable(&reg_l13, "8038_l13", 2220000);
		pmic_regulator_enable(&reg_l25, "8038_l25", 1740000);

		pmic_gpio_sleep_clk_output.function = PM_GPIO_FUNC_1;
		err = pm8xxx_gpio_config(PM8038_GPIO_PM_TO_SYS(htc_sleep_clk_pin),
				&pmic_gpio_sleep_clk_output);
	} else {
		printk(KERN_DEBUG "DIS SLEEP CLK\n");

		pmic_gpio_sleep_clk_output.function = PM_GPIO_FUNC_NORMAL;
		err = pm8xxx_gpio_config(PM8038_GPIO_PM_TO_SYS(htc_sleep_clk_pin),
				&pmic_gpio_sleep_clk_output);

		pmic_regulator_disable(&reg_l25);
		pmic_regulator_disable(&reg_l13);
	}

	if (err) {
		if (on)
			printk(KERN_DEBUG "ERR EN SLEEP CLK, err=%d\n", err);
		else
			printk(KERN_DEBUG "ERR DIS SLEEP CLK, err=%d\n", err);
	}

	return err;
}

int htc_wifi_bt_sleep_clk_ctl(int on, int id)
{
	int err = 0;

	printk(KERN_DEBUG "%s ON=%d, ID=%d\n", __func__, on, id);

	if (htc_sleep_clk_pin < 0) {
		printk(KERN_DEBUG "== ERR SLP CLK PIN=%d ==\n",
			htc_sleep_clk_pin);
		return htc_sleep_clk_pin;
	}

	mutex_lock(&htc_w_b_mutex);
	if (on) {
		if ((CLK_OFF == htc_sleep_clk_state_wifi)
			&& (CLK_OFF == htc_sleep_clk_state_bt)) {

			err = set_wifi_bt_sleep_clk(CLK_ON);

			if (err) {
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
		}

		if (id == ID_BT)
			htc_sleep_clk_state_bt = CLK_ON;
		else
			htc_sleep_clk_state_wifi = CLK_ON;
	} else {
		if (((id == ID_BT) && (CLK_OFF == htc_sleep_clk_state_wifi))
			|| ((id == ID_WIFI)
			&& (CLK_OFF == htc_sleep_clk_state_bt))) {

			err = set_wifi_bt_sleep_clk(CLK_OFF);

			if (err) {
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
		} else {
			printk(KERN_DEBUG "KEEP SLEEP CLK ALIVE\n");
		}

		if (id)
			htc_sleep_clk_state_bt = CLK_OFF;
		else
			htc_sleep_clk_state_wifi = CLK_OFF;
	}
	mutex_unlock(&htc_w_b_mutex);

	printk(KERN_DEBUG "%s ON=%d, ID=%d DONE\n", __func__, on, id);

	return 0;
}

int htc_sleep_clk_init(int sleep_clk_pin)
{
	htc_sleep_clk_pin = sleep_clk_pin;

	htc_sleep_clk_state_wifi = CLK_OFF;
	htc_sleep_clk_state_bt = CLK_OFF;

	printk(KERN_DEBUG "%s, pin=%d\n", __func__, htc_sleep_clk_pin);

	return true;
}
