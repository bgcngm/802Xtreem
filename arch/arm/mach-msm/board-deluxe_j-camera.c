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
#include "board-deluxe_j.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#define CAM_PIN_PMGPIO_V_RAW_1V2_EN	0
#define CAM_PIN_GPIO_V_CAM_D1V2_EN	V_CAM_D1V2_EN
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XB	MCAM_D1V2_EN_XB
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XC	MCAM_D1V2_EN_XC

#define CAM_PIN_GPIO_V_CAM2_D1V2_EN	V_CAM2_D1V2_EN_XA_XB
#define CAM_PIN_GPIO_V_CAM2_D1V8_EN	PM8921_GPIO_PM_TO_SYS(V_CAM2_D1V8_EN)

#define CAM_PIN_GPIO_V_RAW_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_RAW_1V8_EN)
#define CAM_PIN_GPIO_RAW_RSTN	RAW_RST
#define CAM_PIN_GPIO_RAW_INTR0	RAW_INT0
#define CAM_PIN_GPIO_RAW_INTR1	RAW_INT1
#define CAM_PIN_GPIO_RAW_INTR0_XC	RAW_INT0_XC
#define CAM_PIN_GPIO_RAW_INTR1_XC	RAW_INT1_XC
#define CAM_PIN_GPIO_CAM_MCLK0	CAM_MCLK1
#define CAM_PIN_GPIO_CAM_MCLK1	CAM2_MCLK	/*CAMIF_MCLK*/

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

#define CAM_PIN_CAMERA_ID_XC PM8921_GPIO_PM_TO_SYS(CAM_ID_XC)
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4


#if 0	//temp for flash light

enum cam_flashlight_mode_flags {
	FL_MODE_OFF = 0,
	FL_MODE_TORCH,
	FL_MODE_FLASH,
	FL_MODE_PRE_FLASH,
	FL_MODE_TORCH_LED_A,
	FL_MODE_TORCH_LED_B,
	FL_MODE_TORCH_LEVEL_1,
	FL_MODE_TORCH_LEVEL_2,
	FL_MODE_CAMERA_EFFECT_FLASH,
	FL_MODE_CAMERA_EFFECT_PRE_FLASH,
	FL_MODE_FLASH_LEVEL1,
	FL_MODE_FLASH_LEVEL2,
	FL_MODE_FLASH_LEVEL3,
	FL_MODE_FLASH_LEVEL4,
	FL_MODE_FLASH_LEVEL5,
	FL_MODE_FLASH_LEVEL6,
	FL_MODE_FLASH_LEVEL7,

};

#endif

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

	{
		.func = GPIOMUX_FUNC_2, /*active 12 - A FUNC2 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},
};

static struct msm_gpiomux_config deluxe_j_cam_common_configs[] = {
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
};

static struct msm_gpiomux_config deluxe_j_cam_common_configs_xc[] = {
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
};

#if 0
/* HTC_START_Simon.Ti_Liu_20120702_IMPLEMENT_MCLK_SWITCH */
static struct msm_gpiomux_config ar0260_front_cam_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[10],  /*O(H) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[9], /*O(L) 2MA*/
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

#if 1	// for pre-evt no camera

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

static int deluxe_j_csi_vreg_on(void);
static int deluxe_j_csi_vreg_off(void);

struct msm_camera_device_platform_data deluxe_j_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = deluxe_j_csi_vreg_on,
		.camera_csi_off = deluxe_j_csi_vreg_off,
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
		.camera_csi_on = deluxe_j_csi_vreg_on,
		.camera_csi_off = deluxe_j_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
int deluxe_j_flashlight_control(int mode)
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
	.camera_flash = deluxe_j_flashlight_control,
};
#endif

#ifdef CONFIG_RAWCHIP
static int deluxe_j_use_ext_1v2(void)
{
	return 1;
}

static int deluxe_j_rawchip_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* PM8921_GPIO_PM_TO_SYS(CAM_PIN_GPIO_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	if (rc) {
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto enable_1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);

	mdelay(5);


			mdelay(1);

			rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "rawchip");
			pr_info("rawchip external 1v2 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
			if (rc < 0) {
				pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V2_EN);
				goto enable_ext_1v2_fail;
			}
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
			gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);


	return rc;

enable_ext_1v2_fail:


	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
enable_1v8_fail:
	return rc;
}

static int deluxe_j_rawchip_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);


			rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "rawchip");
			if (rc)
				pr_err("rawchip off(\
					\"gpio %d\", 1.2V) FAILED %d\n",
					CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);

			mdelay(1);




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

