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
//include "board-dlp_china.h"
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

static int dlp_china_get_hw_component(void)
{
    int hw_com = 0;

    hw_com |= HTC_AUDIO_RT5501;
    return hw_com;
}

/*
 * Original Dual membrane (snd_soc_msm_2x_Fusion3_DMIC)
 *    DLP#DWG (PO68300) - 00037500
 *    DLP#DTU (PO68400) - 00037501
 *    DLP#DUG (PO68500) - 00037502
 *
 * ST Single Membrane (snd_soc_msm_2x_Fusion3_DMIC_S)
 *    DLP#DWG (PO68300) - 00038F00
 *    DLP#DTU (PO68400) - 00038F01
 *    DLP#DUG (PO68500) - 00038F02
 *
 * Knowles Single Membrane (snd_soc_msm_2x_Fusion3_DMIC_SK)
 *    DLP#DWG (PO68300) - 00039C00
 *    DLP#DTU (PO68400) - 00039C01
 *    DLP#DUG (PO68500) - 00039C02
 *
 */

static int dlp_china_enable_digital_mic(void)
{
	int ret;
	if ((skuid & 0xFFF00) == 0x37500) /* Original Dual membrane */
		ret = 1;
	else if ((skuid & 0xFFF00) == 0x38F00) /* ST Single Membrane */
		ret = 2;
	else if ((skuid & 0xFFF00) == 0x39C00) /* Knowles Single Membrane */
		ret = 3;
	else /* Original Dual membrane */
		ret = 1;
	printk(KERN_INFO "dlp_china_enable_digital_mic[check dmic type]:skuid=0x%x, system_rev=%x return %d\n", skuid, system_rev,ret);
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

static int dlp_china_aud_speaker_vdd_enable(char *power_vreg_name, unsigned volt)
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
        .enable_digital_mic = dlp_china_enable_digital_mic,
        .get_hw_component = dlp_china_get_hw_component,
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

static int __init dlp_china_audio_init(void)
{
        int ret = 0;

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);
/* Due to this file is common for all 8064 china, we add flag to divide projects */
/* DLP#China use differnet regulator source for VREG_SPK_1V8 */
	if (system_rev >= XB) /* VREG_SPK_1V8 use LVS2 for board after XB (include XB) */
		dlp_china_aud_speaker_vdd_enable("tfa9887_vdd_LVS2", 1800000);
	else /* VREG_SPK_1V8 use L23 for board before XB */
		dlp_china_aud_speaker_vdd_enable("tfa9887_vdd_L23", 1800000);
	pr_info("%s", __func__);
	return ret;

}
late_initcall(dlp_china_audio_init);

static void __exit dlp_china_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(dlp_china_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
