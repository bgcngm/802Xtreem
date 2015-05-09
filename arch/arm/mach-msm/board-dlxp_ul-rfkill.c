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
#include <mach/htc_4335_wl_reg.h>

#include "board-dlxp_ul.h"

//BCM4335_WiFiBTCOEX++
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/* Broadcom BT/Wi-Fi on/off lock */
#define BTLOCK_NAME     "btlock"
#define BTLOCK_MINOR    MISC_DYNAMIC_MINOR //224

/* BT lock waiting timeout, in second */
#define BTLOCK_TIMEOUT	2

#define PR(msg, ...) printk("####"msg, ##__VA_ARGS__)

struct btlock {
	int lock;
	int cookie;
};

static struct semaphore btlock;
static int count = 1;
static int owner_cookie = -1;

int bcm_bt_lock(int cookie)
{
	int ret;
	char cookie_msg[5] = {0};

	ret = down_timeout(&btlock, msecs_to_jiffies(BTLOCK_TIMEOUT*1000));
	if (ret == 0) {
		memcpy(cookie_msg, &cookie, sizeof(cookie));
		owner_cookie = cookie;
		count--;
		PR("btlock acquired cookie: %s\n", cookie_msg);
	}

	return ret;
}

void bcm_bt_unlock(int cookie)
{
	char owner_msg[5] = {0};
	char cookie_msg[5] = {0};

	memcpy(cookie_msg, &cookie, sizeof(cookie));
	if (owner_cookie == cookie) {
		owner_cookie = -1;
		if (count++ > 1)
			PR("error, release a lock that was not acquired**\n");
		up(&btlock);
		PR("btlock released, cookie: %s\n", cookie_msg);
	} else {
		memcpy(owner_msg, &owner_cookie, sizeof(owner_cookie));
		PR("ignore lock release,  cookie mismatch: %s owner %s \n", cookie_msg,
				owner_cookie == 0 ? "NULL" : owner_msg);
	}
}

static int btlock_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int btlock_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t btlock_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	struct btlock lock_para;
	ssize_t ret = 0;

	if (count < sizeof(struct btlock))
		return -EINVAL;

	if (copy_from_user(&lock_para, buffer, sizeof(struct btlock))) {
		return -EFAULT;
	}

	if (lock_para.lock == 0) {
		bcm_bt_unlock(lock_para.cookie);
	} else if (lock_para.lock == 1) {
		ret = bcm_bt_lock(lock_para.cookie);
	} else if (lock_para.lock == 2) {
		ret = bcm_bt_lock(lock_para.cookie);
	}

	return ret;
}

static const struct file_operations btlock_fops = {
	.owner   = THIS_MODULE,
	.open    = btlock_open,
	.release = btlock_release,
	.write   = btlock_write,
};

static struct miscdevice btlock_misc = {
	.name  = BTLOCK_NAME,
	.minor = BTLOCK_MINOR,
	.fops  = &btlock_fops,
};

static int bcm_btlock_init(void)
{
	int ret;

	PR("init\n");

	ret = misc_register(&btlock_misc);
	if (ret != 0) {
		PR("Error: failed to register Misc driver,  ret = %d\n", ret);
		return ret;
	}
	sema_init(&btlock, 1);

	return ret;
}

static void bcm_btlock_exit(void)
{
	PR("btlock_exit:\n");

	misc_deregister(&btlock_misc);
}
//BCM4335_WiFiBTCOEX--

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

struct pm8xxx_gpio_init dlxp_ul_bt_pmic_gpio[] = {
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
static uint32_t dlxp_ul_GPIO_bt_on_table[] = {

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
static uint32_t dlxp_ul_GPIO_bt_off_table[] = {

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

static void dlxp_ul_GPIO_config_bt_on(void)
{
	printk(KERN_INFO "[BT]== R ON ==\n");

	/* set bt on configuration*/
	config_bt_table(dlxp_ul_GPIO_bt_on_table,
				ARRAY_SIZE(dlxp_ul_GPIO_bt_on_table));
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

static void dlxp_ul_GPIO_config_bt_off(void)
{

	/* BT_REG_ON */
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(BT_REG_ON), 0);
	mdelay(1);

	/* set bt off configuration*/
	config_bt_table(dlxp_ul_GPIO_bt_off_table,
				ARRAY_SIZE(dlxp_ul_GPIO_bt_off_table));
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
		dlxp_ul_GPIO_config_bt_on();
	else
		dlxp_ul_GPIO_config_bt_off();

	return 0;
}

static struct rfkill_ops dlxp_ul_rfkill_ops = {
	.set_block = bluetooth_set_power,
};

static int dlxp_ul_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true;  /* off */
	int i=0;

	/* always turn on clock? */
	/* htc_wifi_bt_sleep_clk_ctl(CLK_ON, ID_BT); */
	mdelay(2);

	for( i = 0; i < ARRAY_SIZE(dlxp_ul_bt_pmic_gpio); i++) {
		rc = pm8xxx_gpio_config(dlxp_ul_bt_pmic_gpio[i].gpio,
					&dlxp_ul_bt_pmic_gpio[i].config);
		if (rc)
			pr_info("[bt] %s: Config ERROR: GPIO=%u, rc=%d\n",
				__func__, dlxp_ul_bt_pmic_gpio[i].gpio, rc);
	}

//BCM4335_WiFiBTCOEX++
	bcm_btlock_init();
//BCM4335_WiFiBTCOEX--

	bluetooth_set_power(NULL, default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
				&dlxp_ul_rfkill_ops, NULL);
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

static int dlxp_ul_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);

//BCM4335_WiFiBTCOEX++
	bcm_btlock_exit();
//BCM4335_WiFiBTCOEX--

	return 0;
}

static struct platform_driver dlxp_ul_rfkill_driver = {
	.probe = dlxp_ul_rfkill_probe,
	.remove = dlxp_ul_rfkill_remove,
	.driver = {
		.name = "dlxp_ul_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init dlxp_ul_rfkill_init(void)
{
	return platform_driver_register(&dlxp_ul_rfkill_driver);
}

static void __exit dlxp_ul_rfkill_exit(void)
{
	platform_driver_unregister(&dlxp_ul_rfkill_driver);
}

module_init(dlxp_ul_rfkill_init);
module_exit(dlxp_ul_rfkill_exit);
MODULE_DESCRIPTION("dlxp_ul rfkill");
MODULE_AUTHOR("htc_ssdbt <htc_ssdbt@htc.com>");
MODULE_LICENSE("GPL");
