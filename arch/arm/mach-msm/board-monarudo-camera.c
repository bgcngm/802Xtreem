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
 *
 */

#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#include <asm/setup.h>

#include "devices.h"
#include "board-monarudo.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#define CAM_PIN_PMGPIO_V_RAW_1V2_EN	0
#define CAM_PIN_GPIO_V_CAM_D1V2_EN	V_CAM_D1V2_EN_XA_XB
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XC	V_CAM_D1V2_EN_XC_XD
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XD	MCAM_D1V2_EN_XD	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
#define CAM_PIN_GPIO_V_CAM2_D1V2_EN	V_CAM2_D1V2_EN_XA_XB
#define CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC	PM8921_GPIO_PM_TO_SYS(V_CAM2_D1V8_EN_XC_XD)
#define CAM_PIN_GPIO_V_RAW_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_RAW_1V8_EN)
#define CAM_PIN_GPIO_RAW_RSTN	RAW_RST_XA_XB
#define CAM_PIN_GPIO_RAW_RSTN_XC	RAW_RST_XC_XD
#define CAM_PIN_GPIO_RAW_INTR0	RAW_INT0_XA_XB
#define CAM_PIN_GPIO_RAW_INTR0_XC	RAW_INT0_XC_XD
#define CAM_PIN_GPIO_RAW_INTR1	RAW_INT1_XA_XB
#define CAM_PIN_GPIO_RAW_INTR1_XC	RAW_INT1_XC_XD
#define CAM_PIN_GPIO_CAM_MCLK0	CAM_MCLK0
#define CAM_PIN_GPIO_CAM_MCLK1	CAM2_MCLK_XA_XB_XC	/*CAMIF_MCLK*/

#define CAM_PIN_GPIO_CAM_I2C_DAT	I2C4_DATA_CAM	/*CAMIF_I2C_DATA*/
#define CAM_PIN_GPIO_CAM_I2C_CLK	I2C4_CLK_CAM	/*CAMIF_I2C_CLK*/

#define CAM_PIN_GPIO_MCAM_SPI_CLK	MCAM_SPI_CLK
#define CAM_PIN_GPIO_MCAM_SPI_CS0	MCAM_SPI_CS0
#define CAM_PIN_GPIO_MCAM_SPI_DI	MCAM_SPI_DI
#define CAM_PIN_GPIO_MCAM_SPI_DO	MCAM_SPI_DO
#define CAM_PIN_GPIO_CAM_PWDN	PM8921_GPIO_PM_TO_SYS(CAM1_PWDN)	//SCLK			GPIO(28)
#define CAM_PIN_GPIO_CAM_VCM_PD	PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD)	//AP2MDM_PON_RESET_N	GPIO(59)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz
#define CAM_PIN_GPIO_CAM2_STANDBY	0

#define CAM_PIN_CAMERA_ID_PVT PM8921_GPIO_PM_TO_SYS(MAIN_CAM_ID_PVT)


#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4	//board-8960.h
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static struct gpiomux_setting cam_settings[] = {
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

	{
		.func = GPIOMUX_FUNC_1, /*active 11 - suspend - I(L) 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
};

static struct msm_gpiomux_config monarudo_cam_common_configs[] = {
#if 1
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
#endif
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	/* gpio config for Rawchip SPI - gsbi10 */
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10], /* O(L) 2MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0], /* I(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
};

static struct msm_gpiomux_config monarudo_cam_common_configs_xc[] = {
#if 1
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
#endif
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0_XC,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1_XC,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	/* gpio config for Rawchip SPI - gsbi10 */
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10], /* O(L) 2MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0], /* I(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
};

static struct msm_gpiomux_config monarudo_cam_common_configs_xd_and_later[] = {
#if 0
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
#endif	
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[11],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0_XC,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1_XC,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	/* gpio config for Rawchip SPI - gsbi10 */
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], /*A FUNC2 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10], /* O(L) 2MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0], /* I(L) 8MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
};


#if 0
/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static struct msm_gpiomux_config ar0260_front_cam_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[10],  /*O(L) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[9], /*O(H) 2MA*/
		},
	},
};


static struct msm_gpiomux_config imx175_back_cam_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[9],  /*O(H) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[10], /*O(L) 2MA*/
		},
	},
};
/* HTC_END */
#endif


