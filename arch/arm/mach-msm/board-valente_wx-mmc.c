#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/system_info.h>
#include <mach/vreg.h>
#include <mach/htc_pwrsink.h>

#include <asm/mach/mmc.h>

#include "devices.h"
#include "board-valente_wx.h"
#include "board-common-wimax.h"

#include <mach/msm_iomap.h>
#include <linux/irq.h>
#include "board-valente_wx-mmc.h"

#include <mach/rpm.h>
#include <mach/rpm-regulator.h>
#include "rpm_resources.h"

/* ---- PM QOS ---- */
/*
static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 8000, 100000, 1,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1500, 5000, 60100000, 3000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		false,
		1800, 5000, 60350000, 3500,
	},
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, ACTIVE, MAX, ACTIVE),
		false,
		3800, 4500, 65350000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		2800, 2500, 66850000, 4800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		4800, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6800, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		7800, 0, 76350000, 9800,
	},
};


static uint32_t msm_rpm_get_swfi_latency(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_rpmrs_levels); i++) {
		if (msm_rpmrs_levels[i].sleep_mode ==
			MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)
			return msm_rpmrs_levels[i].latency_us;
	}

	return 0;
}
*/

/* ---- SDCARD ---- */
/* ---- WIFI ---- */

#ifdef CONFIG_WIMAX

/* ---- WIMAX ---- */
static uint32_t wimax_on_gpio_table[] = {
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CLK_CPU, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CMD, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D0,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D1,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D2,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D3,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT3 */
};

