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
#include "board-k2_ul.h"
#include "../../../sound/soc/msm/msm-pcm-routing.h"
#include "../../../sound/soc/msm/msm-compr-q6.h"

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static void k2ul_amp_speaker(bool en)
{
	gpio_set_value(MSM_AUD_SPK_EN, en);
}

static void k2ul_amp_receiver(bool en)
{
}

static void k2ul_amp_headset(bool en)
{
}

static void k2ul_amp_hac(bool en)
{
	gpio_set_value(AUD_HAC_SD, en);
}

struct htc_8930_gpio_pdata htc_audio_gpio = {
	.amp_speaker = k2ul_amp_speaker,
	.amp_receiver = k2ul_amp_receiver,
	.amp_headset = k2ul_amp_headset,
	.amp_hac = k2ul_amp_hac,
	.mi2s_gpio = {
	{
		.gpio_no = MSM_AUD_FM_I2SSYNC,
		.gpio_name = "AUD_SPK_MI2S_WS",
	},
	{
		.gpio_no = MSM_AUD_FM_I2SBCLK,
		.gpio_name = "AUD_SPK_MI2S_BCLK",
	},
	{
		.gpio_no = MSM_AUD_FM_I2SDIN,
		.gpio_name = "AUD_SPK_MI2S_DI",
	},
	{
		.gpio_no = MSM_AUD_FM_I2SDOUT,
		.gpio_name = "AUD_SPK_MI2S_DO",
	},
	},
	.i2s_gpio = {
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_CLK",
	},
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_WS",
	},
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_DI",
	},
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_D0",
	},
	},
	.aux_pcm_gpio = {
	{
		.gpio_no = MSM_AUD_BTPCM_DIN,
		.gpio_name = "AUD_AUX_PCM_DIN",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_DOUT,
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

static int k2ul_get_hw_revision(void)
{
	int audio_hw_rev;

	audio_hw_rev = 0;

	pr_info("%s: audio hw rev is %d\n", __func__, audio_hw_rev);
	return audio_hw_rev;
}

static int k2ul_get_hw_component(void)
{
	return 0;
}

static int k2ul_enable_digital_mic(void)
{
	return 0;
}

static void k2ul_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

static int k2ul_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_info("%s: mode %d\n", __func__, mode);
	return mode;
}

static int k2ul_get_24b_audio(void)
{
	return 1;
}

static struct acoustic_ops acoustic = {
	.set_q6_effect = k2ul_set_q6_effect_mode,
	.get_htc_revision = k2ul_get_hw_revision,
	.get_hw_component = k2ul_get_hw_component,
	.enable_digital_mic = k2ul_enable_digital_mic,
	.get_24b_audio = k2ul_get_24b_audio,
};

static struct q6asm_ops qops = {
	.get_q6_effect = k2ul_get_q6_effect_mode,
};

static struct msm_pcm_routing_ops rops = {
	.get_q6_effect = k2ul_get_q6_effect_mode,
};

static struct msm_compr_q6_ops cops = {
	.get_24b_audio = k2ul_get_24b_audio,
};

static int __init k2ul_audio_init(void)
{
	int ret = 0;
	pr_info("%s", __func__);

	/* SPK AMP */
	gpio_tlmm_config(GPIO_CFG(MSM_AUD_SPK_EN, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(MSM_AUD_SPK_EN, 0);

	/* HAC AMP */
	gpio_tlmm_config(GPIO_CFG(AUD_HAC_SD, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(AUD_HAC_SD, 0);

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);

	return ret;
}
module_init(k2ul_audio_init);

static void __exit k2ul_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(k2ul_audio_exit);

MODULE_DESCRIPTION("ALSA Board K2UL");
MODULE_LICENSE("GPL v2");