#ifdef CONFIG_MSM_CAMERA
static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 27648000,
		.ib  = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 483063040,
		.ib  = 1832252160,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274423680,
		.ib  = 1097694720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 468686080,
		.ib  = 1874744320,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540518400,
		.ib  = 1351296000,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static int monarudo_csi_vreg_on(void);
static int monarudo_csi_vreg_off(void);

struct msm_camera_device_platform_data monarudo_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = monarudo_csi_vreg_on,
		.camera_csi_off = monarudo_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 0,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = monarudo_csi_vreg_on,
		.camera_csi_off = monarudo_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
int monarudo_flashlight_control(int mode)
{
pr_info("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}


static struct msm_camera_sensor_flash_src msm_camera_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = monarudo_flashlight_control,
};
#endif

#ifdef CONFIG_RAWCHIP
static int monarudo_use_ext_1v2(void)
{
#if 0	/* HTC_START - for power sequence */
	if (system_rev >= 1) /* for XB */
		return 1;
	else /* for XA */
		return 0;
#else
	return 1;
#endif
}

static int monarudo_rawchip_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2 = 0;
	pr_info("%s\n", __func__);

	/* PM8921_GPIO_PM_TO_SYS(CAM_PIN_GPIO_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	if (rc) {
		pr_info("rawchip on (\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto enable_1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);

	mdelay(5);

	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	mdelay(1);
	rc = gpio_request(gpio_cam_d1v2, "rawchip");
	pr_info("rawchip external 1v2 gpio_request,%d rc(%d)\n", gpio_cam_d1v2, rc);
	if (rc < 0) {
		pr_info("GPIO(%d) request failed", gpio_cam_d1v2);
		goto enable_ext_1v2_fail;
	}
	gpio_direction_output(gpio_cam_d1v2, 1);
	gpio_free(gpio_cam_d1v2);

	return rc;

enable_ext_1v2_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_info("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
enable_1v8_fail:
	return rc;
}

static int monarudo_rawchip_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2 = 0;

	pr_info("%s\n", __func__);

	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2, "rawchip");
	if (rc)
		pr_err("rawchip off(\
			\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	gpio_direction_output(gpio_cam_d1v2, 0);
	gpio_free(gpio_cam_d1v2);

	mdelay(5);

	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("rawchip off\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);

	return rc;
}

static struct msm_camera_rawchip_info monarudo_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = monarudo_rawchip_vreg_on,
	.camera_rawchip_power_off = monarudo_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = monarudo_use_ext_1v2,
};

struct platform_device monarudo_msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &monarudo_msm_rawchip_board_info,
	},
};

static struct msm_camera_rawchip_info monarudo_msm_rawchip_board_info_xc = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN_XC,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0_XC),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1_XC),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = monarudo_rawchip_vreg_on,
	.camera_rawchip_power_off = monarudo_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = monarudo_use_ext_1v2,
};

struct platform_device monarudo_msm_rawchip_device_xc = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &monarudo_msm_rawchip_board_info_xc,
	},
};
#endif

static uint16_t msm_cam_gpio_tbl[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_CAM_MCLK1,
#if 0
	CAM_PIN_GPIO_CAM_I2C_DAT, /*CAMIF_I2C_DATA*/
	CAM_PIN_GPIO_CAM_I2C_CLK, /*CAMIF_I2C_CLK*/
	//CAM_PIN_GPIO_RAW_INTR0,
	//CAM_PIN_GPIO_RAW_INTR1,
#endif
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static uint16_t ar0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
//	CAM_PIN_GPIO_RAW_INTR0_XC,
//	CAM_PIN_GPIO_RAW_INTR1_XC,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

static uint16_t imx175_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
//	CAM_PIN_GPIO_RAW_INTR0_XC,
//	CAM_PIN_GPIO_RAW_INTR1_XC,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
/* HTC_END */

static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static struct msm_camera_gpio_conf ar0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = 0,//ar0260_front_cam_configs,
	.cam_gpiomux_conf_tbl_size = 0,//ARRAY_SIZE(ar0260_front_cam_configs),
	.cam_gpio_tbl = ar0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ar0260_front_cam_gpio),
};

static struct msm_camera_gpio_conf imx175_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = 0,//imx175_back_cam_configs,
	.cam_gpiomux_conf_tbl_size = 0,//ARRAY_SIZE(imx175_back_cam_configs),
	.cam_gpio_tbl = imx175_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx175_back_cam_gpio),
};
/* HTC_END*/

