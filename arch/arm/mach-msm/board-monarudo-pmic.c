/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <asm/setup.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/restart.h>
#include "devices.h"
#include "board-monarudo.h"

void monarudo_pm8xxx_adc_device_register(void);

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8921_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
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

#define PM8921_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8821_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8821_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8921_GPIO_DISABLE(_gpio) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8921_GPIO_OUTPUT(_gpio, _val, _strength) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_BUFCONF(_gpio, _val, _strength, _bufconf) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT,\
			PM_GPIO_OUT_BUF_##_bufconf, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_INPUT(_gpio, _pull) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8921_GPIO_OUTPUT_VIN(_gpio, _val, _vin) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, _vin, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* Initial PM8921 GPIO configurations. Modify the structure need to inform kernel team*/
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {
	PM8921_GPIO_OUTPUT(14, 0, HIGH),	/* default uart path*/
	PM8921_GPIO_OUTPUT(15, 0, HIGH),
};

static struct pm8xxx_gpio_init pm8921_cdp_kp_gpios[] __initdata = {
	//PM8921_GPIO_INPUT(37, PM_GPIO_PULL_UP_1P5),
};

static struct pm8xxx_gpio_init pm8921_amp_gpios[] __initdata = {
	PM8921_GPIO_OUTPUT(4, 1, MED),
};

/* Initial PM8XXX MPP configurations */
static struct pm8xxx_mpp_init pm8xxx_mpps[] __initdata = {
	PM8921_MPP_INIT(3, D_OUTPUT, PM8921_MPP_DIG_LEVEL_VPH, DOUT_CTRL_LOW),
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8921_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8921_MPP_INIT(PM8XXX_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH5, AOUT_CTRL_DISABLE),
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	PM8921_MPP_INIT(PM8XXX_AMUX_MPP_1, D_BI_DIR, PM8921_MPP_DIG_LEVEL_S4, BI_PULLUP_10KOHM),
	PM8921_MPP_INIT(PM8XXX_AMUX_MPP_2, D_BI_DIR, PM8921_MPP_DIG_LEVEL_L17, BI_PULLUP_10KOHM),
	PM8921_MPP_INIT(PM8XXX_AMUX_MPP_3, D_BI_DIR, PM8921_MPP_DIG_LEVEL_S4, BI_PULLUP_10KOHM),
	PM8921_MPP_INIT(PM8XXX_AMUX_MPP_4, D_BI_DIR, PM8921_MPP_DIG_LEVEL_L17, BI_PULLUP_10KOHM),
#endif
};

static struct pm8xxx_gpio_init pm8921_gpios_uart_path[]  = {
	PM8921_GPIO_OUTPUT(14, 0, HIGH),	/* output low*/
	PM8921_GPIO_OUTPUT(15, 0, HIGH),	/* output low*/
};

static struct pm8xxx_gpio_init pm8921_gpios_usb_path[]  = {
	PM8921_GPIO_OUTPUT(14, 1, HIGH),	/* output high*/
	PM8921_GPIO_OUTPUT(15, 0, HIGH),	/* output low*/
};

void monarudo_usb_uart_switch(int nvbus)
{
	printk(KERN_INFO "%s: %s, rev=%d\n", __func__, nvbus ? "uart" : "usb", system_rev);
	if(nvbus == 1) { /* vbus gone */
		pm8xxx_gpio_config(pm8921_gpios_uart_path[0].gpio, &pm8921_gpios_uart_path[0].config);
		pm8xxx_gpio_config(pm8921_gpios_uart_path[1].gpio, &pm8921_gpios_uart_path[1].config);
	} else {	/* vbus present, pin pull low */
		pm8xxx_gpio_config(pm8921_gpios_usb_path[0].gpio, &pm8921_gpios_usb_path[0].config);
		pm8xxx_gpio_config(pm8921_gpios_usb_path[1].gpio, &pm8921_gpios_usb_path[1].config);
	}
}


void __init monarudo_pm8xxx_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8921_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pm8921_cdp_kp_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_cdp_kp_gpios[i].gpio,
					&pm8921_cdp_kp_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n",
				__func__, rc);
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pm8xxx_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8xxx_mpps[i].mpp,
					&pm8xxx_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
	if (system_rev >= XB)
		pm8xxx_gpio_config(pm8921_amp_gpios[0].gpio, &pm8921_amp_gpios[0].config);
}

