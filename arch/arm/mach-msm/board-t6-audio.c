/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * HTC: elite machine driver which defines board-specific data
 * Copy from sound/soc/msm/msm8960.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <mach/htc_acoustic_8960.h>
#include <sound/pcm.h>
#include <sound/q6asm.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <mach/tpa6185.h>
#include <mach/rt5501.h>
#include "../sound/soc/msm/msm-pcm-routing.h"
#include "../sound/soc/msm/msm-compr-q6.h"

#if defined(CONFIG_MACH_T6_UL)
#include "board-t6ul.h"
#elif defined(CONFIG_MACH_T6_WL)
#include "board-t6wl.h"
#elif defined(CONFIG_MACH_T6_WHL)
#include "board-t6whl.h"
#elif defined(CONFIG_MACH_T6_UHL)
#include "board-t6uhl.h"
#endif

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static int t6_get_hw_component(void)
{
    int hw_com = 0;

    hw_com |= HTC_AUDIO_RT5501;
    return hw_com;
}

static int t6_enable_digital_mic(void)
{
	int ret = 3; /* Use knowles single membrane MIC for T6 series */
	printk(KERN_INFO "t6_enable_digital_mic[ret: %d]:skuid=0x%x, system_rev=%x\n", ret, skuid, system_rev);
	return ret;
}

void apq8064_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

int apq8064_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_info("%s: mode %d\n", __func__, mode);
	return mode;
}

int apq8064_get_24b_audio(void)
{
	return 1;
}

/*
* For T6 sku, such as DUG/DCG/DTU, NXP tfa9887's power source has been
* changed to PMIC L23 (VREG_SPK_1V8).
* For T6 sku, VREG_SPK_1V8 uses LVS2 instead. And also be noticed, LVS don't
* need to set voltage.
*
*/
static int t6_aud_speaker_vdd_enable(char *power_vreg_name, unsigned volt)
{
	struct regulator *aud_spk_amp_power;
	unsigned power_vreg_volt = volt;
	int ret = 0;

	pr_info("[AUD]%s, power_vreg_name=%s, volt=%u\n", __func__, power_vreg_name, volt);

	if (strlen(power_vreg_name) <= 0) {
		pr_err("[AUD]%s, vreg_name null is an invalid value\n", __func__);
		return -ENODEV;
	}

	if (power_vreg_volt < 0) {
		pr_err("[AUD]%s, power_vreg_volt is an invalid value\n", __func__);
		return -ENODEV;
	}

	aud_spk_amp_power = regulator_get(NULL, power_vreg_name);
	if (IS_ERR(aud_spk_amp_power)) {
		pr_err("[AUD]%s: Unable to get %s\n", __func__, power_vreg_name);
		return -ENODEV;
	}

/* T6 all boards use LVS2 & LVS don't need to set voltage */
#if 0
	ret = regulator_set_voltage(aud_spk_amp_power, power_vreg_volt, power_vreg_volt);
	if (ret < 0) {
		pr_err("[AUD]%s: unable to set %s voltage to %d rc:%d\n",
				__func__, power_vreg_name, power_vreg_volt, ret);
		regulator_put(aud_spk_amp_power);
		aud_spk_amp_power = NULL;
		return -ENODEV;
	}
#endif

	ret = regulator_enable(aud_spk_amp_power);
	if (ret < 0) {
		pr_err("[AUD]%s: Enable regulator %s failed\n", __func__, power_vreg_name);
		regulator_put(aud_spk_amp_power);
		aud_spk_amp_power = NULL;
		return -ENODEV;
	} else {
		pr_info("[AUD]%s: Enable regulator %s OK\n", __func__, power_vreg_name);
	}

	return 0;
}

static int t6_aud_vdd_enable(char *power_vreg_name, unsigned volt)
{
	struct regulator *aud_power;
	unsigned power_vreg_volt = volt;
	int ret = 0;

	pr_info("[AUD]%s, power_vreg_name=%s, volt=%u\n", __func__, power_vreg_name, volt);

	if (strlen(power_vreg_name) <= 0) {
		pr_err("[AUD]%s, vreg_name null is an invalid value\n", __func__);
		return -ENODEV;
	}

	if (power_vreg_volt < 0) {
		pr_err("[AUD]%s, power_vreg_volt is an invalid value\n", __func__);
		return -ENODEV;
	}

	aud_power = regulator_get(NULL, power_vreg_name);
	if (IS_ERR(aud_power)) {
		pr_err("[AUD]%s: Unable to get %s\n", __func__, power_vreg_name);
		return -ENODEV;
	}

	ret = regulator_set_voltage(aud_power, power_vreg_volt, power_vreg_volt);
	if (ret < 0) {
		pr_err("[AUD]%s: unable to set %s voltage to %d rc:%d\n",
				__func__, power_vreg_name, power_vreg_volt, ret);
		regulator_put(aud_power);
		aud_power = NULL;
		return -ENODEV;
	}

	ret = regulator_enable(aud_power);
	if (ret < 0) {
		pr_err("[AUD]%s: Enable regulator %s failed\n", __func__, power_vreg_name);
		regulator_put(aud_power);
		aud_power = NULL;
		return -ENODEV;
	} else {
		pr_info("[AUD]%s: Enable regulator %s OK\n", __func__, power_vreg_name);
	}

	return 0;
}

static struct acoustic_ops acoustic = {
        .enable_digital_mic = t6_enable_digital_mic,
        .get_hw_component = t6_get_hw_component,
	.set_q6_effect = apq8064_set_q6_effect_mode
};

static struct q6asm_ops qops = {
	.get_q6_effect = apq8064_get_q6_effect_mode,
};

static struct msm_pcm_routing_ops rops = {
	.get_q6_effect = apq8064_get_q6_effect_mode,
};

static struct msm_compr_q6_ops cops = {
	.get_24b_audio = apq8064_get_24b_audio,
};

static int __init t6_audio_init(void)
{
        int ret = 0;

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);
	//All T6 boards use LVS2
	t6_aud_vdd_enable("aud_vdd_L9", 3000000);
	t6_aud_speaker_vdd_enable("tfa9887_vdd_LVS2", 1800000);

	pr_info("%s", __func__);
	return ret;

}
late_initcall(t6_audio_init);

static void __exit t6_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(t6_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