static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
//static struct regulator *reg_8921_l12;
static struct regulator *reg_8921_lvs4;
//static struct regulator *reg_8921_lvs6;

static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power)
{
	int rc;

	if (power == NULL)
		return -ENODEV;

	*sensor_power = regulator_get(NULL, power);

	if (IS_ERR(*sensor_power)) {
		pr_info("%s: failed to Unable to get %s\n", __func__, power);
		return -ENODEV;
	}

	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_info("%s: failed to unable to set %s voltage to %d rc:%d\n",
					__func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}

	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_info("%s: failed to Enable regulator %s failed\n", __func__, power);
		regulator_put(*sensor_power);
		*sensor_power = NULL;
		return -ENODEV;
	}

	return rc;
}

static int camera_sensor_power_disable(struct regulator *sensor_power)
{

	int rc;
	if (sensor_power == NULL)
		return -ENODEV;

	if (IS_ERR(sensor_power)) {
		pr_info("%s: failed to Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_info("%s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int monarudo_csi_vreg_on(void)
{
	pr_info("%s called\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int monarudo_csi_vreg_off(void)
{
	pr_info("%s called\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

static void mclk_switch(int camera_id)
{
	int rc=0;

	if (system_rev >= XD)
	{
		rc = gpio_request(CAM_PIN_GPIO_CAM_MCLK1, "CAM_PIN_GPIO_CAM_MCLK1");
		if (rc==0) {
			gpio_direction_output(CAM_PIN_GPIO_CAM_MCLK1, camera_id);
			gpio_free(CAM_PIN_GPIO_CAM_MCLK1);
			mdelay(5);
		}
		else {
			pr_err("mclk switch fail\n");
		}
	}
}


static int monarudo_imx175_ov8838_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2 = 0;
	unsigned gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	// mclk switch to main cam
	mclk_switch(0);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);



	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_info("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2, 1);
	gpio_free(gpio_cam_d1v2);
	mdelay(1);

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
	if (system_rev >= XD) {
		//XD board, main cam use independence D1V2
		pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN_XD\n", __func__);
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, "CAM_D1V2_EN_XD");
		pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN_XD (%d)\n", __func__, rc);
		if (rc) {
			pr_err("sensor_power_enable\
			        (\"gpio %d\", 1.2V) FAILED %d\n",
			        CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD);
	}
	mdelay(1);
/* HTC_END*/

#if 1	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2_en, "CAM2_IO_D1V8_EN");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_info("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail2;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	msleep(51);
#endif	

	/* IO */
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);

	/* analog */
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "CAM2_RST");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	return rc;

enable_io_fail:
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}

enable_digital_fail2:

	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_info("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
		goto enable_digital_fail;
	}
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
	
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int monarudo_imx175_ov8838_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2 = 0;
	unsigned gpio_cam_d1v2_en = 0;

	pr_info("called%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
	mdelay(1);

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
	/* digital */
	if (system_rev >= XD) {
		//XD board, main cam use independence D1V2
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, "CAM_D1V2_EN_XD");
		pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN_XD (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_disable\
			        (\"gpio %d\", 1.2V) FAILED %d\n",
			        CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, rc);
		}
		else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XD);
		}
	}
	mdelay(1);
/* HTC_END*/

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "CAM_RST");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	mdelay(1);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);


	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}

	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	rc = gpio_request(gpio_cam_d1v2_en, "CAM2_IO_D1V8_EN");
	if (rc < 0) {
		pr_info("GPIO(%d) request failed", gpio_cam_d1v2_en);
	} else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}

	return rc;
}


#if 0
struct msm_camera_device_platform_data monarudo_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = monarudo_csi_vreg_on,
		.camera_csi_off = monarudo_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = monarudo_csi_vreg_on,
		.camera_csi_off = monarudo_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};
#endif

/* HTC_START steven multiple VCM 20120604 */
#if defined(CONFIG_AD5823_ACT)
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091))
static struct i2c_board_info ad5823_actuator_i2c_info = {
	I2C_BOARD_INFO("ad5823_act", 0x1C),
};

