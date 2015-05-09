/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#include <sound/pcm.h>
#include <sound/q6asm.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <mach/htc_acoustic_8960.h>
#include <mach/htc_audiogpio_8930.h>
#include <mach/tfa9887.h>
#include <mach/rt5501.h>
#include "board-operaul.h"
#include "../../../sound/soc/msm/msm-pcm-routing.h"
#include "../../../sound/soc/msm/msm-compr-q6.h"

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static void opera_amp_speaker(bool en)
{
#ifdef CONFIG_AMP_TFA9887
	set_tfa9887_spkamp(en, 0);
#endif
}

static void opera_amp_receiver(bool en)
{
}

static void opera_amp_headset(bool en)
{
#ifdef CONFIG_AMP_RT5501
	if(query_rt5501()) {
		set_rt5501_amp(en);
	}
#endif
}

static void opera_amp_hac(bool en)
{
}

struct htc_8930_gpio_pdata htc_audio_gpio = {
	.amp_speaker = opera_amp_speaker,
	.amp_receiver = opera_amp_receiver,
	.amp_headset = opera_amp_headset,
	.amp_hac = opera_amp_hac,
	.mi2s_gpio = {
	{
		.gpio_no = MSM_AUD_SPK_I2S_WS,
		.gpio_name = "AUD_SPK_MI2S_WS",
	},
	{
		.gpio_no = MSM_AUD_SPK_I2S_BCLK,
		.gpio_name = "AUD_SPK_MI2S_BCLK",
	},
	{
		.gpio_no = MSM_AUD_SPK_I2S_DI,
		.gpio_name = "AUD_SPK_MI2S_DI",
	},
	{
		.gpio_no = MSM_AUD_SPK_I2S_DO,
		.gpio_name = "AUD_SPK_MI2S_DO",
	},
	},
	.i2s_gpio = {
	{
		.gpio_no = MSM_AUD_FM_I2SBCLK,
		.gpio_name = "AUD_FM_I2S_CLK",
	},
	{
		.gpio_no = MSM_AUD_FM_I2SSYNC,
		.gpio_name = "AUD_FM_I2S_WS",
	},
	{
		.gpio_no = MSM_AUD_FM_I2SDIN,
		.gpio_name = "AUD_FM_I2S_DI",
	},
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_D0",
	},
	},
	.aux_pcm_gpio = {
	{
		.gpio_no = MSM_AUD_BTPCM_IN,
		.gpio_name = "AUD_AUX_PCM_DIN",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_OUT,
		.gpio_name = "AUD_AUX_PCM_DOUT",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_SYNC,
		.gpio_name = "AUD_AUX_PCM_SYNC",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_CLK,
		.gpio_name = "AUD_AUX_PCM_CLK",
	},
	},
};
EXPORT_SYMBOL_GPL(htc_audio_gpio);

static int opera_get_hw_revision(void)
{
	int audio_hw_rev;
    int XF = 5;
    printk(KERN_INFO "[XF Only] opera_get_hw_revision: system_rev=%x\n", system_rev);
    //Add for WLS/WLV
    if ((system_rev < XF)) {
        audio_hw_rev = 0;
    }
    else{
        audio_hw_rev = 1;
    }

	pr_info("%s: audio hw rev is %d\n", __func__, audio_hw_rev);
	return audio_hw_rev;
}

static int opera_get_hw_component(void)
{
	int hw_com = 0;

	hw_com |= HTC_AUDIO_TFA9887;

	if(query_rt5501())
		hw_com |= HTC_AUDIO_RT5501;

	pr_info("%s: audio hw component is %x\n", __func__, hw_com);
	return hw_com;
}

static int opera_enable_digital_mic(void)
{
	int XF = 5;
	printk(KERN_INFO "[XF Only] opera_enable_digital_mic: system_rev=%x\n", system_rev);
	//Add for WLS/WLV
	if ((system_rev < XF)) {
		return 0;
	}
	else {
		return 1;
	}
}

static void opera_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

static int opera_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_info("%s: mode %d\n", __func__, mode);
	return mode;
}

static int opera_get_24b_audio(void)
{
	return 0;
}

static struct acoustic_ops acoustic = {
	.set_q6_effect = opera_set_q6_effect_mode,
	.get_htc_revision = opera_get_hw_revision,
	.get_hw_component = opera_get_hw_component,
	.enable_digital_mic = opera_enable_digital_mic,
	.get_24b_audio = opera_get_24b_audio,
};

static struct q6asm_ops qops = {
	.get_q6_effect = opera_get_q6_effect_mode,
};

static struct msm_pcm_routing_ops rops = {
	.get_q6_effect = opera_get_q6_effect_mode,
};

static struct msm_compr_q6_ops cops = {
	.get_24b_audio = opera_get_24b_audio,
};

static int __init opera_audio_init(void)
{
	int ret = 0;
	pr_info("%s", __func__);

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);

	return ret;
}
module_init(opera_audio_init);

static void __exit opera_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(opera_audio_exit);

MODULE_DESCRIPTION("ALSA Board OPERA");
MODULE_LICENSE("GPL v2");
