/* arch/arm/mach-msm/board-jel_dd-gpio.c
 * Copyright (C) 2011 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <mach/gpiomux.h>
#include "board-jel_dd.h"


static struct gpiomux_setting gsbi3 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if 0
static struct gpiomux_setting gsbi4 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting gsbi8 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if 0
/* The SPI configurations apply to GSBI 10*/
static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_10MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_10MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
};

static struct gpiomux_setting audio_auxpcm_input[] = {
        /* Suspended state */
        {
		.func = GPIOMUX_FUNC_GPIO,
                .drv = GPIOMUX_DRV_10MA,
                .pull = GPIOMUX_PULL_DOWN,
                .dir = GPIOMUX_IN,
        },
        /* Active state */
        {
		.func = GPIOMUX_FUNC_1,
                .drv = GPIOMUX_DRV_10MA,
                .pull = GPIOMUX_PULL_NONE,
                .dir = GPIOMUX_IN,
        },
};


static struct msm_gpiomux_config jel_dd_gsbi_configs[] __initdata = {
	{
		.gpio      = JEL_DD_GPIO_TP_I2C_SDA,	/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3,
			[GPIOMUX_ACTIVE] = &gsbi3,
		},
	},
	{
		.gpio      = JEL_DD_GPIO_TP_I2C_SCL,	/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3,
			[GPIOMUX_ACTIVE] = &gsbi3,
		},
	},