static struct msm_actuator_info ad5823_actuator_info = {
	.board_info     = &ad5823_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
#endif

#if defined(CONFIG_TI201_ACT)
#if defined(CONFIG_IMX091)
static struct i2c_board_info ti201_actuator_i2c_info = {
	I2C_BOARD_INFO("ti201_act", 0x1C),
};

static struct msm_actuator_info ti201_actuator_info = {
	.board_info     = &ti201_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
#endif

#if defined(CONFIG_AD5816_ACT)
#if defined(CONFIG_IMX091)
static struct i2c_board_info ad5816_actuator_i2c_info = {
	I2C_BOARD_INFO("ad5816_act", 0x1C),
};

static struct msm_actuator_info ad5816_actuator_info = {
	.board_info     = &ad5816_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
#endif
/* HTC_END steven multiple VCM 20120604 */

#ifdef CONFIG_IMX175


static struct msm_camera_csi_lane_params imx175_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx175_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx175_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_imx175_led_table[] = {
	  {
		.enable = 1,                    //odilen
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 150,
		.lumen_value = 150,  //150
		.min_step = 61,//61,  //odilen
		.max_step = 255
	  },
	  {
		.enable = 1,//1,          //odilen
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 49,
		.max_step = 61
	  },
	  {
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL7,  //odilen
		.current_ma = 700,
		.lumen_value = 750,
		.min_step = 0,   //odilen
		.max_step = 49//49    //
	  },
	  {
		.enable = 0,//1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 44    //mk0210
	  },
	  {
		.enable = 0,// 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	  },
	  {
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	  },
	  {
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	  },
	  {
		.enable =0,// 2,     //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217   //mk0221
		.min_step = 271,
		.max_step = 317    //mk0210
	  },
	  {
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	  },
	  {
		.enable = 0,//3,  //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	  },
	  {
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	  },
};

static struct camera_led_info msm_camera_sensor_imx175_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx175_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx175_flash_info = {
	.led_info = &msm_camera_sensor_imx175_led_info,
	.led_est_table = msm_camera_sensor_imx175_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx175_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx175_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_imx175 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

/* HTC_START steven multiple VCM 20120605 */
/*Actuator info start*/
#ifdef CONFIG_IMX175
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
static struct msm_actuator_info *imx175_actuator_table[] = {
#if defined(CONFIG_AD5823_ACT)
    &ad5823_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_AD5816_ACT)
    &ad5816_actuator_info,
#endif
};
#endif
#endif
/*Actuator info end*/
/* HTC_END steven multiple VCM 20120605 */

static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = monarudo_imx175_ov8838_vreg_on,
	.camera_power_off = monarudo_imx175_ov8838_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START steven multiple VCM 20120605 */
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx175_actuator_table),
	.actuator_info_table = &imx175_actuator_table[0],
#endif
/* HTC_END steven multiple VCM 20120605 */
#ifdef CONFIG_AD5823_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static struct msm_camera_sensor_info msm_camera_sensor_imx175_data_xd = {
	.sensor_name	= "imx175",
	.camera_power_on = monarudo_imx175_ov8838_vreg_on,
	.camera_power_off = monarudo_imx175_ov8838_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &imx175_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx175_actuator_table),
	.actuator_info_table = &imx175_actuator_table[0],
#endif
#ifdef CONFIG_AD5823_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg,
};
/* HTC_END */



#endif	//CONFIG_IMX175

#ifdef CONFIG_IMX091
static int monarudo_imx091_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2 = 0;
	pr_info("called%s\n", __func__);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	/* analog */
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2, 1);
	gpio_free(gpio_cam_d1v2);
	mdelay(1);

	/* IO */
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int monarudo_imx091_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2 = 0;

