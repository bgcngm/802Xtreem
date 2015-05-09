/*
 * Copyright (C) 2009 Google, Inc.
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

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <linux/io.h>

#include "board-jel_dd.h"
#include <linux/tty.h>
#include <mach/msm_serial_hs.h>

#define MODULE_NAME "[GSM_RADIO]" /* HTC version */

struct class *gsm_class;
static struct miscdevice gsm_radio_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc-gsm_radio",
};

#ifdef CONFIG_IMC_UART2DM_HANDSHAKE
u8 radio_state = 0;
u8 modem_fatal = 0;
extern int wakeup_irq;
#endif

int fatal_irq;

static int oldStatus=-1, newStatus=-1;//-1=init 0=insert 1=remove

static struct work_struct send_uevt_wq;

struct htc_simhotswap_info {
	struct kobject simhotswap_kobj;
	struct delayed_work hotswap_work;
	struct workqueue_struct *hotswap_wq;
	struct mutex lock;
};

static struct htc_simhotswap_info htc_hotswap_info;
static struct kset *htc_hotswap_kset;

static void htc_simhotswap_kobject_release(struct kobject *kobj)
{
	printk(KERN_ERR "htc_hotswap_kobject_release.\n");
	return;
}

static struct kobj_type htc_hotswap_ktype = {
	.release = htc_simhotswap_kobject_release,
};

static uint32_t jeweldd_gsm_radio_on_table[] = {
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_AP_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_AP_XMM_WAKE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_AP_STATUS, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_AP_XMM_STATUS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_PWR_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_RTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_SIM_DETECT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t jeweldd_gsm_radio_off_table[] = {
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_AP_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_AP_XMM_WAKE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_AP_STATUS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_AP_XMM_STATUS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_PWR_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_XMM_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_UART_RTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(JEL_DD_GPIO_GSM_SIM_DETECT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static void config_gsm_radio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void send_uevent_work(struct work_struct *work)
{
	printk(KERN_INFO MODULE_NAME "%s+\n",__func__);

	modem_fatal = 1;

	disable_irq_nosync(fatal_irq);
	if (wakeup_irq>=0)
		disable_irq_nosync(wakeup_irq);

	printk(KERN_INFO MODULE_NAME "%s send uevent\n",__func__);
	kobject_uevent(&gsm_radio_misc.this_device->kobj, KOBJ_CHANGE);

	printk(KERN_INFO MODULE_NAME "%s-\n",__func__);
}

static irqreturn_t imc_fatal_isr(int irq, void *dev)
{
	if (radio_state)
		schedule_work(&send_uevt_wq);

	return IRQ_HANDLED;
}

void radio_on(void)
{
	u16 retries=0;

	printk(KERN_INFO MODULE_NAME "radio_on+\n");

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_EN, 1);

	msleep(1000);

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 1);

	msleep(1);

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 0);

	msleep(27);

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_RESET, 1);

	msleep(1000);

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 1);

	msleep(1);

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 0);

	//msleep(100);

	config_gsm_radio_table(jeweldd_gsm_radio_on_table, ARRAY_SIZE(jeweldd_gsm_radio_on_table));

#ifdef CONFIG_IMC_UART2DM_HANDSHAKE
    while(!gpio_get_value(JEL_DD_GPIO_GSM_XMM_AP_STATUS)){
		if (retries>=3000){
			printk(KERN_INFO MODULE_NAME "%s wait for BB_STATUS + timeout -30s\n", __FUNCTION__);
			return;
		}

		msleep(10);
		retries++;
	}
	printk(KERN_INFO MODULE_NAME "BB_STATUS +\n");