static struct msm_camera_rawchip_info deluxe_j_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = deluxe_j_rawchip_vreg_on,
	.camera_rawchip_power_off = deluxe_j_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = deluxe_j_use_ext_1v2,
};

static struct msm_camera_rawchip_info deluxe_j_msm_rawchip_board_info_xc = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0_XC),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1_XC),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = deluxe_j_rawchip_vreg_on,
	.camera_rawchip_power_off = deluxe_j_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = deluxe_j_use_ext_1v2,
};


struct platform_device deluxe_j_msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &deluxe_j_msm_rawchip_board_info,
	},
};

struct platform_device deluxe_j_msm_rawchip_device_xc = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &deluxe_j_msm_rawchip_board_info_xc,
	},
};

#endif


static uint16_t msm_cam_gpio_tbl[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	//CAM_PIN_GPIO_CAM_MCLK1,
#if 0
	CAM_PIN_GPIO_CAM_I2C_DAT, /*CAMIF_I2C_DATA*/
	CAM_PIN_GPIO_CAM_I2C_CLK, /*CAMIF_I2C_CLK*/
#endif
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

/* HTC_START_Simon.Ti_Liu_20120702_IMPLEMENT_MCLK_SWITCH*/
static uint16_t ar0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

static uint16_t imx175_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
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

static struct msm_camera_gpio_conf ar0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = 0,//ar0260_front_cam_configs,
	.cam_gpiomux_conf_tbl_size =0,// ARRAY_SIZE(ar0260_front_cam_configs),
	.cam_gpio_tbl = ar0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ar0260_front_cam_gpio),
};

static struct msm_camera_gpio_conf imx175_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = 0,//imx175_back_cam_configs,
	.cam_gpiomux_conf_tbl_size = 0,//ARRAY_SIZE(imx175_back_cam_configs),
	.cam_gpio_tbl = imx175_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx175_back_cam_gpio),
};


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
		pr_err("%s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}

	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_err("%s: unable to set %s voltage to %d rc:%d\n",
					__func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}

	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_err("%s: Enable regulator %s failed\n", __func__, power);
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
		pr_err("%s: Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_err("%s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int deluxe_j_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int deluxe_j_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

static void mclk_switch(int camera_id)
{
	int rc=0;

	if (system_rev >= XB) {
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

// imx175 and ov8838 are using same power seq
static int deluxe_j_imx175_ov8838_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	// mclk switch to main cam
	mclk_switch (0);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	mdelay(1); 

	if (system_rev >= XC) {
		//XB board, main cam use independence D1V2
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, "CAM_D1V2_EN_XC");
		if (rc) {
			pr_err("sensor_power_enable(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC);
	} else if (system_rev >= XB) {
		//XB board, main cam use independence D1V2
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN_XB");
		if (rc) {
			pr_err("sensor_power_enable(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB);
	}
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "ar0260");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8_EN);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	mdelay(51);

	/* IO */

	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	return rc;

enable_io_fail:
/* HTC_END */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int deluxe_j_imx175_ov8838_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	}
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}

	if (system_rev >= XC ) {
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, "CAM_D1V2_EN_XC");
		if (rc < 0)
		        pr_err("sensor_power_disable(\"gpio %d\", 1.2V) FAILED %d\n",
		                CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, rc);
		else {
		        gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC, 0);
		        gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XC);
		}
	} else if (system_rev >= XB ) {
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN_XB");
		if (rc < 0)
		        pr_err("sensor_power_disable(\"gpio %d\", 1.2V) FAILED %d\n",
		                CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, rc);
		else {
		        gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB, 0);
		        gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN_XB);
		}
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
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
		pr_err("sensor_power_disable(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}



#if 0
struct msm_camera_device_platform_data deluxe_j_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = deluxe_j_csi_vreg_on,
		.camera_csi_off = deluxe_j_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = deluxe_j_csi_vreg_on,
		.camera_csi_off = deluxe_j_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};
#endif

/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
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
/* HTC_END */

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

/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
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


static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = deluxe_j_imx175_ov8838_vreg_on,
	.camera_power_off = deluxe_j_imx175_ov8838_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx175_actuator_table),
	.actuator_info_table = &imx175_actuator_table[0],
#endif
/* HTC_END */
#ifdef CONFIG_AD5823_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

/* HTC_START_Simon.Ti_Liu_20120702_IMPLEMENT_MCLK_SWITCH*/
static struct msm_camera_sensor_info msm_camera_sensor_imx175_data_xb = {
	.sensor_name	= "imx175",
	.camera_power_on = deluxe_j_imx175_ov8838_vreg_on,
	.camera_power_off = deluxe_j_imx175_ov8838_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &imx175_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx175_actuator_table),
	.actuator_info_table = &imx175_actuator_table[0],