	pr_info("called%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

static struct msm_camera_csi_lane_params imx091_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx091_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx091_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_imx091_led_table[] = {
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL1,
//		.current_ma = 150,
//		.lumen_value = 150,
//		.min_step = 50,
//		.max_step = 70
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 29,//23,  //mk0210
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL5,
//		.current_ma = 500,
//		.lumen_value = 500,
//		.min_step = 23,//25,
//		.max_step = 29//26,
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL7,
//		.current_ma = 700,
//		.lumen_value = 700,
//		.min_step = 21,
//		.max_step = 22
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 22    //mk0210
	},

		{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	},
		{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,     //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217   //mk0221
		.min_step = 271,
		.max_step = 317    //mk0210
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0,//3,  //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},};

static struct camera_led_info msm_camera_sensor_imx091_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx091_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx091_flash_info = {
	.led_info = &msm_camera_sensor_imx091_led_info,
	.led_est_table = msm_camera_sensor_imx091_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx091_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx091_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_imx091 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

/* HTC_START steven multiple VCM 20120604 */
/*Actuator info start*/
#ifdef CONFIG_IMX091
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
static struct msm_actuator_info *imx091_actuator_table[] = {
#if defined(CONFIG_AD5823_ACT)
    &ad5823_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_AD5816_ACT)
    &ad5816_actuator_info,
#endif
};
#endif
#endif
/*Actuator info end*/
/* HTC_END steven multiple VCM 20120604 */

static struct msm_camera_sensor_info msm_camera_sensor_imx091_data = {
	.sensor_name	= "imx091",
	.camera_power_on = monarudo_imx091_vreg_on,
	.camera_power_off = monarudo_imx091_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx091,
	.sensor_platform_info = &sensor_imx091_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START steven multiple VCM 20120604 */
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx091_actuator_table),
	.actuator_info_table = &imx091_actuator_table[0],
#endif
/* HTC_END steven multiple VCM 20120604 */
#if defined(CONFIG_AD5823_ACT)
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_imx091_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};
#endif	//CONFIG_IMX091

#ifdef CONFIG_OV8838

static struct msm_camera_csi_lane_params ov8838_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_ov8838_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &ov8838_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_ov8838_led_table[] = {
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL1,
//		.current_ma = 150,
//		.lumen_value = 150,
//		.min_step = 50,
//		.max_step = 70
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 29,//23,  //mk0210
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL5,
//		.current_ma = 500,
//		.lumen_value = 500,
//		.min_step = 23,//25,
//		.max_step = 29//26,
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL7,
//		.current_ma = 700,
//		.lumen_value = 700,
//		.min_step = 21,
//		.max_step = 22
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 22    //mk0210
	},

		{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	},
		{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,     //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217   //mk0221
		.min_step = 271,
		.max_step = 317    //mk0210
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0,//3,  //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},};

static struct camera_led_info msm_camera_sensor_ov8838_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_ov8838_led_table),
};

static struct camera_flash_info msm_camera_sensor_ov8838_flash_info = {
	.led_info = &msm_camera_sensor_ov8838_led_info,
	.led_est_table = msm_camera_sensor_ov8838_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_ov8838_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_ov8838_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_ov8838 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

/* HTC_START steven multiple VCM 20120605 */
/*Actuator info start*/
#ifdef CONFIG_OV8838
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
static struct msm_actuator_info *ov8838_actuator_table[] = {
#if defined(CONFIG_AD5823_ACT)
    &ad5823_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_AD5816_ACT)
    &ad5816_actuator_info,
#endif
};
#endif
#endif
/*Actuator info end*/
/* HTC_END steven multiple VCM 20120605 */

static struct msm_camera_sensor_info msm_camera_sensor_ov8838_data = {
	.sensor_name	= "ov8838",
	.camera_power_on = monarudo_imx175_ov8838_vreg_on,
	.camera_power_off = monarudo_imx175_ov8838_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov8838,
	.sensor_platform_info = &sensor_ov8838_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START steven multiple VCM 20120605 */
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(ov8838_actuator_table),
	.actuator_info_table = &ov8838_actuator_table[0],
#endif
/* HTC_END steven multiple VCM 20120605 */
#ifdef CONFIG_TI201_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = 1,
	.flash_cfg = &msm_camera_sensor_ov8838_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};
#endif	//CONFIG_OV8838


#ifdef CONFIG_S5K3H2YX
static int monarudo_s5k3h2yx_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2 = 0;
	pr_info("called%s\n", __func__);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_info("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	/* analog */
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_info("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_info("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2, 1);
	gpio_free(gpio_cam_d1v2);
	mdelay(1);

	/* IO */
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_info("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int monarudo_s5k3h2yx_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2 = 0;

	pr_info("called%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2 = CAM_PIN_GPIO_V_CAM_D1V2_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2, "CAM_D1V2_EN");
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2, 0);
		gpio_free(gpio_cam_d1v2);
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_info("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params s5k3h2yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_s5k3h2yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &s5k3h2yx_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL1,
//		.current_ma = 150,
//		.lumen_value = 150,
//		.min_step = 50,
//		.max_step = 70
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 29,//23,  //mk0210
		.max_step = 128				
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28			
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL5,
//		.current_ma = 500,
//		.lumen_value = 500,
//		.min_step = 23,//25,
//		.max_step = 29//26,
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
//		{
//		.enable = 0,
//		.led_state = FL_MODE_FLASH_LEVEL7,
//		.current_ma = 700,
//		.lumen_value = 700,
//		.min_step = 21,
//		.max_step = 22
//	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 22    //mk0210
	},

		{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	},
		{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,     //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217   //mk0221
		.min_step = 271,
		.max_step = 317    //mk0210
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0,//3,  //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},};

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k3h2yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k3h2yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k3h2yx_led_info,
	.led_est_table = msm_camera_sensor_s5k3h2yx_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_s5k3h2yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */


static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = monarudo_s5k3h2yx_vreg_on,
	.camera_power_off = monarudo_s5k3h2yx_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_S5K3H2YX

#ifdef CONFIG_S5K6A1GX
//#define CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN 93
static int monarudo_s5k6a1gx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("sensor_power_enable(\"8921_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	udelay(50);

	/* IO */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("sensor_power_enable(\"8921_lvs4\", 1.8V) == %d\n", rc);
	if (rc < 0) {
			pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	udelay(50);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto enable_rst_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	udelay(50);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
	pr_info("digital gpio_request,%d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	udelay(50);

	return rc;

enable_digital_fail:
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
enable_rst_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	return rc;
}

static int monarudo_s5k6a1gx_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
	pr_info("digital gpio_request,%d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	if (rc < 0)
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	}
	udelay(50);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	udelay(50);

	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
			pr_err("sensor_power_disable(\"8921_lvs4\") FAILED %d\n", rc);

	udelay(50);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
	udelay(50);

	return rc;
}

static struct msm_camera_csi_lane_params s5k6a1gx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_s5k6a1gx_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &s5k6a1gx_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_s5k6a1gx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a1gx_data = {
	.sensor_name	= "s5k6a1gx",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = monarudo_s5k6a1gx_vreg_on,
	.camera_power_off = monarudo_s5k6a1gx_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a1gx,
	.sensor_platform_info = &sensor_s5k6a1gx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
};

#endif	//CONFIG_S5K6A1GX

#ifdef CONFIG_AR0260
//#define CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN 93
static int monarudo_ar0260_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	// mclk switch to main cam
	mclk_switch(0);

	/* PM8921_GPIO_PM_TO_SYS(CAM_PIN_GPIO_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	if (rc) {
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);

	mdelay(5);

#if 1	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2_en, "ar0260");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
#endif

	mdelay(60);	// 50 ~ 200 ms

#if 1	/* IO */	// i2c and io use the same power
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("sensor_power_enable(\"8921_lvs4\", 1.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
#endif

	udelay(50);

#if 1	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("sensor_power_enable(\"8921_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
#endif

	udelay(50);


	// mclk switch to 2nd cam
	mclk_switch(1);

	return rc;

enable_analog_fail:
#if 1	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif

enable_io_fail:
#if 1	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}

	rc = gpio_request(gpio_cam_d1v2_en, "ar0260");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 0);
	gpio_free(gpio_cam_d1v2_en);
#endif

enable_digital_fail:

	return rc;
}

static int monarudo_ar0260_vreg_off(void)
{
	int rc;
	unsigned gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);/* temp test */pr_info("%s: sensor_power_disable(\"reg_8921_l8\") == %d\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
	udelay(50);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);/* temp test */pr_info("%s: sensor_power_disable(\"reg_8921_lvs4\") == %d\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_lvs4\") FAILED %d\n", rc);

	udelay(50);

	/* digital */
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}
	rc = gpio_request(gpio_cam_d1v2_en, "ar0260");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}

	/* PM8921_GPIO_PM_TO_SYS(CAM_PIN_GPIO_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	if (rc) {
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	}
	else
	{
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	}
	udelay(50);
	
	// mclk switch to main cam
	mclk_switch(0);

	return rc;
}

static struct msm_camera_csi_lane_params ar0260_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ar0260_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 0,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &ar0260_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_ar0260 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data = {
	.sensor_name	= "ar0260",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = monarudo_ar0260_vreg_on,
	.camera_power_off = monarudo_ar0260_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[1],
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data_xd = {
	.sensor_name	= "ar0260",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = monarudo_ar0260_vreg_on,
	.camera_power_off = monarudo_ar0260_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_ENABLE,//RAWCHIP_MIPI_BYPASS, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
};
/* HTC_END */
#endif	//CONFIG_AR0260

#ifdef CONFIG_OV2722
/*
reset low
dovdd 1.8
delay 1ms
avdd 2.8
delay 5ms
reset low

*/
static int deluxe_j_ov2722_vreg_on(void)
{
	//return deluxe_j_ar0260_vreg_on();
#if 1
	int gpio_cam_d1v2_en=0;
	int rc;
	pr_info("[CAM] %s\n", __func__);

	// mclk switch to main cam
	mclk_switch(0);

	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("[CAM] rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto exit;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	mdelay(5);

	// for ar0260 i2c bus busy
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}
	rc = gpio_request(gpio_cam_d1v2_en, "ov2722");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);

	
	/* reset high */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_high_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);