static uint32_t wimax_reset_table[] = {
	GPIO_CFG(VALENTE_WX_WIMAX_EXT_RSTz,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* EXT_RST */
	GPIO_CFG(VALENTE_WX_WIMAX_EXT_RSTz,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* EXT_RST */
};

static uint32_t wimax_off_gpio_table[] = {
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CLK_CPU, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CMD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D1,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT3 */
};

static uint32_t wimax_initial_gpio_table[] = {
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CLK_CPU, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_CMD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D1,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(VALENTE_WX_WIMAX_SDIO_D3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT3 */
	GPIO_CFG(VALENTE_WX_WIMAX_EXT_RSTz,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* EXT_RST */
};

static int mmc_wimax_initial_gpio_set_value(void)
{
	printk(KERN_INFO "%s:[WIMAX] set GPIO value to default \n", __func__);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_CLK_CPU,0);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_CMD,0);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_D0,0);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_D1,0);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_D2,0);
	gpio_set_value(VALENTE_WX_WIMAX_SDIO_D3,0);
	gpio_set_value(VALENTE_WX_WIMAX_EXT_RSTz,0);
	return 0;
}

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
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

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE_FOR_WIMAX(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT_FOR_WIMAX(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_LOW, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_HIGH_FOR_WIMAX(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT_FOR_WIMAX(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* For FATAL_ERROR init */
#define PM8XXX_GPIO_INPUT_FOR_WIMAX_FATAL_ERROR(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FOR_WIMAX_FUNC_FOR_WIMAX(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_LOW, \
			_func, 0, 0)


static struct pm8xxx_gpio_init wimax_init_pmgpios[] __initdata = {
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_PVDD_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_DVDD_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_HIGH_FOR_WIMAX(VALENTE_WX_V_WIMAX_1V2_RF_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_WIMAX_SLEEP_CLK, 0),
	PM8XXX_GPIO_INPUT_FOR_WIMAX_FATAL_ERROR(VALENTE_WX_FATAL_ERROR_IND, PM_GPIO_PULL_DN),
};

static struct pm8xxx_gpio_init wimax_pvdd_pmgpios[] = {
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_PVDD_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_PVDD_EN, 1),
};

static struct pm8xxx_gpio_init wimax_dvdd_pmgpios[] = {
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_DVDD_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_V_WIMAX_DVDD_EN, 1),
};

static struct pm8xxx_gpio_init wimax_1v2_pmgpios[] = {
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_HIGH_FOR_WIMAX(VALENTE_WX_V_WIMAX_1V2_RF_EN, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_HIGH_FOR_WIMAX(VALENTE_WX_V_WIMAX_1V2_RF_EN, 1),
};

static struct pm8xxx_gpio_init wimax_sleep_clk_pmgpios[] = {
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_STRE_LOW_FOR_WIMAX(VALENTE_WX_WIMAX_SLEEP_CLK, 0),
	PM8XXX_GPIO_OUTPUT_FOR_WIMAX_FUNC_FOR_WIMAX(VALENTE_WX_WIMAX_SLEEP_CLK, 0, PM_GPIO_FUNC_1),
};

static void (*wimax_status_cb)(int card_present, void *dev_id);
static void *wimax_status_cb_devid;
static int mmc_wimax_cd = 0;
static int mmc_wimax_hostwakeup_gpio = PM8921_GPIO_PM_TO_SYS(VALENTE_WX_FATAL_ERROR_IND);

static int mmc_wimax_status_register(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	if (wimax_status_cb)
		return -EAGAIN;
	printk(KERN_INFO "[WIMAX] %s\n", __func__);
	wimax_status_cb = callback;
	wimax_status_cb_devid = dev_id;
	return 0;
}

static unsigned int mmc_wimax_status(struct device *dev)
{
	printk(KERN_INFO "[WIMAX] %s\n", __func__);
	return mmc_wimax_cd;
}

int mmc_wimax_set_carddetect(int val)
{
	printk(KERN_INFO "[WIMAX] %s: %d\n", __func__, val);
	mmc_wimax_cd = val;
	if (wimax_status_cb)
		wimax_status_cb(val, wimax_status_cb_devid);
	else
		printk(KERN_WARNING "[WIMAX] %s: Nobody to notify\n", __func__);

	return 0;
}
EXPORT_SYMBOL(mmc_wimax_set_carddetect);

static unsigned int mmc_wimax_type = MMC_TYPE_SDIO_WIMAX;

static struct mmc_platform_data mmc_wimax_data = {
	.ocr_mask		= MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30,
	.status			= mmc_wimax_status,
	.register_status_notify	= mmc_wimax_status_register,
	.embedded_sdio		= NULL,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin   = 400000,
	.msmsdcc_fmid   = 24000000,
	.msmsdcc_fmax   = 48000000,
	.nonremovable   = 0,
	.slot_type		= &mmc_wimax_type,
//	.pclk_src_dfab	= 1,
};

int mmc_wimax_power(int on)
{
	int i, rc;
	printk(KERN_INFO "[WIMAX] %s on=%d\n", __func__,on);

	if (on)	{
		gpio_set_value(VALENTE_WX_WIMAX_EXT_RSTz,0);

		pm8xxx_gpio_config(wimax_pvdd_pmgpios[1].gpio, &wimax_pvdd_pmgpios[1].config);
		msleep(10);
		pm8xxx_gpio_config(wimax_dvdd_pmgpios[1].gpio, &wimax_dvdd_pmgpios[1].config);
		msleep(3);
		pm8xxx_gpio_config(wimax_1v2_pmgpios[1].gpio, &wimax_1v2_pmgpios[1].config);
		msleep(130);
		pm8xxx_gpio_config(wimax_sleep_clk_pmgpios[1].gpio, &wimax_sleep_clk_pmgpios[1].config);

		for (i = 0; i < ARRAY_SIZE(wimax_on_gpio_table); i++) {
			rc = gpio_tlmm_config(wimax_on_gpio_table[i], GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, wimax_on_gpio_table[i], rc);
			}
		}

		msleep(3);
		gpio_tlmm_config(wimax_reset_table[1], GPIO_CFG_ENABLE);
		gpio_set_value(VALENTE_WX_WIMAX_EXT_RSTz,1);

	} else {
		/*Power OFF sequence*/
		gpio_tlmm_config(wimax_reset_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(VALENTE_WX_WIMAX_EXT_RSTz,0);

		for (i = 0; i < ARRAY_SIZE(wimax_off_gpio_table); i++) {
			rc = gpio_tlmm_config(wimax_off_gpio_table[i], GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, wimax_off_gpio_table[i], rc);
			}
		}
		mmc_wimax_initial_gpio_set_value();

		msleep(5);
		pm8xxx_gpio_config(wimax_sleep_clk_pmgpios[0].gpio, &wimax_sleep_clk_pmgpios[0].config);
		pm8xxx_gpio_config(wimax_1v2_pmgpios[0].gpio, &wimax_1v2_pmgpios[0].config);
		msleep(3);
		pm8xxx_gpio_config(wimax_dvdd_pmgpios[0].gpio, &wimax_dvdd_pmgpios[0].config);
		msleep(3);
		pm8xxx_gpio_config(wimax_pvdd_pmgpios[0].gpio, &wimax_pvdd_pmgpios[0].config);
	}

	return 0;
}
EXPORT_SYMBOL(mmc_wimax_power);

/* UART */

uint32_t usbuart_pin_enable_usb_table[] = {
	GPIO_CFG(VALENTE_WX_GPIO_MHL_USB_ENz , 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

uint32_t usbuart_pin_enable_uart_table[] = {
	GPIO_CFG(VALENTE_WX_GPIO_MHL_USB_ENz , 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

void valente_wx_usb_uart_switch(int nvbus)
{
	printk(KERN_INFO "%s: %s, rev=%d\n", __func__, nvbus ? "uart" : "usb", system_rev);
	if(nvbus == 1) { /* vbus gone, pin pull up */
		gpio_tlmm_config(usbuart_pin_enable_uart_table[0], GPIO_CFG_ENABLE);
	} else {	/* vbus present, pin pull low */
		gpio_tlmm_config(usbuart_pin_enable_usb_table[0], GPIO_CFG_ENABLE);
	}
}

int wimax_uart_switch = 0;
int mmc_wimax_uart_switch(int uart)
{
	printk(KERN_INFO "[WIMAX] %s uart:%d\n", __func__, uart);
	wimax_uart_switch = uart;

    gpio_set_value(VALENTE_WX_CPUz_WIMAX_SW, uart?1:0); /* CPU_WIMAX_SW */
	valente_wx_usb_uart_switch(1);

	return 0;
}
EXPORT_SYMBOL(mmc_wimax_uart_switch);

int mmc_wimax_get_uart_switch(void)
{
	printk(KERN_INFO "[WIMAX] %s uart:%d\n", __func__, wimax_uart_switch);
	return wimax_uart_switch?1:0;
}
EXPORT_SYMBOL(mmc_wimax_get_uart_switch);

/*non-8X60 PROJECT need to use GPIO mapping to decode the IRQ number(id) for PMIC GPIO*/
int mmc_wimax_get_hostwakeup_gpio(void)
{
	return mmc_wimax_hostwakeup_gpio;
}
EXPORT_SYMBOL(mmc_wimax_get_hostwakeup_gpio);

/*8X60 PROJECT need to use Marco PM8058_GPIO_IRQ to decode the IRQ number(id) for PMIC GPIO*/
int mmc_wimax_get_hostwakeup_IRQ_ID(void)
{
	return PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VALENTE_WX_FATAL_ERROR_IND);
}
EXPORT_SYMBOL(mmc_wimax_get_hostwakeup_IRQ_ID);

void mmc_wimax_enable_host_wakeup(int on)
{
	if (mmc_wimax_get_status()) {
		if (on) {
			if (!mmc_wimax_get_gpio_irq_enabled()) {
				if (printk_ratelimit())
					printk(KERN_INFO "[WIMAX] set PMIC GPIO%d as wakeup source on IRQ %d\n", VALENTE_WX_FATAL_ERROR_IND+1, mmc_wimax_get_hostwakeup_IRQ_ID());
				enable_irq(mmc_wimax_get_hostwakeup_IRQ_ID());
				enable_irq_wake(mmc_wimax_get_hostwakeup_IRQ_ID());
				mmc_wimax_set_gpio_irq_enabled(1);
			}
		} else {
			if (mmc_wimax_get_gpio_irq_enabled()) {
				if (printk_ratelimit())
					printk(KERN_INFO "[WIMAX] disable PMIC GPIO%d wakeup source\n", VALENTE_WX_FATAL_ERROR_IND+1);
				disable_irq_wake(mmc_wimax_get_hostwakeup_IRQ_ID());
				disable_irq_nosync(mmc_wimax_get_hostwakeup_IRQ_ID());
				mmc_wimax_set_gpio_irq_enabled(0);
			}
		}
	} else
		printk(KERN_INFO "[WIMAX] %s mmc_wimax_sdio_status is OFF\n", __func__);
}
EXPORT_SYMBOL(mmc_wimax_enable_host_wakeup);

#endif

int __init valente_wx_init_mmc()
{
#ifdef CONFIG_WIMAX
	int i, rc;

	printk(KERN_INFO "valente_wx: %s\n", __func__);

	/* SDC2: WiMAX */
	/* PM QoS for wimax */
    //mmc_wimax_data.swfi_latency = msm_rpm_get_swfi_latency();
	msm_add_sdcc(2, &mmc_wimax_data);

	for (i = 0; i < ARRAY_SIZE(wimax_init_pmgpios); i++) {
		rc = pm8xxx_gpio_config(wimax_init_pmgpios[i].gpio,
					&wimax_init_pmgpios[i].config);
		if (rc)
			printk("%s: wimax_init_pmgpios: rc=%d\n", __func__, rc);
	}

	/* Initialized WiMAX pins */
	for (i = 0; i < ARRAY_SIZE(wimax_initial_gpio_table); i++) {
		rc = gpio_tlmm_config(wimax_initial_gpio_table[i], GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				   "%s: gpio_tlmm_config(%#x)=%d\n",
				   __func__, wimax_initial_gpio_table[i], rc);
		}
	}

	mmc_wimax_initial_gpio_set_value();
#endif

	return 0;
}