#if 0 /* move config to cam_settings */
	{
		.gpio      = JEL_DD_GPIO_CAM_I2C_DAT,	/* GSBI4 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4,
		},
	},
	{
		.gpio      = JEL_DD_GPIO_CAM_I2C_CLK,	/* GSBI4 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4,
		},
	},
	{
		/* GSBI10 SPI QUP JEL_DD_GPIO_MCAM_SPI_CLK */
		.gpio      = JEL_DD_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		/* GSBI10 SPI QUP JEL_DD_GPIO_MCAM_SPI_CS0 */
		.gpio      = JEL_DD_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		/* GSBI10 SPI QUP JEL_DD_GPIO_MCAM_SPI_DI */
		.gpio      = JEL_DD_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		/* GSBI10 SPI QUP JEL_DD_GPIO_MCAM_SPI_DO */
		.gpio      = JEL_DD_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
#endif
	{
		.gpio	   = JEL_DD_GPIO_AC_I2C_SDA,	/* GSBI8 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi8,
			[GPIOMUX_ACTIVE] = &gsbi8,
		},
	},
	{
		.gpio	   = JEL_DD_GPIO_AC_I2C_SCL,	/* GSBI8 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi8,
			[GPIOMUX_ACTIVE] = &gsbi8,
		},
	},
	{
		.gpio      = JEL_DD_GPIO_SR_I2C_DAT,	/* GSBI12 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
			[GPIOMUX_ACTIVE] = &gsbi12,
		},
	},
	{
		.gpio      = JEL_DD_GPIO_SR_I2C_CLK,	/* GSBI12 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
			[GPIOMUX_ACTIVE] = &gsbi12,
		},
	},
};

static struct msm_gpiomux_config jel_dd_slimbus_configs[] __initdata = {
	{
		.gpio	= JEL_DD_GPIO_AUD_WCD_SB_CLK,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= JEL_DD_GPIO_AUD_WCD_SB_DATA,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config jel_dd_audio_codec_configs[] __initdata = {
	{
		.gpio = JEL_DD_GPIO_AUD_WCD_MCLK,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

static struct msm_gpiomux_config jel_dd_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm_input[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm_input[1],
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};


static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = JEL_DD_GPIO_WCN_CMD_DATA2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_WCN_CMD_DATA1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_WCN_CMD_DATA0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_WCN_CMD_SET,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_WCN_CMD_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

static struct gpiomux_setting cam_settings[11] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend - I(L) 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1 - A FUNC1 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2 - O(L) 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3 - A FUNC1 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 4 - A FUNC2 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 5 - I(L) 4MA*/
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6 - A FUNC2 2MA*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 7 - I(NP) 2MA*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 8 - I(L) 2MA*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 9 - O(H) 2MA*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 10 - O(L) 2MA*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

};

static struct msm_gpiomux_config jel_dd_cam_configs[] = {
	{
		.gpio = JEL_DD_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = JEL_DD_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = JEL_DD_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = JEL_DD_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = JEL_DD_GPIO_RAW_INTR0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	{
		.gpio = JEL_DD_GPIO_RAW_INTR1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	/* gpio config for Rawchip SPI - gsbi10 */
	{
		.gpio      = JEL_DD_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 4MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
	{
		.gpio      = JEL_DD_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 4MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
	{
		.gpio      = JEL_DD_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 4MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
	{
		.gpio      = JEL_DD_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 4MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
};

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_mdp_vsync_configs[] __initdata = {
	{
		.gpio = JEL_DD_GPIO_LCD_TE,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL

static struct gpiomux_setting mhl_i2c_suspend_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_i2c_active_cfg = {
    .func = GPIOMUX_FUNC_1,
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config jel_dd_mhl_i2c_configs[] __initdata = {
    {
        .gpio = 36,
        .settings = {
            [GPIOMUX_ACTIVE]    = &mhl_i2c_active_cfg,
            [GPIOMUX_SUSPENDED] = &mhl_i2c_suspend_cfg,
        },
    },
    {
        .gpio = 37,
        .settings = {
            [GPIOMUX_ACTIVE]    = &mhl_i2c_active_cfg,
            [GPIOMUX_SUSPENDED] = &mhl_i2c_suspend_cfg,
        },
    },
};

static struct gpiomux_setting mhl_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_active_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting mhl_active_2_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config jel_dd_mhl_configs[] __initdata = {
	{
		.gpio = JEL_DD_GPIO_MHL_RSTz,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mhl_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_MHL_INT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mhl_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
};


static struct gpiomux_setting hdmi_suspend_pd_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting hdmi_suspend_np_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config jel_dd_hdmi_configs[] __initdata = {
	{
		.gpio = JEL_DD_GPIO_HDMI_DDC_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_np_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_HDMI_DDC_DATA,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_np_cfg,
		},
	},
	{
		.gpio = JEL_DD_GPIO_HDMI_HPD,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_pd_cfg,
		},
	},
};
#endif


static struct gpiomux_setting usb_id_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config cable_detect_usbid_config[] __initdata = {
	{
		.gpio = JEL_DD_GPIO_USB_ID1,
		.settings = {
			[GPIOMUX_ACTIVE] = &usb_id_cfg,
			[GPIOMUX_SUSPENDED] = &usb_id_cfg,
		},
	}
};

int __init jel_dd_gpiomux_init(void)
{
	int rc;

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return rc;
	}

	msm_gpiomux_install(jel_dd_cam_configs,
			ARRAY_SIZE(jel_dd_cam_configs));

	msm_gpiomux_install(jel_dd_gsbi_configs,
			ARRAY_SIZE(jel_dd_gsbi_configs));

	msm_gpiomux_install(jel_dd_slimbus_configs,
			ARRAY_SIZE(jel_dd_slimbus_configs));

	msm_gpiomux_install(jel_dd_audio_codec_configs,
			ARRAY_SIZE(jel_dd_audio_codec_configs));

	msm_gpiomux_install(jel_dd_audio_auxpcm_configs,
			ARRAY_SIZE(jel_dd_audio_auxpcm_configs));

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm_gpiomux_install(jel_dd_mhl_i2c_configs,
			ARRAY_SIZE(jel_dd_mhl_i2c_configs));
	msm_gpiomux_install(jel_dd_hdmi_configs,
			ARRAY_SIZE(jel_dd_hdmi_configs));
	msm_gpiomux_install(jel_dd_mhl_configs,
			ARRAY_SIZE(jel_dd_mhl_configs));
#endif
	msm_gpiomux_install(msm8960_mdp_vsync_configs,
			ARRAY_SIZE(msm8960_mdp_vsync_configs));

	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));

	msm_gpiomux_install(cable_detect_usbid_config,
			ARRAY_SIZE(cable_detect_usbid_config));

	return 0;
}

