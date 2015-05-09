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

#include <linux/interrupt.h>
#include <linux/mfd/pm8xxx/pm8038.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/msm_ssbi.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/restart.h>
#include "devices.h"
#include "board-8930.h"
#include "board-zara_cl.h"

void pm8xxx_adc_device_driver_register(void);

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
	.gpio	= PM8038_GPIO_PM_TO_SYS(_gpio), \
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
	.mpp	= PM8038_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM8038_GPIO_VIN_L11, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM8038_GPIO_VIN_L11, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM8038_GPIO_VIN_L11, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM8038_GPIO_VIN_L11, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN(_gpio, _val, _vin) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, _vin, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* Initial pm8038 GPIO configurations */
static struct pm8xxx_gpio_init pm8038_gpios[] __initdata = {
	/* keys GPIOs */

	PM8XXX_GPIO_OUTPUT_FUNC(1, 0, PM_GPIO_FUNC_1),
	PM8XXX_GPIO_OUTPUT_FUNC(2, 0, PM_GPIO_FUNC_1),
};

/* Initial pm8038 MPP configurations */
static struct pm8xxx_mpp_init pm8038_mpps[] __initdata = {
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8XXX_MPP_INIT(3, D_INPUT, PM8038_MPP_DIG_LEVEL_VPH, DIN_TO_INT),
};