static struct pm8xxx_pwrkey_platform_data monarudo_pm8921_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 15625,
	.wakeup			= 1,
};

static struct pm8xxx_misc_platform_data monarudo_pm8921_misc_pdata = {
	.priority		= 0,
};

#define PM8921_LC_LED_MAX_CURRENT	4	/* I = 4mA */
#define PM8921_LC_LED_LOW_CURRENT	1	/* I = 1mA */
#define PM8XXX_LED_PWM_PERIOD		1000
#define PM8XXX_LED_PWM_DUTY_MS		20
/**
 * PM8XXX_PWM_CHANNEL_NONE shall be used when LED shall not be
 * driven using PWM feature.
 */
#define PM8XXX_PWM_CHANNEL_NONE		-1

static struct pm8xxx_gpio_init green_gpios[] = {
	PM8921_GPIO_INIT(GREEN_BACK_LED_XC_XD, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
				PM_GPIO_STRENGTH_HIGH, \
				PM_GPIO_FUNC_2, 0, 0),
	PM8921_GPIO_INIT(GREEN_BACK_LED_XC_XD, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
				PM_GPIO_STRENGTH_HIGH, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
};

static struct pm8xxx_gpio_init amber_gpios[] = {
	PM8921_GPIO_INIT(AMBER_BACK_LED_XC_XD, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
				PM_GPIO_STRENGTH_HIGH, \
				PM_GPIO_FUNC_2, 0, 0),
	PM8921_GPIO_INIT(AMBER_BACK_LED_XC_XD, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
				PM_GPIO_STRENGTH_HIGH, \
				PM_GPIO_FUNC_NORMAL, 0, 0),

};

static void green_gpio_config(bool enable)
{
		if (enable)
			pm8xxx_gpio_config(green_gpios[0].gpio, &green_gpios[0].config);
		else
			pm8xxx_gpio_config(green_gpios[1].gpio, &green_gpios[1].config);
}

static void amber_gpio_config(bool enable)
{
		if (enable)
			pm8xxx_gpio_config(amber_gpios[0].gpio, &amber_gpios[0].config);
		else
			pm8xxx_gpio_config(amber_gpios[1].gpio, &amber_gpios[1].config);
}

static struct pm8xxx_led_configure pm8921_led_info[] = {
	[0] = {
		.name		= "button-backlight",
		.flags		= PM8XXX_ID_LED_0,
		.function_flags = LED_PWM_FUNCTION | LED_BRETH_FUNCTION,
		.period_us 	= USEC_PER_SEC / 1000,
		.start_index 	= 0,
		.duites_size 	= 8,
		.duty_time_ms 	= 64,
		.lut_flag 	= PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_PAUSE_HI_EN,
		.out_current    = 3,
		.duties		= {0, 15, 30, 45, 60, 75, 90, 100,
				100, 90, 75, 60, 45, 30, 15, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0},
	},
	[1] = {
		.name           = "green",
		.flags		= PM8XXX_ID_GPIO24,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = green_gpio_config,
		.led_sync		= 1,
	},
	[2] = {
		.name           = "amber",
		.flags		= PM8XXX_ID_GPIO25,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = amber_gpio_config,
		.led_sync		= 1,
	},
	[3] = {
		.name		= "green-back",
		.flags		= PM8XXX_ID_LED_1,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.out_current	= 2,
	},
	[4] = {
		.name		= "amber-back",
		.flags		= PM8XXX_ID_LED_2,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.out_current	= 2,
	},
};

static struct pm8xxx_led_platform_data apq8064_pm8921_leds_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};



