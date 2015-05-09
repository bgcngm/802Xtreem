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
#include "board-m7wl.h"
#include <mach/tpa6185.h>
#include <mach/rt5501.h>
#include "../sound/soc/msm/msm-pcm-routing.h"
#include "../sound/soc/msm/msm-compr-q6.h"

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static int m7wl_get_hw_component(void)
{
    int hw_com = 0;

    hw_com |= HTC_AUDIO_RT5501;
    return hw_com;
}

static int m7wl_enable_digital_mic(void)
{
    int ret;
    //Add for WLJ
    if ((system_rev == XA)||(system_rev == XB)||(system_rev == XC)){
        if ((skuid & 0xFF) == 0x3) {
            printk(KERN_INFO "(skuid & 0xFF) == 0x3\n");
            ret = 1;
        }
        else if ((skuid & 0xFF) == 0x2) {
            printk(KERN_INFO "(skuid & 0xFF) == 0x2\n");
            ret = 1;
        }
        ret = 0;
    }
    else{
        if ((skuid & 0xFFF00) == 0x35B00)
            ret = 1;
        else if ((skuid & 0xFFF00) == 0x38A00)
            ret = 2;
        else
            ret = 3;
    }
    printk(KERN_INFO "m7wlj_enable_digital_mic:skuid=0x%x, system_rev=%x return %d\n", skuid, system_rev,ret);
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

static struct acoustic_ops acoustic = {
        .enable_digital_mic = m7wl_enable_digital_mic,
        .get_hw_component = m7wl_get_hw_component,
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

static void m7wl_audio_pmic_mpp_config(void)
{
	unsigned ret;

	struct pm8xxx_mpp_config_data m7wl_audio_pmic_mpp = {
		.type	= PM8XXX_MPP_TYPE_D_OUTPUT,
		.level	= PM8921_MPP_DIG_LEVEL_S4,
		.control = PM8XXX_MPP_DOUT_CTRL_LOW,
	};

	ret = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(9),
		&m7wl_audio_pmic_mpp);
	if (ret < 0)
		pr_err("%s:MPP_9 configuration failed\n", __func__);
}

static int __init m7wl_audio_init(void)
{
        int ret = 0;

	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);
	m7wl_audio_pmic_mpp_config();
	pr_info("%s", __func__);
	return ret;

}
late_initcall(m7wl_audio_init);

static void __exit m7wl_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(m7wl_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