	/* io */	// i2c and io use the same power
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);


	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	/* reset low */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_low_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	// mclk switch to 2nd cam
	mclk_switch(1);
	return rc;

reset_low_fail:
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"reg_8921_l8\") FAILED %d\n", rc);

enable_analog_fail:
	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"reg_8921_lvs4\") FAILED %d\n", rc);

enable_io_fail:
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	if (rc >= 0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	
reset_high_fail:
	rc = gpio_request(gpio_cam_d1v2_en, "V_RAW_1V8_EN");
	if (rc>=0) {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}

enable_digital_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	}


exit:

	return rc;
#endif	
}

static int deluxe_j_ov2722_vreg_off(void)
{
	//return deluxe_j_ar0260_vreg_off();
#if 1
	int rc;
	int gpio_cam_d1v2_en=0;
	pr_info("[CAM] %s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);/* temp test */
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
		
	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_lvs4\") FAILED %d\n", rc);


	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc < 0) {
		pr_err("[CAM] rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	}
	else
	{
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	}

	// digital
	if (system_rev == XA || system_rev == XB) {
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V2_EN;
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN_XC;
	}
	rc = gpio_request(gpio_cam_d1v2_en, "V_RAW_1V8_EN");
	if (rc>=0) {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}	
	// mclk switch to main cam
	mclk_switch(0);

	return rc;
#endif
}

static struct msm_camera_csi_lane_params ov2722_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ov2722_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &ov2722_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_ov2722 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2722_data = {
	.sensor_name	= "ov2722",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = deluxe_j_ov2722_vreg_on,
	.camera_power_off = deluxe_j_ov2722_vreg_off,
	.pdata	= &monarudo_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov2722,
	.sensor_platform_info = &sensor_ov2722_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_ENABLE,//RAWCHIP_MIPI_BYPASS,
};

#endif	//CONFIG_OV2722



#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
_func, _inv, _disable) \
{ \
.config        = { \
.direction     = _dir, \
.output_buffer = _buf, \
.output_value  = _val, \
.pull          = _pull, \
.vin_sel       = _vin, \
.out_strength  = _out_strength, \
.function      = _func, \
.inv_int_pol   = _inv, \
.disable_pin   = _disable, \
} \
}
	