void __init msm8930_pm8038_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8038_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8038_gpios[i].gpio,
					&pm8038_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	/* Initial MPP configuration. */
	for (i = 0; i < ARRAY_SIZE(pm8038_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8038_mpps[i].mpp,
					&pm8038_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

static struct pm8xxx_adc_amux pm8xxx_adc_channels_data[] = {
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
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV2,
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
	{"pa_therm1", ADC_MPP_1_AMUX4, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"mpp_amux6", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties pm8xxx_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static const struct pm8xxx_adc_map_pt zara_cl_adcmap_btm_threshold[] = {
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

static const struct pm8xxx_adc_map_pt zara_cl_sprint_adcmap_btm_threshold[] = {
	{-200,	1670},
	{-190,	1662},
	{-180,	1654},
	{-170,	1645},
	{-160,	1636},
	{-150,	1626},
	{-140,	1616},
	{-130,	1606},
	{-120,	1595},
	{-110,	1583},
	{-100,	1571},
	{-90,	1559},
	{-80,	1546},
	{-70,	1533},
	{-60,	1519},
	{-50,	1505},
	{-40,	1491},
	{-30,	1476},
	{-20,	1460},
	{-10,	1444},
	{-0,	1428},
	{10,	1411},
	{20,	1393},
	{30,	1376},
	{40,	1358},
	{50,	1339},
	{60,	1321},
	{70,	1301},
	{80,	1282},
	{90,	1262},
	{100,	1242},
	{110,	1222},
	{120,	1202},
	{130,	1181},
	{140,	1161},
	{150,	1140},
	{160,	1119},
	{170,	1097},
	{180,	1077},
	{190,	1055},
	{200,	1034},
	{210,	1013},
	{220,	992},
	{230,	971},
	{240,	950},
	{250,	930},
	{260,	909},
	{270,	889},
	{280,	869},
	{290,	849},
	{300,	829},
	{310,	810},
	{320,	790},
	{330,	772},
	{340,	753},
	{350,	735},
	{360,	717},
	{370,	700},
	{380,	683},
	{390,	666},
	{400,	649},
	{410,	633},
	{420,	618},
	{430,	602},
	{440,	587},
	{450,	573},
	{460,	559},
	{470,	545},
	{480,	531},
	{490,	518},
	{500,	506},
	{510,	493},
	{520,	481},
	{530,	470},
	{540,	458},
	{550,	447},
	{560,	437},
	{570,	426},
	{580,	416},
	{590,	406},
	{600,	397},
	{610,	388},
	{620,	379},
	{630,	371},
	{640,	362},
	{650,	354},
	{660,	347},
	{670,	339},
	{680,	332},
	{690,	325},
	{700,	318},
	{710,	312},
	{720,	306},
	{730,	299},
	{740,	294},
	{750,	288},
	{760,	282},
	{770,	277},
	{780,	272},
	{790,	267}
};

/* if board file doesn't assign map table, default one will be used */
static struct pm8xxx_adc_map_table pm8xxx_adcmap_btm_table = {
	.table = zara_cl_adcmap_btm_threshold,
	.size = ARRAY_SIZE(zara_cl_adcmap_btm_threshold),
};

static struct pm8xxx_adc_platform_data pm8xxx_adc_pdata = {
	.adc_channel            = pm8xxx_adc_channels_data,
	.adc_num_board_channel  = ARRAY_SIZE(pm8xxx_adc_channels_data),
	.adc_prop               = &pm8xxx_adc_data,
	.adc_mpp_base		= PM8038_MPP_PM_TO_SYS(1),
	.adc_map_btm_table	= &pm8xxx_adcmap_btm_table,
	.pm8xxx_adc_device_register	= pm8xxx_adc_device_driver_register,
};

static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata __devinitdata = {
	.irq_base		= PM8038_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(104),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata __devinitdata = {
	.gpio_base	= PM8038_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata __devinitdata = {
	.mpp_base	= PM8038_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_rtc_platform_data pm8xxx_rtc_pdata __devinitdata = {
	.rtc_write_enable	= true,
#ifdef CONFIG_HTC_OFFMODE_ALARM
	.rtc_alarm_powerup      = true,
#else
	.rtc_alarm_powerup      = false,
#endif
};

static struct pm8xxx_pwrkey_platform_data pm8xxx_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 15625,
	.wakeup			= 1,
};

static int pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	225,
};

#define MAX_VOLTAGE_MV		4340
static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 960,
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 50,
	.term_current		= 210,
	.cool_temp		= 0,
	.warm_temp		= 48,
	.temp_check_period	= 1,
	.max_bat_chg_current	= 1425,
	.cool_bat_chg_current	= 1125,
	.warm_bat_chg_current	= 1125,
	.cool_bat_voltage	= 4200,
	.warm_bat_voltage	= 4000,
	.mbat_in_gpio		= 94,/* MBAT_IN : 94*/
	.is_embeded_batt	= 0,
	.eoc_ibat_thre_ma	   = 50,
	.ichg_threshold_ua		= -1200000,
	.ichg_regulation_thr_ua 	  = -310000,
	.cable_in_irq           = MSM_GPIO_TO_INT(MSM_OVP_INTz),
	.cable_in_gpio          = MSM_OVP_INTz,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.cold_thr = PM_SMBC_BATT_TEMP_COLD_THR__HIGH,
	.hot_thr = PM_SMBC_BATT_TEMP_HOT_THR__LOW,
	.led_src_config		= LED_SRC_VPH_PWR,
	.rconn_mohm		= 10,
};

#define PM8XXX_LED_PWM_PERIOD		1000
#define PM8XXX_LED_PWM_DUTY_MS		64
#define PM8038_RGB_LED_MAX_CURRENT	12

static struct led_info pm8038_led_info[] = {
	[0] = {
		.name			= "amber",
	},
	[1] = {
		.name			= "green",
	},
	[2] = {
		.name			= "button-backlight",
	},
};

static struct led_platform_data pm8038_led_core_pdata = {
	.num_leds = ARRAY_SIZE(pm8038_led_info),
	.leds = pm8038_led_info,
};

#if defined(CONFIG_MACH_ZARA_CL) || defined(CONFIG_MACH_K2_CL) || defined(CONFIG_MACH_ZARA_WL)
static int pm8038_led0_pwm_duty_pcts[64] = {
			0, 15, 30, 45, 60, 75, 90, 100,
			100, 90, 75, 60, 45, 30, 15, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0

};

static struct pm8xxx_pwm_duty_cycles pm8038_led0_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8038_led0_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8038_led0_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 0,
};
#endif
static struct pm8xxx_led_config pm8038_led_configs[] = {
	[0] = {
		.id = PM8XXX_ID_RGB_LED_RED,
		.mode = PM8XXX_LED_MODE_PWM1,
		.pwm_channel = 5,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_coefficient = 10,
	},
	[1] = {
		.id = PM8XXX_ID_RGB_LED_GREEN,
		.mode = PM8XXX_LED_MODE_PWM1,
		.pwm_channel = 4,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_coefficient = 7,
	},
	[2] = {
		.id = PM8XXX_ID_RGB_LED_BLUE,
		.mode = PM8XXX_LED_MODE_PWM1,
		.max_current = PM8038_RGB_LED_MAX_CURRENT,
		.pwm_channel = 0,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_duty_cycles = &pm8038_led0_pwm_duty_cycles,
	},
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata = {
	.led_core = &pm8038_led_core_pdata,
	.configs = pm8038_led_configs,
	.num_configs = ARRAY_SIZE(pm8038_led_configs),
};

static struct pm8xxx_ccadc_platform_data pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms		= 600000,
};

static struct pm8xxx_misc_platform_data pm8xxx_misc_pdata = {
	.priority		= 0,
};

static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.r_sense		= 10,
	.i_test			= 2000,
	.v_failure		= 3000,
	.max_voltage_uv		= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm		= 0,
	.criteria_sw_est_ocv = 86400000, /*24 hour*/
	.rconn_mohm_sw_est_ocv = 10,
};

static struct pm8xxx_vibrator_platform_data pm8xxx_vib_pdata = {
	.initial_vibrate_ms = 0,
	.max_timeout_ms = 15000,
	.level_mV = 3100,
};

static struct pm8038_platform_data pm8038_platform_data __devinitdata = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.rtc_pdata              = &pm8xxx_rtc_pdata,
	.pwrkey_pdata		= &pm8xxx_pwrkey_pdata,
	.misc_pdata		= &pm8xxx_misc_pdata,
	.regulator_pdatas	= msm8930_pm8038_regulator_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.adc_pdata		= &pm8xxx_adc_pdata,
	.leds_pdata		= &pm8xxx_leds_pdata,
	.vibrator_pdata         = &pm8xxx_vib_pdata,
	.ccadc_pdata		= &pm8xxx_ccadc_pdata,
};

static struct msm_ssbi_platform_data msm8930_ssbi_pm8038_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8038-core",
		.platform_data		= &pm8038_platform_data,
	},
};

static int __init check_cid_setup(char *str)
{
	if (!strncmp(str, "SPCS", 4)) {
		pm8xxx_adcmap_btm_table.table
			= zara_cl_sprint_adcmap_btm_threshold;
		pm8xxx_adcmap_btm_table.size
			= ARRAY_SIZE(zara_cl_sprint_adcmap_btm_threshold);
	}
	return 1;
}
__setup("androidboot.cid=", check_cid_setup);

void __init msm8930_init_pmic(void)
{
	pmic_reset_irq = PM8038_IRQ_BASE + PM8038_RESOUT_IRQ;
	msm8960_device_ssbi_pmic.dev.platform_data =
				&msm8930_ssbi_pm8038_pdata;
	pm8038_platform_data.num_regulators
		= msm8930_pm8038_regulator_pdata_len;
}