static struct pm8xxx_adc_amux monarudo_pm8921_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"mpp_amux6", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties monarudo_pm8921_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};
static const struct pm8xxx_adc_map_pt monarudo_adcmap_btm_table[] = {
	{-200,	1671},
	{-190,	1663},
	{-180,	1654},
	{-170,	1646},
	{-160,	1636},
	{-150,	1627},
	{-140,	1617},
	{-130,	1606},
	{-120,	1595},
	{-110,	1584},
	{-100,	1572},
	{-90,	1560},
	{-80,	1548},
	{-70,	1534},
	{-60,	1521},
	{-50,	1507},
	{-40,	1492},
	{-30,	1477},
	{-20,	1462},
	{-10,	1446},
	{-0,	1430},
	{10,	1413},
	{20,	1396},
	{30,	1379},
	{40,	1361},
	{50,	1343},
	{60,	1325},
	{70,	1306},
	{80,	1287},
	{90,	1267},
	{100,	1248},
	{110,	1228},
	{120,	1208},
	{130,	1188},
	{140,	1168},
	{150,	1147},
	{160,	1127},
	{170,	1106},
	{180,	1086},
	{190,	1065},
	{200,	1044},
	{210,	1024},
	{220,	1004},
	{230,	983},
	{240,	963},
	{250,	943},
	{260,	923},
	{270,	903},
	{280,	884},
	{290,	864},
	{300,	845},
	{310,	827},
	{320,	808},
	{330,	790},
	{340,	772},
	{350,	755},
	{360,	738},
	{370,	721},
	{380,	704},
	{390,	688},
	{400,	672},
	{410,	657},
	{420,	642},
	{430,	627},
	{440,	613},
	{450,	599},
	{460,	585},
	{470,	572},
	{480,	559},
	{490,	547},
	{500,	535},
	{510,	523},
	{520,	511},
	{530,	500},
	{540,	489},
	{550,	479},
	{560,	469},
	{570,	459},
	{580,	449},
	{590,	440},
	{600,	431},
	{610,	423},
	{620,	414},
	{630,	406},
	{640,	398},
	{650,	390},
	{660,	383},
	{670,	376},
	{680,	369},
	{690,	363},
	{700,	356},
	{710,	350},
	{720,	344},
	{730,	338},
	{740,	333},
	{750,	327},
	{760,	322},
	{770,	317},
	{780,	312},
	{790,	308}
};

static struct pm8xxx_adc_map_table pm8xxx_adcmap_btm_table = {
	.table = monarudo_adcmap_btm_table,
	.size = ARRAY_SIZE(monarudo_adcmap_btm_table),
};

static struct pm8xxx_adc_platform_data monarudo_pm8921_adc_pdata = {
	.adc_channel		= monarudo_pm8921_adc_channels_data,
	.adc_num_board_channel	= ARRAY_SIZE(monarudo_pm8921_adc_channels_data),
	.adc_prop		= &monarudo_pm8921_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
	.adc_map_btm_table	= &pm8xxx_adcmap_btm_table,
	.pm8xxx_adc_device_register	= monarudo_pm8xxx_adc_device_register,
};

static struct pm8xxx_mpp_platform_data
monarudo_pm8921_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_gpio_platform_data
monarudo_pm8921_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_irq_platform_data
monarudo_pm8921_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8921_APC_USR_IRQ_N),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
	.dev_id			= 0,
};

static struct pm8xxx_rtc_platform_data
monarudo_pm8921_rtc_pdata = {
	.rtc_write_enable       = true,
#ifdef CONFIG_HTC_OFFMODE_ALARM
	.rtc_alarm_powerup      = true,
#else
	.rtc_alarm_powerup      = false,
#endif
};

static int monarudo_pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	225,
};

#define MAX_VOLTAGE_MV          4200
static struct pm8921_charger_platform_data
pm8921_chg_pdata __devinitdata = {
	.safety_time		= 960,
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 50,
	.term_current		= 75,
	.cool_temp		= 0,
	.warm_temp		= 48,
	.temp_check_period	= 1,
	.dc_unplug_check	= true,
	.max_bat_chg_current	= 1025,
	.cool_bat_chg_current	= 1025,
	.warm_bat_chg_current	= 1025,
	.cool_bat_voltage	= 4200,
	.warm_bat_voltage	= 4000,
	.mbat_in_gpio		= 0, /* No MBAT_IN*/
	.wlc_tx_gpio		= 0, /* Used to detect if WLC pad is existed */
	.is_embeded_batt	= 1,
	.vin_min_wlc		= 4800, /* only for WLC in DLX_WL */
	.thermal_mitigation	= monarudo_pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(monarudo_pm8921_therm_mitigation),
	.cold_thr = PM_SMBC_BATT_TEMP_COLD_THR__HIGH,
	.hot_thr = PM_SMBC_BATT_TEMP_HOT_THR__LOW,
};