#endif
/* HTC_END */
#ifdef CONFIG_AD5823_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};
/* HTC_END */



#endif	//CONFIG_IMX175

#ifdef CONFIG_IMX091
static int deluxe_j_imx091_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

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
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
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

	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int deluxe_j_imx091_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);


	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);


	/* IO */

	rc = camera_sensor_power_disable(reg_8921_lvs4);

	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX091_ACT
static struct i2c_board_info imx091_actuator_i2c_info = {
	I2C_BOARD_INFO("imx091_act", 0x11),
};

static struct msm_actuator_info imx091_actuator_info = {
	.board_info     = &imx091_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

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

/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
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
/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/

static struct msm_camera_sensor_info msm_camera_sensor_imx091_data = {
	.sensor_name	= "imx091",
	.camera_power_on = deluxe_j_imx091_vreg_on,
	.camera_power_off = deluxe_j_imx091_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx091,
	.sensor_platform_info = &sensor_imx091_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
/* HTC_START Simon.Ti_Liu_20120620 add actuator info*/
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx091_actuator_table),
	.actuator_info_table = &imx091_actuator_table[0],
#endif
/* HTC_END */
#if defined(CONFIG_AD5823_ACT)
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
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
	.camera_power_on = deluxe_j_imx175_ov8838_vreg_on,
	.camera_power_off = deluxe_j_imx175_ov8838_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
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
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_ov8838_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};
#endif	//CONFIG_OV8838
#ifdef CONFIG_S5K3H2YX
static int deluxe_j_s5k3h2yx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

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
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
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

	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int deluxe_j_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
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
	.camera_power_on = deluxe_j_s5k3h2yx_vreg_on,
	.camera_power_off = deluxe_j_s5k3h2yx_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_S5K3H2YX

#ifdef CONFIG_S5K6A1GX
//#define CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN 93
static int deluxe_j_s5k6a1gx_vreg_on(void)
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
//	}
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
//	}
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

static int deluxe_j_s5k6a1gx_vreg_off(void)
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
	.camera_power_on = deluxe_j_s5k6a1gx_vreg_on,
	.camera_power_off = deluxe_j_s5k6a1gx_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a1gx,
	.sensor_platform_info = &sensor_s5k6a1gx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
};

#endif	//CONFIG_S5K6A1GX

#ifdef CONFIG_AR0260

static int deluxe_j_ar0260_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	// mclk switch to main cam
	mclk_switch (0);

	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	mdelay(5);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "ar0260");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8_EN);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);

	mdelay(60);	// 50 ~ 200 ms

	/* IO */	// i2c and io use the same power
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);
	
	// mclk switch to front cam
	mclk_switch (1);
	
	return rc;

enable_analog_fail:
	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);

enable_io_fail:
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "ar0260");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8_EN);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);


enable_digital_fail:

	return rc;
}

static int deluxe_j_ar0260_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);/* temp test */
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs4);/* temp test */
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	}
	mdelay(1);

	/* PM8921_GPIO_PM_TO_SYS(CAM_PIN_GPIO_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
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
	mdelay(1);

	
	// mclk switch to main cam
	mclk_switch (0);
	

	return rc;
}

static struct msm_camera_csi_lane_params ar0260_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ar0260_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 1,
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
	.camera_power_on = deluxe_j_ar0260_vreg_on,
	.camera_power_off = deluxe_j_ar0260_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[1],
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data_xb = {
	.sensor_name	= "ar0260",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = deluxe_j_ar0260_vreg_on,
	.camera_power_off = deluxe_j_ar0260_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,/* HTC_START_Simon.Ti_Liu_20120702_IMPLEMENT_MCLK_SWITCH*/
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_ENABLE,//RAWCHIP_MIPI_BYPASS, /* HTC_START_Simon.Ti_Liu_20120702_Enhance_bypass */
};


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
	int rc;
	pr_info("[CAM] %s\n", __func__);
	
	// mclk switch to 2nd cam
	mclk_switch (0);


	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("[CAM] rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		goto exit;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	mdelay(5);
	
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
	mdelay(5);
	
	// mclk switch to 2nd cam
	mclk_switch (1);

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
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	}
exit:

	return rc;
}

static int deluxe_j_ov2722_vreg_off(void)
{
	int rc;
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
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8_EN, "V_RAW_1V8_EN");
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8_EN);
	}	
	// mclk switch to ma cam
	mclk_switch (0);

	return rc;
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

