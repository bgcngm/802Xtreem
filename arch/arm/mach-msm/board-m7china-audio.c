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
//include "board-m7china.h"
#if defined(CONFIG_MACH_M7_DCG)
#include "board-m7dcg.h"
#elif defined(CONFIG_MACH_M7_DTU)
#include "board-m7dtu.h"
#elif defined(CONFIG_MACH_M7_DUG)
#include "board-m7dug.h"
#elif defined(CONFIG_MACH_M7C_DTU)
#include "board-m7cdtu.h"
#elif defined(CONFIG_MACH_M7C_DUG)
#include "board-m7cdug.h"
#elif defined(CONFIG_MACH_M7C_DWG)
#include "board-m7cdwg.h"
#elif defined(CONFIG_MACH_DLP_DTU)
#include "board-dlp_dtu.h"
#elif defined(CONFIG_MACH_DLP_DUG)
#include "board-dlp_dug.h"
#elif defined(CONFIG_MACH_DLP_DWG)
#include "board-dlp_dwg.h"
#elif defined(CONFIG_MACH_T6_TL)
#include "board-t6tl.h"
#elif defined(CONFIG_MACH_T6_DUG)
#include "board-t6dug.h"
#elif defined(CONFIG_MACH_T6_DWG)
#include "board-t6dwg.h"
#endif


static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static int m7china_get_hw_component(void)
{
    int hw_com = 0;

    hw_com |= HTC_AUDIO_RT5501;
    return hw_com;
}

static int m7china_enable_digital_mic(void)
{
#if defined(CONFIG_MACH_M7_DCG) || defined(CONFIG_MACH_M7_DTU) || defined(CONFIG_MACH_M7_DUG)
	printk(KERN_INFO "m7china_enable_digital_mic:skuid=0x%x, system_rev=%x\n", skuid, system_rev);
	if ((system_rev == XA) || (system_rev == XB)){
		if (((skuid & 0xFF) == 0x06) ||
			((skuid & 0xFF) == 0x07) ||
			((skuid & 0xFF) == 0x08) ||
			((skuid & 0xFF) == 0x09) ||
			((skuid & 0xFF) == 0x0A) ||
			((skuid & 0xFF) == 0x0B)) {
			printk(KERN_INFO "(skuid & 0xFF) == %x\n", (skuid & 0xFF));
			return 1;
		}
		printk(KERN_INFO "without DMIC (skuid & 0xFF) == %x\n", (skuid & 0xFF));
	}
	else if (system_rev > XB){
		return 1;
	}
	return 0;

#elif defined(CONFIG_MACH_M7C_DTU) || defined(CONFIG_MACH_M7C_DUG) || defined(CONFIG_MACH_M7C_DWG)
	int ret;
	if ((skuid & 0xFFF00) == 0x36100)
		ret = 1;
	else if ((skuid & 0xFFF00) == 0x38B00)
		ret = 2;
	else
		ret = 3;
	printk(KERN_INFO "m7china_enable_digital_mic[m7c sku, check dmic type]:skuid=0x%x, system_rev=%x return %d\n", skuid, system_rev,ret);
	return ret;
#else
	printk(KERN_INFO "m7china_enable_digital_mic[not m7c sku, always dmic 1]:skuid=0x%x, system_rev=%x\n", skuid, system_rev);
	return 1;
#endif
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
* For M7 China sku, such as DUG/DCG/DTU, NXP tfa9887's power source has been
* changed to PMIC L23 (VREG_SPK_1V8).
*/
static int m7china_aud_speaker_vdd_enable(char *power_vreg_name, unsigned volt)
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

#if defined(CONFIG_MACH_DLP_DTU) || defined(CONFIG_MACH_DLP_DUG) || defined(CONFIG_MACH_DLP_DWG)
	if (system_rev >= XB) { /* VREG_SPK_1V8 use LVS2 for board after XB (include XB) */
		/* LVS don't need to set voltage */
	} else { /* VREG_SPK_1V8 use L23 for board before XB */
		ret = regulator_set_voltage(aud_spk_amp_power, power_vreg_volt, power_vreg_volt);
		if (ret < 0) {
			pr_err("[AUD]%s: unable to set %s voltage to %d rc:%d\n",
					__func__, power_vreg_name, power_vreg_volt, ret);
			regulator_put(aud_spk_amp_power);
			aud_spk_amp_power = NULL;
			return -ENODEV;
		}
	}
#else
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

static struct acoustic_ops acoustic = {
        .enable_digital_mic = m7china_enable_digital_mic,
        .get_hw_component = m7china_get_hw_component,
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

static int __init m7china_audio_init(void)
{
        int ret = 0;

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);
/* Due to this file is common for all 8064 china, we add flag to divide projects */
/* DLP#China use differnet regulator source for VREG_SPK_1V8 */
#if defined(CONFIG_MACH_DLP_DTU) || defined(CONFIG_MACH_DLP_DUG) || defined(CONFIG_MACH_DLP_DWG)
	if (system_rev >= XB) /* VREG_SPK_1V8 use LVS2 for board after XB (include XB) */
		m7china_aud_speaker_vdd_enable("tfa9887_vdd_LVS2", 1800000);
	else /* VREG_SPK_1V8 use L23 for board before XB */
		m7china_aud_speaker_vdd_enable("tfa9887_vdd_L23", 1800000);
#else
	m7china_aud_speaker_vdd_enable("tfa9887_vdd", 1800000);
#endif
	pr_info("%s", __func__);
	return ret;

}
late_initcall(m7china_audio_init);

static void __exit m7china_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(m7china_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