static struct pm8xxx_ccadc_platform_data
monarudo_pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms		= 600000,
};

static struct pm8921_bms_platform_data
pm8921_bms_pdata __devinitdata = {
	.r_sense		= 10,
	.i_test			= 2000,
	.v_failure		= 3000,
	.max_voltage_uv		= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm		= 0,
};

static int __init check_dq_setup(char *str)
{
	if (!strcmp(str, "PASS")) {
		pr_info("[BATT] overwrite HV battery config\n");
		pm8921_chg_pdata.max_voltage = 4340;
		pm8921_chg_pdata.cool_bat_voltage = 4340;
		pm8921_bms_pdata.max_voltage_uv = 4340 * 1000;
	} else {
		pr_info("[BATT] use default battery config\n");
		pm8921_chg_pdata.max_voltage = 4200;
		pm8921_chg_pdata.cool_bat_voltage = 4200;
		pm8921_bms_pdata.max_voltage_uv = 4200 * 1000;
	}
	return 1;
}
__setup("androidboot.dq=", check_dq_setup);

static struct pm8921_platform_data
monarudo_pm8921_platform_data __devinitdata = {
	.regulator_pdatas	= monarudo_pm8921_regulator_pdata,
	.irq_pdata		= &monarudo_pm8921_irq_pdata,
	.gpio_pdata		= &monarudo_pm8921_gpio_pdata,
	.mpp_pdata		= &monarudo_pm8921_mpp_pdata,
	.rtc_pdata		= &monarudo_pm8921_rtc_pdata,
	.pwrkey_pdata	= &monarudo_pm8921_pwrkey_pdata,
	.leds_pdata		= &apq8064_pm8921_leds_pdata,
	.misc_pdata		= &monarudo_pm8921_misc_pdata,
#if 0
	.leds_pdata		= &monarudo_pm8921_leds_pdata,
#endif
	.adc_pdata		= &monarudo_pm8921_adc_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.ccadc_pdata		= &monarudo_pm8xxx_ccadc_pdata,
};

static struct pm8xxx_irq_platform_data
monarudo_pm8821_irq_pdata __devinitdata = {
	.irq_base		= PM8821_IRQ_BASE,
	.devirq			= PM8821_SEC_IRQ_N,
	.irq_trigger_flag	= IRQF_TRIGGER_HIGH,
	.dev_id			= 1,
};

static struct pm8xxx_mpp_platform_data
monarudo_pm8821_mpp_pdata __devinitdata = {
	.mpp_base	= PM8821_MPP_PM_TO_SYS(1),
};

static struct pm8821_platform_data
monarudo_pm8821_platform_data __devinitdata = {
	.irq_pdata	= &monarudo_pm8821_irq_pdata,
	.mpp_pdata	= &monarudo_pm8821_mpp_pdata,
};

static struct msm_ssbi_platform_data monarudo_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8921-core",
		.platform_data	= &monarudo_pm8921_platform_data,
	},
};

static struct msm_ssbi_platform_data monarudo_ssbi_pm8821_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8821-core",
		.platform_data	= &monarudo_pm8821_platform_data,
	},
};

void __init monarudo_init_pmic(void)
{
	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;

	if (system_rev <= XC) {
		/* Adjust r_sense to get correct value of current &
		 * Magnify ibat_max & ibat_safe (bootloader) to
		 * cover wrong higher current than real one */
		pm8921_bms_pdata.r_sense = 10/0.754;
		monarudo_pm8xxx_ccadc_pdata.r_sense = 10/0.754;
		pm8921_chg_pdata.max_bat_chg_current = 1400;
		pm8921_chg_pdata.cool_bat_chg_current = 1400;
		pm8921_chg_pdata.warm_bat_chg_current = 1400;
	}
	if (system_rev >= PVT)
		pm8921_chg_pdata.wlc_tx_gpio = PM8921_GPIO_PM_TO_SYS(WC_TX_WPGz_PVT);
	apq8064_device_ssbi_pmic1.dev.platform_data =
						&monarudo_ssbi_pm8921_pdata;
	apq8064_device_ssbi_pmic2.dev.platform_data =
				&monarudo_ssbi_pm8821_pdata;
	monarudo_pm8921_platform_data.num_regulators =
					monarudo_pm8921_regulator_pdata_len;

}