static struct msm_camera_sensor_info msm_camera_sensor_ov2722_data_xb = {
	.sensor_name	= "ov2722",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = deluxe_j_ov2722_vreg_on,
	.camera_power_off = deluxe_j_ov2722_vreg_off,
	.pdata	= &deluxe_j_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov2722,
	.sensor_platform_info = &sensor_ov2722_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_ENABLE,//RAWCHIP_MIPI_BYPASS,
};

#endif	//CONFIG_OV2722



#endif	// for pre-evt no camera
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};


#ifdef CONFIG_I2C
static struct i2c_board_info deluxe_j_camera_i2c_boardinfo[] = {

#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data,
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
};

static struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xb[] = {

#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xb,/* HTC_START_Simon.Ti_Liu_20120702_IMPLEMENT_MCLK_SWITCH*/
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
		.platform_data = &msm_camera_sensor_ar0260_data_xb,
		},
#endif
#ifdef CONFIG_S5K6A1GX
		{
		I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
		.platform_data = &msm_camera_sensor_s5k6a1gx_data,
		},
#endif
};

struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xd_imx175_ar0260[] = {
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xb,
		},
#endif

#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data_xb,
		},
#endif
};

struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xd_imx175_ov2722[] = {
#ifdef CONFIG_IMX175
		{
		I2C_BOARD_INFO("imx175", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx175_data_xb,
		},
#endif

#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data_xb,
		},
#endif

};

struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xd_ov8838_ar0260[] = {

#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif

#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data_xb,
		},
#endif
};

struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xd_ov8838_ov2722[] = {

#ifdef CONFIG_OV8838
		{
		I2C_BOARD_INFO("ov8838", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov8838_data,
		},
#endif

#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data_xb,
		},
#endif

};



//struct i2c_board_info deluxe_j_camera_i2c_boardinfo_xc;


struct msm_camera_board_info deluxe_j_camera_board_info = {
	.board_info = deluxe_j_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo),
};

struct msm_camera_board_info deluxe_j_camera_board_info_xb = {
	.board_info = deluxe_j_camera_i2c_boardinfo_xb,
	.num_i2c_board_info = ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo_xb),
};


#endif  //CONFIG_I2C

int deluxe_j_camera_id(int* id)
{
	int rc=0;
	
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

	rc = gpio_request(CAM_PIN_CAMERA_ID_XC, "CAM_PIN_ID_XC");
	if (rc) {
		pr_err("read cam id fail %d\n", rc);
		return rc;
	}
	*id = gpio_get_value(CAM_PIN_CAMERA_ID_XC);
	pr_info("camera id = %d\n", *id);
	gpio_free(CAM_PIN_CAMERA_ID_XC);

	camera_sensor_power_disable(reg_8921_lvs4);	// I2C

	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("rawchip on(\"gpio %d\", 1.2V) FAILED %d\n",CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		return rc;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	
	return rc;
}

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>

extern unsigned int engineerid; // bit 0

void __init deluxe_j_init_cam(void)
{
	int camera_id=0;
	int rc=0;

	pr_info("%s", __func__);

	if (system_rev >= XC) {
		msm_gpiomux_install(deluxe_j_cam_common_configs_xc,ARRAY_SIZE(deluxe_j_cam_common_configs_xc));
		platform_device_register(&deluxe_j_msm_rawchip_device_xc);
	} else {
		msm_gpiomux_install(deluxe_j_cam_common_configs,ARRAY_SIZE(deluxe_j_cam_common_configs));
		platform_device_register(&deluxe_j_msm_rawchip_device);
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
	if (system_rev >= XD) {
		/* Pre-setting for i2c sensor device */
		rc = deluxe_j_camera_id(&camera_id);
		if (rc<0)
		{
			pr_info("can't read camera id\n");
			return;
		}
		pr_info("camera id=%d\n",camera_id);

		//engineerid =1;

		if (camera_id ==0 && (engineerid&1)==0)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				deluxe_j_camera_i2c_boardinfo_xd_imx175_ar0260,
				ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo_xd_imx175_ar0260));
		}
		else if (camera_id ==0 && (engineerid&1)==1)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				deluxe_j_camera_i2c_boardinfo_xd_imx175_ov2722,
				ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo_xd_imx175_ov2722));
		}
		else if (camera_id ==1 && (engineerid&1)==0)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				deluxe_j_camera_i2c_boardinfo_xd_ov8838_ar0260,
				ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo_xd_ov8838_ar0260));
		}
		else if (camera_id ==1 && (engineerid&1)==1)
		{
			i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
				deluxe_j_camera_i2c_boardinfo_xd_ov8838_ov2722,
				ARRAY_SIZE(deluxe_j_camera_i2c_boardinfo_xd_ov8838_ov2722));
		}
	}
#endif
	
}


#endif	//CONFIG_MSM_CAMERA