struct pm8xxx_gpio_init {
unsigned  gpio;
struct pm_gpio config;
};

extern unsigned int engineerid; // bit 0

int monarudo_camera_id(int* id)
{
	int rc=0;
	
	struct pm8xxx_gpio_init gpio[] = {
		PM8XXX_GPIO_INIT(CAM_ID_XB, PM_GPIO_DIR_IN,
		PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_DN,
		PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
		PM_GPIO_FUNC_NORMAL, 0, 0),
	};
	pm8xxx_gpio_config(gpio[0].gpio,
	                &gpio[0].config);
	
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		return rc;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	mdelay(1);

	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		return rc;
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_CAMERA_ID_PVT, "CAM_PIN_CAMERA_ID_PVT");
	if (rc) {
		pr_err("read cam id fail %d\n", rc);
		return rc;
	}
	*id = gpio_get_value(CAM_PIN_CAMERA_ID_PVT);
	pr_info("camera id = %d\n", *id);
	gpio_free(CAM_PIN_CAMERA_ID_PVT);


	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		return rc;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);

	rc = camera_sensor_power_disable(reg_8921_lvs4);	// I2C
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		return rc;
	}
	
	return rc;
}

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>

static struct i2c_board_info boardinfo_pvt_imx175_ar0260[] = {
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xd,
		},
#endif

#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data_xd,
		},
#endif
};

static struct i2c_board_info boardinfo_pvt_imx175_ov2722[] = {
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xd,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif
};


static struct i2c_board_info boardinfo_pvt_ov8838_ar0260[] = {

#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data_xd,
		},
#endif
};