#endif

	radio_state = 1;

	gpio_tlmm_config(GPIO_CFG(JEL_DD_GPIO_GSM_UART_TX, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(JEL_DD_GPIO_GSM_UART_RX, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(JEL_DD_GPIO_GSM_UART_CTS, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(JEL_DD_GPIO_GSM_UART_RTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	enable_irq(fatal_irq);

	modem_fatal = 0;

	printk(KERN_INFO MODULE_NAME "radio_on-\n");

}

void radio_off(void)
{
	printk(KERN_INFO MODULE_NAME "radio_off+\n");

	if (radio_state)
		disable_irq(fatal_irq);

	radio_state = 0;

	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_EN, 0);
	gpio_set_value(JEL_DD_GPIO_GSM_XMM_RESET, 0);
	gpio_set_value(JEL_DD_GPIO_GSM_XMM_PWR_ONOFF, 0);

	msleep(10);

	config_gsm_radio_table(jeweldd_gsm_radio_off_table, ARRAY_SIZE(jeweldd_gsm_radio_off_table));

	msleep(10);

	gpio_set_value(JEL_DD_GPIO_GSM_AP_XMM_WAKE, 0);
	gpio_set_value(JEL_DD_GPIO_GSM_AP_XMM_STATUS, 0);
	gpio_set_value(JEL_DD_GPIO_GSM_UART_TX, 0);
	gpio_set_value(JEL_DD_GPIO_GSM_UART_RTS, 0);

	printk(KERN_INFO MODULE_NAME "radio_off-\n");
}

static ssize_t htc_gsm_radio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;
	ret = sprintf(buf, "%d", radio_state);

	return ret;
}

static ssize_t htc_gsm_radio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int i;
	i = simple_strtoul(buf, NULL, 10);
	printk(KERN_INFO MODULE_NAME "%s\n",__func__);
	if (i){
		radio_on();
	}
	else{
		radio_off();
	}

	return size;
}

static void hotswap_work_func(struct work_struct *work)
{
	int status;
	char message[20] = "SIMHOTSWAP=";
	char *envp[] = { message, NULL };

	printk(KERN_WARNING MODULE_NAME "SIM_DETECT To buffering get SIM_Detect 90ms\n");

	mutex_lock(&htc_hotswap_info.lock);
	status = gpio_get_value(JEL_DD_GPIO_GSM_SIM_DETECT);
	if (status){
		strncat(message, "REMOVE", 6);
		printk(KERN_WARNING MODULE_NAME "SIM_DETECT REMOVE\n");
		newStatus=status;
		}else{
		strncat(message, "INSERT", 6);
		printk(KERN_WARNING MODULE_NAME "SIM_DETECT INSERT\n");
		newStatus=status;
		}
		pr_info("SIM_STATUS = %d oldStatus=%d --> newStatus=%d\n", status,oldStatus,newStatus);
	if(oldStatus!=newStatus){
		kobject_uevent_env(&htc_hotswap_info.simhotswap_kobj, KOBJ_CHANGE, envp);
		printk(KERN_WARNING MODULE_NAME "SIM_DETECT %s\n",status==1?"REMOVE":"INSERT");
		oldStatus=newStatus;
	}
	else{
		printk(KERN_WARNING MODULE_NAME "SIM_DETECT DONOT SEND UEVENT SIM_SWAP\n");
	}
	mutex_unlock(&htc_hotswap_info.lock);

	return;
}

static irqreturn_t sim_detect_irq(int irq, void *dev_id)
{
	schedule_delayed_work(&htc_hotswap_info.hotswap_work, msecs_to_jiffies(90));

	return IRQ_HANDLED;
}


static DEVICE_ATTR(set_radio, S_IRUGO | S_IWUSR, htc_gsm_radio_show, htc_gsm_radio_store);

static int __init gsm_radio_init(void) {
	int ret;
	int rc = 0;
	
	printk(KERN_INFO MODULE_NAME "gsm_radio_init()\n");

	ret = misc_register(&gsm_radio_misc);
	if (ret < 0) {
		printk(KERN_WARNING MODULE_NAME "failed to register misc device!\n");
		goto err_misc_register;
	}

	gsm_class = class_create(THIS_MODULE, "gsm_radio");
	if (IS_ERR(gsm_class)) {
		ret = PTR_ERR(gsm_class);
		gsm_class = NULL;
		printk(KERN_WARNING MODULE_NAME "class_create failed!\n");
		goto err_class_create;
	}

	gsm_radio_misc.this_device = device_create(gsm_class, NULL, 0 , NULL, "pwr_ctl");
	if (IS_ERR(gsm_radio_misc.this_device)) {
		ret = PTR_ERR(gsm_radio_misc.this_device);
		gsm_radio_misc.this_device = NULL;
		printk(KERN_WARNING MODULE_NAME "device_create failed!\n");
		goto err_device_create;
	}

	ret = device_create_file(gsm_radio_misc.this_device, &dev_attr_set_radio);
	if (ret < 0) {
		printk(KERN_WARNING MODULE_NAME "devices_create_file failed!\n");
		goto err_device_create_file;
	}

	radio_off();

	INIT_WORK(&send_uevt_wq, send_uevent_work);

	//imc_pmic_init();

	fatal_irq = gpio_to_irq(PM8921_GPIO_PM_TO_SYS(JEL_DD_GPIO_GSM_XMM_AP_FATAL));

	rc = request_irq(fatal_irq, imc_fatal_isr, IRQF_TRIGGER_RISING, "msm_hs_fatal", gsm_radio_misc.this_device);
	if (unlikely(rc)) {
		printk(KERN_WARNING MODULE_NAME "request XMM_AP_FATAL irq failed!");
		goto err_device_create_file;
	}

	irq_set_irq_wake(fatal_irq, 1);

	disable_irq(fatal_irq);

	// sim hotswap
	mutex_init(&htc_hotswap_info.lock);

	INIT_DELAYED_WORK(&htc_hotswap_info.hotswap_work, hotswap_work_func);
	htc_hotswap_info.hotswap_wq = create_singlethread_workqueue("htc_simhotswap");

	htc_hotswap_kset = kset_create_and_add("event", NULL,
			kobject_get(&gsm_radio_misc.this_device->kobj));
	if (!htc_hotswap_kset) {
		ret = -ENOMEM;
		goto err_device_create_file;
	}

	htc_hotswap_info.simhotswap_kobj.kset = htc_hotswap_kset;

	ret = kobject_init_and_add(&htc_hotswap_info.simhotswap_kobj,
			&htc_hotswap_ktype, NULL, "simhotswap");
	if (ret) {
		kobject_put(&htc_hotswap_info.simhotswap_kobj);
		goto err_device_create_file;
	}
	oldStatus = gpio_get_value(JEL_DD_GPIO_GSM_SIM_DETECT);
	printk(KERN_WARNING MODULE_NAME "htc_simhotswap_probe(): finish SIM init status=%d\n",oldStatus);

	ret = request_irq(gpio_to_irq(JEL_DD_GPIO_GSM_SIM_DETECT),
			sim_detect_irq,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"sim_detect", NULL);
	if (ret) {
		printk(KERN_WARNING MODULE_NAME "%s:Failed to request irq, ret=%d\n", __func__, ret);
	}

	return 0;

err_device_create_file:
	device_destroy(gsm_class, 0);
err_device_create:
	class_destroy(gsm_class);
err_class_create:
	misc_deregister(&gsm_radio_misc);
err_misc_register:
	return ret;
}

static void __exit gsm_radio_exit(void) {
	int ret;

	irq_set_irq_wake(fatal_irq, 0);
	free_irq(fatal_irq, gsm_radio_misc.this_device);

	device_remove_file(gsm_radio_misc.this_device, &dev_attr_set_radio);
	device_destroy(gsm_class, 0);
	class_destroy(gsm_class);

	ret = misc_deregister(&gsm_radio_misc);
	if (ret < 0)
		printk(KERN_WARNING MODULE_NAME"failed to unregister misc device!\n");
}

late_initcall(gsm_radio_init);
module_exit(gsm_radio_exit);

MODULE_AUTHOR("Ceci Wu <ceci_wu@htc.com>");
MODULE_DESCRIPTION("HTC GSM RADIO driver");
MODULE_LICENSE("GPL");