static struct i2c_board_info boardinfo_pvt_ov8838_ov2722[] = {

#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif

#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif

};

void __init monarudo_init_cam(void)
{
	int camera_id=0;
	int rc=0;
	pr_info("%s", __func__);

	if (system_rev == XA || system_rev == XB) {
		msm_gpiomux_install(monarudo_cam_common_configs,
				ARRAY_SIZE(monarudo_cam_common_configs));
	} else if (system_rev == XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		pr_info("system_rev=xc,monarudo_cam_common_configs_xc");
		msm_gpiomux_install(monarudo_cam_common_configs_xc,
				ARRAY_SIZE(monarudo_cam_common_configs_xc));
	}
	else {
		pr_info("system_rev=xd or later");
		msm_gpiomux_install(monarudo_cam_common_configs_xd_and_later,
			ARRAY_SIZE(monarudo_cam_common_configs_xd_and_later));
	}
	if (system_rev == XA || system_rev == XB) {
		platform_device_register(&monarudo_msm_rawchip_device);
	} else if (system_rev >= XC) {	/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
		pr_info("system_rev=xc,monarudo_msm_rawchip_device_xc");
		platform_device_register(&monarudo_msm_rawchip_device_xc);
	}
	
	platform_device_register(&msm_camera_server);
    platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);

#ifdef CONFIG_I2C
	if (system_rev >= PVT) {
		/* Pre-setting for i2c sensor device */
		rc = monarudo_camera_id(&camera_id);
		if (rc<0)
		{
			pr_info("can't read camera id\n");
			return;
		}
		//engineerid=1;

		pr_info("camera id=%d engineerid=0x%x\n",camera_id,engineerid);
		if (camera_id ==0 && (engineerid&1)==0)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				boardinfo_pvt_imx175_ar0260,
				ARRAY_SIZE(boardinfo_pvt_imx175_ar0260));
		}
		else if (camera_id ==0 && (engineerid&1)==1)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				boardinfo_pvt_imx175_ov2722,
				ARRAY_SIZE(boardinfo_pvt_imx175_ov2722));
		}
		else if (camera_id ==1 && (engineerid&1)==0)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				boardinfo_pvt_ov8838_ar0260,
				ARRAY_SIZE(boardinfo_pvt_ov8838_ar0260));
		}
		else if (camera_id ==1 && (engineerid&1)==1)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				boardinfo_pvt_ov8838_ov2722,
				ARRAY_SIZE(boardinfo_pvt_ov8838_ov2722));
		}
	}
#endif
}

#ifdef CONFIG_I2C
static struct i2c_board_info monarudo_camera_i2c_boardinfo[] = {
#if 1	// for pre-evt no camera
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data,
		},
#endif
#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif
#ifdef CONFIG_IMX091
		{
		I2C_BOARD_INFO("imx091", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx091_data,
		},
#endif
#ifdef CONFIG_S5K3H2YX
		{
		I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
		},
#endif
#ifdef CONFIG_S5K6A1GX
		{
		I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
		.platform_data = &msm_camera_sensor_s5k6a1gx_data,
		},
#endif
#endif	// for pre-evt no camera
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
static struct i2c_board_info monarudo_camera_i2c_boardinfo_xd[] = {
#if 1	// for pre-evt no camera
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xd,
		},
#endif
#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif
#ifdef CONFIG_IMX091
		{
		I2C_BOARD_INFO("imx091", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx091_data,
		},
#endif
#ifdef CONFIG_S5K3H2YX
		{
		I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data_xd,
		},
#endif
#ifdef CONFIG_S5K6A1GX
		{
		I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
		.platform_data = &msm_camera_sensor_s5k6a1gx_data,
		},
#endif
#endif	// for pre-evt no camera
};
/* HTC_END */

struct msm_camera_board_info monarudo_camera_board_info = {
	.board_info = monarudo_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(monarudo_camera_i2c_boardinfo),
};

/* HTC_START_Simon.Ti_Liu_20120711_IMPLEMENT_MCLK_SWITCH */
struct msm_camera_board_info monarudo_camera_board_info_xd = {
	.board_info = monarudo_camera_i2c_boardinfo_xd,
	.num_i2c_board_info = ARRAY_SIZE(monarudo_camera_i2c_boardinfo_xd),
};
/* HTC_END */
#endif  //CONFIG_I2C

#endif	//CONFIG_MSM_CAMERA
