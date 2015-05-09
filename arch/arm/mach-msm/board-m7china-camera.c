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

#include "devices.h"
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

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

/* HTC_START - for HW VCM work-around */
#if defined(CONFIG_RUMBAS_ACT)
void m7china_vcm_vreg_on(void);
void m7china_vcm_vreg_off(void);
#endif
/* HTC_END */

#define CAM_PIN_PMGPIO_V_RAW_1V2_EN	PM8921_GPIO_PM_TO_SYS(V_RAW_1V2_EN)
#define CAM_PIN_PMGPIO_V_RAW_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_RAW_1V8_EN)
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XA	V_CAM_D1V2_EN_XA
#define CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB	PM8921_GPIO_PM_TO_SYS(V_CAM_D1V2_EN_XB)
#define CAM_PIN_GPIO_MCAM_D1V2_EN	PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN)
//#define CAM_PIN_GPIO_V_CAM_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_CAM_1V8_EN)
#define CAM_PIN_GPIO_V_CAM2_D1V8 PM8921_GPIO_PM_TO_SYS(V_CAM2_1V8_EN)

#define CAM_PIN_GPIO_RAW_RSTN	RAW_RST
#define CAM_PIN_GPIO_RAW_INTR0	RAW_INT0
#define CAM_PIN_GPIO_RAW_INTR1	RAW_INT1_XB
#define CAM_PIN_GPIO_CAM_MCLK0	CAM_MCLK0	/*CAMIF_MCLK*/
#define CAM_PIN_GPIO_CAM_SEL	CAM_SEL	/*CAMIF_MCLK*/

#define CAM_PIN_GPIO_CAM_I2C_DAT	I2C4_DATA_CAM	/*CAMIF_I2C_DATA*/
#define CAM_PIN_GPIO_CAM_I2C_CLK	I2C4_CLK_CAM	/*CAMIF_I2C_CLK*/

#define CAM_PIN_GPIO_MCAM_SPI_CLK	MCAM_SPI_CLK
#define CAM_PIN_GPIO_MCAM_SPI_CS0	MCAM_SPI_CS0
#define CAM_PIN_GPIO_MCAM_SPI_DI	MCAM_SPI_DI
#define CAM_PIN_GPIO_MCAM_SPI_DO	MCAM_SPI_DO
#define CAM_PIN_GPIO_CAM_PWDN	PM8921_GPIO_PM_TO_SYS(CAM1_PWDN)	//SCLK			GPIO(28)
#define CAM_PIN_GPIO_CAM_VCM_PD	PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD)	//AP2MDM_PON_RESET_N	GPIO(59)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz

#define CAM_PIN_CAMERA_ID PM8921_GPIO_PM_TO_SYS(MAIN_CAM_ID)
#define CAM_PIN_FRONT_CAMERA_ID PM8921_GPIO_PM_TO_SYS(CAM2_ID)

#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4	//board-8960.h

extern unsigned int system_rev;
extern unsigned int engineerid; // bit 0

/* HTC_START 20130329 */
#if defined(CONFIG_ACT_OIS_BINDER)
extern void HtcActOisBinder_i2c_add_driver(void* i2c_client);
extern void HtcActOisBinder_open_init(void);
extern void HtcActOisBinder_power_down(void);
extern int32_t HtcActOisBinder_act_set_ois_mode(int ois_mode);
extern int32_t HtcActOisBinder_mappingTbl_i2c_write(int startup_mode, void* sensor_actuator_info);
#endif
/* HTC_END */


#if defined(CONFIG_VD6869)
static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data;
#endif
#if defined(CONFIG_IMX135)
static struct msm_camera_sensor_info msm_camera_sensor_imx135_data;
#endif
#if defined(CONFIG_AR0260)
static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data;
#endif
#if defined(CONFIG_OV2722)
static struct msm_camera_sensor_info msm_camera_sensor_ov2722_data;
#endif
#if defined(CONFIG_OV4688)
static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data;
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
		.func = GPIOMUX_FUNC_GPIO, /* 12 - O(L) 4MA*/
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /* 13 - O(H) 4MA*/
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /* 14 - O(L) 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /* 15 - O(H) 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_1, /* 16 - I(PD) 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1, /* 17 - A FUNC1 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2, /* 18 - A FUNC2 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /* 19 - I(L) 6MA*/
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
};

static struct msm_gpiomux_config m7china_cam_common_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], /*A FUNC1 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[14], /*O(L) 6MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], /*A FUNC1 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], /*A FUNC1 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19],
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
			[GPIOMUX_ACTIVE] = &cam_settings[18], /*A FUNC2 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19], /*I(PD)  6MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], /*A FUNC2 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19], /* I(PD)  6MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], /*A FUNC2 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19], /* I(PD) 6MA*/
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], /*A FUNC2 6MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[19], /*I(PD) 6MA*/
		},
	},
};

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
		.ab  = 700000000, /* 468686080, */
		.ib  = 3749488640U, /* 1874744320, */
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
		.ab  = 200000000, /* 540518400, */
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


#if 1	/* HTC_START synced */

static int m7china_csi_vreg_on(void);
static int m7china_csi_vreg_off(void);

struct msm_camera_device_platform_data m7china_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = m7china_csi_vreg_on,
		.camera_csi_off = m7china_csi_vreg_off,
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
		.camera_csi_on = m7china_csi_vreg_on,
		.camera_csi_off = m7china_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
#if defined(CONFIG_IMX175) || defined(CONFIG_IMX135) || defined(CONFIG_VD6869) || defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_OV4688)
int m7china_flashlight_control(int mode)
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
	.camera_flash = m7china_flashlight_control,
};
#endif
#endif

int gpio_set(int gpio,int enable)
{
	int rc = 0;
	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	rc = gpio_request(gpio, "gpio");
	if (rc < 0) {
		pr_err("set gpio(%d) fail", gpio);
	    return rc;
	}
	gpio_direction_output(gpio, enable);
	gpio_free(gpio);

	return rc;
}
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


int check_yushanII_flag(void)
{
#if defined(CONFIG_VD6869)
	if (msm_camera_sensor_vd6869_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , VD6869 htc_image=%d\n", msm_camera_sensor_vd6869_data.htc_image);
		return 0;
	}
#endif
#if defined(CONFIG_IMX135)
	if (msm_camera_sensor_imx135_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , IMX135 htc_image=%d\n", msm_camera_sensor_imx135_data.htc_image);
		return 0;
	}
#endif

#if defined(CONFIG_OV4688)
	if (msm_camera_sensor_ov4688_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , ov4688 htc_image=%d\n", msm_camera_sensor_ov4688_data.htc_image);
		return 0;
	}
#endif
	pr_info("check_yushanII_flag() , With yushanII\n");
	return 1;
}


#ifdef CONFIG_RAWCHIPII
static int m7china_use_ext_1v2(void)
{
	return 1;
}

static int m7china_rawchip_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v8_en=0;
	int gpio_cam_d1v2_en=0;

	pr_info("%s\n", __func__);

/*
	if (check_yushanII_flag() == 0) {
		return 0;
	}
*/
	gpio_cam_d1v8_en = CAM_PIN_PMGPIO_V_RAW_1V8_EN;
	rc = gpio_request(gpio_cam_d1v8_en, "rawchip_1v8");
	pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v8_en);
		goto enable_1v8_fail;
	}
	gpio_direction_output(gpio_cam_d1v8_en, 1);
	gpio_free(gpio_cam_d1v8_en);
	mdelay(1);

	gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_RAW_1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "rawchip_1v2");
	pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V2_EN (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_1v2_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	return rc;


enable_1v2_fail:
	rc = gpio_request(gpio_cam_d1v8_en, "rawchip_1v8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", gpio_cam_d1v8_en);
	else {
		gpio_direction_output(gpio_cam_d1v8_en, 0);
		gpio_free(gpio_cam_d1v8_en);
	}

enable_1v8_fail:
	return rc;
}

static int m7china_rawchip_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;
	int gpio_cam_d1v8_en=0;

	pr_info("%s\n", __func__);
/*
	if (check_yushanII_flag() == 0) {
		return 0;
	}
*/

	gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_RAW_1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "rawchip_1v2");
	pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V2_EN off (%d)\n", __func__, rc);
	if (rc)
		pr_err("rawchip off(\
			\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	gpio_direction_output(gpio_cam_d1v2_en, 0);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);


	gpio_cam_d1v8_en = CAM_PIN_PMGPIO_V_RAW_1V8_EN;
	rc = gpio_request(gpio_cam_d1v8_en, "rawchip_1v8");
	pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN off (%d)\n", __func__, rc);
	if (rc)
		pr_err("rawchip off(\
			\"gpio %d\", 1.8V) FAILED %d\n",
			gpio_cam_d1v8_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v8_en, 0);
		gpio_free(gpio_cam_d1v8_en);
	}
	mdelay(1);

	return rc;
}

static struct msm_camera_rawchip_info m7china_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = m7china_rawchip_vreg_on,
	.camera_rawchip_power_off = m7china_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = m7china_use_ext_1v2,
};

struct platform_device m7china_msm_rawchip_device = {
	.name	= "yushanII",
	.dev	= {
		.platform_data = &m7china_msm_rawchip_board_info,
	},
};
#endif


#if defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_S5K6A1GX)
static uint16_t msm_cam_gpio_tbl[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_AR0260
static uint16_t ar0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_OV2722
static uint16_t ov2722_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_IMX175
static uint16_t imx175_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_IMX135
static uint16_t imx135_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_VD6869
static uint16_t vd6869_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
};
#endif

#ifdef CONFIG_OV4688
static uint16_t ov4688_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

#endif

#if defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_S5K6A1GX)
static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};
#endif

#ifdef CONFIG_AR0260
static struct msm_camera_gpio_conf ar0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ar0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ar0260_front_cam_gpio),
};
#endif

#ifdef CONFIG_OV2722
static struct msm_camera_gpio_conf ov2722_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ov2722_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ov2722_front_cam_gpio),
};
#endif

#ifdef CONFIG_IMX175
static struct msm_camera_gpio_conf imx175_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = imx175_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx175_back_cam_gpio),
};
#endif

#ifdef CONFIG_IMX135
static struct msm_camera_gpio_conf imx135_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = imx135_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx135_back_cam_gpio),
};
#endif

#ifdef CONFIG_VD6869
static struct msm_camera_gpio_conf vd6869_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = vd6869_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(vd6869_back_cam_gpio),
};
#endif

static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_lvs4;
#if defined(CONFIG_VD6869) || defined(CONFIG_AR0260) || defined(CONFIG_OV2722)
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
#endif
#if defined(CONFIG_IMX135)
static struct regulator *reg_8921_l23;
#endif

#ifdef CONFIG_OV4688
static struct msm_camera_gpio_conf ov4688_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ov4688_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ov4688_back_cam_gpio),
};
#endif

static int m7china_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int m7china_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

static void update_yushanII_flag(enum htc_camera_image_type_board htc_image)
{
	pr_info("update_yushanII_flag() , htc_image=%d\n", htc_image);

#if defined(CONFIG_OV4688)
	msm_camera_sensor_ov4688_data.htc_image = htc_image;
	msm_camera_sensor_ov4688_data.video_hdr_capability &= msm_camera_sensor_ov4688_data.htc_image;
#endif

#if defined(CONFIG_VD6869)
	msm_camera_sensor_vd6869_data.htc_image = htc_image;
	msm_camera_sensor_vd6869_data.video_hdr_capability &= msm_camera_sensor_vd6869_data.htc_image;
#endif
#if defined(CONFIG_IMX135)
	msm_camera_sensor_imx135_data.htc_image = htc_image;
	msm_camera_sensor_imx135_data.video_hdr_capability &= msm_camera_sensor_imx135_data.htc_image;
#endif
#if defined(CONFIG_AR0260)
	msm_camera_sensor_ar0260_data.htc_image = htc_image;
	msm_camera_sensor_ar0260_data.video_hdr_capability &= msm_camera_sensor_ar0260_data.htc_image;
#endif
#if defined(CONFIG_OV2722)
	msm_camera_sensor_ov2722_data.htc_image = htc_image;
	msm_camera_sensor_ov2722_data.video_hdr_capability &= msm_camera_sensor_ov2722_data.htc_image;
#endif

	return;
}

static void m7china_yushanii_probed(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	update_yushanII_flag(htc_image);
}

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
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091) || defined(CONFIG_VD6869))
static struct i2c_board_info ti201_actuator_i2c_info = {
	I2C_BOARD_INFO("ti201_act", 0x1C),
};

static struct msm_actuator_info ti201_actuator_info = {
	.board_info     = &ti201_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
/* HTC_START 20130329 */
#if defined(CONFIG_ACT_OIS_BINDER)
	.oisbinder_i2c_add_driver = HtcActOisBinder_i2c_add_driver,
	.oisbinder_open_init = HtcActOisBinder_open_init,
	.oisbinder_power_down = HtcActOisBinder_open_init,
	.oisbinder_act_set_ois_mode = HtcActOisBinder_act_set_ois_mode,
	.oisbinder_mappingTbl_i2c_write = HtcActOisBinder_mappingTbl_i2c_write,
#endif
/* HTC_END */
};
#endif
#endif

#if defined(CONFIG_AD5816_ACT)
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091))
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

#if defined(CONFIG_RUMBAS_ACT)
static struct i2c_board_info rumbas_actuator_i2c_info = {
	I2C_BOARD_INFO("rumbas_act", 0x32),
};

static struct msm_actuator_info rumbas_actuator_info = {
	.board_info     = &rumbas_actuator_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
	.otp_diviation	= 75,
/* HTC_START - for HW VCM work-around */
#if defined(CONFIG_RUMBAS_ACT)
	.vcm_wa_vreg_on = m7china_vcm_vreg_on,
	.vcm_wa_vreg_off = m7china_vcm_vreg_off,
#endif
/* HTC_END */
};
#endif
/* HTC_END */

// HTC_START pg 20130220 lc898212 act enable
#ifdef CONFIG_LC898212_ACT
static struct i2c_board_info lc898212_actuator_i2c_info = {
	I2C_BOARD_INFO("lc898212_act", 0x11),
};

static struct msm_actuator_info lc898212_actuator_info = {
	.board_info     = &lc898212_actuator_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
// HTC_END pg 20130220 lc898212 act enable

#ifdef CONFIG_IMX175
static int m7china_imx175_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2_en = 0;
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

	if (1) {
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
	}

	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	mdelay(1);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN;

	rc = gpio_request(gpio_cam_d1v2_en, "CAM2_IO_D1V8_EN");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	/* HTC_END */
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

static int m7china_imx175_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2_en = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_disabled\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
	}
	gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
	gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	mdelay(1);

	if (1) {
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
	}

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	/* HTC_END */
	if (rc < 0)
		pr_err("sensor_power_disable\
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
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

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
	.low_cap_limit		= 14,
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
/*Actuator info end*/


static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = m7china_imx175_vreg_on,
	.camera_power_off = m7china_imx175_vreg_off,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
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
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_IMX175


#ifdef CONFIG_IMX135
static int m7china_imx135_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	/* mclk switch */
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
		goto enable_mclk_switch_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
	gpio_free(CAM_PIN_GPIO_CAM_SEL);
	mdelay(1);
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
			goto enable_raw_1v8_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		mdelay(5);
	}
	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	pr_info("%s: 8921_l23 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l23", 1800000, &reg_8921_l23);
	pr_info("%s: 8921_l23 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l23\", 1.8V) FAILED %d\n", rc);
		goto enable_cam2_d1v8_fail;
	}
	mdelay(60);

	/* I/O */
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(5);

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
	gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
	pr_info("%s: gpio_cam_d1v2_en\n", __func__);
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: gpio_cam_d1v2_en (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.05V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(5);

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

	return rc;


enable_vcm_fail:
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l23);
enable_cam2_d1v8_fail:
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
	}
enable_raw_1v8_fail:
enable_mclk_switch_fail:

	return rc;
}

static int m7china_imx135_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	/* VCM */
	pr_info("%s: 8921_l9 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.05V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(10);

	/* analog */
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = camera_sensor_power_disable(reg_8921_l23);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l23\") FAILED %d\n", rc);
	mdelay(1);
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		mdelay(10);
	}
	// mclk switch
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
	}

	return rc;
}

static struct msm_camera_csi_lane_params imx135_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx135_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx135_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_imx135_led_table[] = {
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

static struct camera_led_info msm_camera_sensor_imx135_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx135_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx135_flash_info = {
	.led_info = &msm_camera_sensor_imx135_led_info,
	.led_est_table = msm_camera_sensor_imx135_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx135_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx135_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */


static struct msm_camera_sensor_flash_data flash_imx135 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};


static struct msm_camera_sensor_info msm_camera_sensor_imx135_data = {
	.sensor_name	= "imx135",
	.camera_power_on = m7china_imx135_vreg_on,
	.camera_power_off = m7china_imx135_vreg_off,
	.camera_yushanii_probed = m7china_yushanii_probed,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx135,
	.sensor_platform_info = &sensor_imx135_board_info,
	.gpio_conf = &imx135_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx135_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_IMX135

#ifdef CONFIG_VD6869
static int m7china_vd6869_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
			goto enable_raw_1v8_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		mdelay(5);
	}

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
		goto enable_cam2_d1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(60);

	/* I/O */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(5);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2900000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: gpio_cam_d1v2_en (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2850000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);

	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 3.1V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	return rc;


enable_vcm_fail:
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	}
enable_cam2_d1v8_fail:
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
	}
enable_raw_1v8_fail:

	return rc;
}

static int m7china_vd6869_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	pr_info("%s: 8921_l9 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(10);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("%s: 8921_l8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(1);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}

	}

	return rc;
}

static struct msm_camera_csi_lane_params vd6869_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_vd6869_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,	/* HTC_START steven pixel order select from board info 20121204 */
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#else
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#endif
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &vd6869_csi_lane_params,
	.sensor_mount_angle = ANGLE_90,
	.ews_enable = false,/*HTC chuck add ews enable*/
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_vd6869_led_table[] = {
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL1,
//             .current_ma = 150,
//             .lumen_value = 150,
//             .min_step = 50,
//             .max_step = 70
//     },
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,//200,
                .lumen_value = 1500,//250,//245,//240,   //mk0118
                .min_step = 20,//23,  //mk0210
                .max_step = 28
        },
                {
                .enable = 1,//1,
                .led_state = FL_MODE_FLASH_LEVEL3,
                .current_ma = 300,//300,
                .lumen_value = 300,//350,
                .min_step = 0,
                .max_step = 19
        },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH_LEVEL4,
                .current_ma = 800,//400,
                .lumen_value = 880,//440,
                .min_step = 25,
                .max_step = 26
        },
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL5,
//             .current_ma = 500,
//             .lumen_value = 500,
//             .min_step = 23,//25,
//             .max_step = 29//26,
//     },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH_LEVEL6,
                .current_ma = 1200,//600,
                .lumen_value = 1250,//625,
                .min_step = 23,
                .max_step = 24
        },
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL7,
//             .current_ma = 700,
//             .lumen_value = 700,
//             .min_step = 21,
//             .max_step = 22
//     },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,//750,
                .lumen_value = 1450,//725,   //mk0217  //mk0221
                .min_step = 0,
                .max_step = 22    //mk0210
        },

                {
                .enable =0,// 2,
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
                .enable = 0,//2,     //mk0210
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,
                .lumen_value = 1450,//725,   //mk0217   //mk0221
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

static struct camera_led_info msm_camera_sensor_vd6869_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 1500,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_vd6869_led_table),
};

static struct camera_flash_info msm_camera_sensor_vd6869_flash_info = {
	.led_info = &msm_camera_sensor_vd6869_led_info,
	.led_est_table = msm_camera_sensor_vd6869_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_vd6869_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_vd6869_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */


static struct msm_camera_sensor_flash_data flash_vd6869 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

/* HTC_START 20130329 */
#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
static struct msm_actuator_info *vd6869_actuator_table[] = {
#if defined(CONFIG_RUMBAS_ACT)
    &rumbas_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_LC898212_ACT)
    &lc898212_actuator_info,
#endif
};
#endif
/* HTC_END */

static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data = {
	.sensor_name	= "vd6869",
	.camera_power_on = m7china_vd6869_vreg_on,
	.camera_power_off = m7china_vd6869_vreg_off,
	.camera_yushanii_probed = m7china_yushanii_probed,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
	.flash_data	= &flash_vd6869,
	.sensor_platform_info = &sensor_vd6869_board_info,
	.gpio_conf = &vd6869_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,

/* HTC_START 20130329 */
#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
	.num_actuator_info_table = ARRAY_SIZE(vd6869_actuator_table),
	.actuator_info_table = &vd6869_actuator_table[0],
#endif
/* HTC_END */

#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_vd6869_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_VD6869

#ifdef CONFIG_OV4688

static int m7china_ov4688_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
			goto enable_raw_1v8_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		mdelay(5);
	}

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
		goto enable_cam2_d1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(60);

	/* I/O */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(5);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2900000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: gpio_cam_d1v2_en (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2850000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);

	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 3.1V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	// vcm pd
	rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,1);
	if (rc < 0) {
		goto enable_ov4688_vcm_pd_fail;
	}

	return rc;

enable_ov4688_vcm_pd_fail:
	camera_sensor_power_disable(reg_8921_l9);

enable_vcm_fail:
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	}
enable_cam2_d1v8_fail:
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
	}
enable_raw_1v8_fail:

	return rc;
}

static int m7china_ov4688_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	// vcm pd
	rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);
	if (rc < 0)
		pr_err("Set VCM PD fail\n");
	mdelay(10);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	pr_info("%s: 8921_l9 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(10);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("%s: 8921_l8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(1);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}

	}

	return rc;
}

static struct msm_camera_csi_lane_params ov4688_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_ov4688_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,	/* HTC_START steven pixel order select from board info 20121204 */
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#else
	.mirror_flip = CAMERA_SENSOR_FLIP, // HTC pg 20130219 sensor angle
#endif
	.sensor_reset_enable = 0,
	.sensor_reset = 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &ov4688_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_ov4688_led_table[] = {
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL1,
//             .current_ma = 150,
//             .lumen_value = 150,
//             .min_step = 50,
//             .max_step = 70
//     },
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,//200,
                .lumen_value = 1500,//250,//245,//240,   //mk0118
                .min_step = 20,//23,  //mk0210
                .max_step = 28
        },
                {
                .enable = 1,//1,
                .led_state = FL_MODE_FLASH_LEVEL3,
                .current_ma = 300,//300,
                .lumen_value = 300,//350,
                .min_step = 0,
                .max_step = 19
        },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH_LEVEL4,
                .current_ma = 800,//400,
                .lumen_value = 880,//440,
                .min_step = 25,
                .max_step = 26
        },
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL5,
//             .current_ma = 500,
//             .lumen_value = 500,
//             .min_step = 23,//25,
//             .max_step = 29//26,
//     },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH_LEVEL6,
                .current_ma = 1200,//600,
                .lumen_value = 1250,//625,
                .min_step = 23,
                .max_step = 24
        },
//             {
//             .enable = 0,
//             .led_state = FL_MODE_FLASH_LEVEL7,
//             .current_ma = 700,
//             .lumen_value = 700,
//             .min_step = 21,
//             .max_step = 22
//     },
                {
                .enable = 0,//1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,//750,
                .lumen_value = 1450,//725,   //mk0217  //mk0221
                .min_step = 0,
                .max_step = 22    //mk0210
        },

                {
                .enable =0,// 2,
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
                .enable = 0,//2,     //mk0210
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,
                .lumen_value = 1450,//725,   //mk0217   //mk0221
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


static struct camera_led_info msm_camera_sensor_ov4688_led_info = {
        .enable = 1,
        .low_limit_led_state = FL_MODE_TORCH,
        .max_led_current_ma = 1500,  //mk0210
        .num_led_est_table = ARRAY_SIZE(msm_camera_sensor_ov4688_led_table),
};


static struct camera_flash_info msm_camera_sensor_ov4688_flash_info = {
	.led_info = &msm_camera_sensor_ov4688_led_info,
	.led_est_table = msm_camera_sensor_ov4688_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_ov4688_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_ov4688_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_ov4688 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};


/* HTC_START 20130621 : Support multiple actuators */
#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
static struct msm_actuator_info *ov4688_actuator_table[] = {
#if defined(CONFIG_RUMBAS_ACT)
    &rumbas_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_LC898212_ACT)
    &lc898212_actuator_info,
#endif
};
#endif
/* HTC_END */

static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data = {
	.sensor_name	= "ov4688",
	.camera_power_on = m7china_ov4688_vreg_on,
	.camera_power_off = m7china_ov4688_vreg_off,
	.camera_yushanii_probed = m7china_yushanii_probed,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov4688,
	.sensor_platform_info = &sensor_ov4688_board_info,
	.gpio_conf = &ov4688_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,

/* HTC_START 20130621 : Support multiple actuators */
#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
	.num_actuator_info_table = ARRAY_SIZE(ov4688_actuator_table),
	.actuator_info_table = &ov4688_actuator_table[0],
#endif
/* HTC_END */

#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif

	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_ov4688_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif//end ov4688
#ifdef CONFIG_IMX091
static int m7china_imx091_vreg_on(void)
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

	if (1) {
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
	}

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	/* HTC_END */
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

static int m7china_imx091_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	if (1) {
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
	}

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	/* HTC_END */
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
	.low_cap_limit		= 14,
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
	.camera_power_on = m7china_imx091_vreg_on,
	.camera_power_off = m7china_imx091_vreg_off,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
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
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx091_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_IMX091


#ifdef CONFIG_S5K3H2YX
static int m7china_s5k3h2yx_vreg_on(void)
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

	if (1) {
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
	}

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	// I2C
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	/* HTC_END */
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

static int m7china_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	if (1) {
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
	}

	/* IO */
#if 0	/* HTC_START - for power sequence */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	/* HTC_END */
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
	.low_cap_limit		= 14,
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
	.camera_power_on = m7china_s5k3h2yx_vreg_on,
	.camera_power_off = m7china_s5k3h2yx_vreg_off,
	.pdata	= &m7china_msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_S5K3H2YX

#ifdef CONFIG_S5K6A1GX
static int m7china_s5k6a1gx_vreg_on(void)
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

static int m7china_s5k6a1gx_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* digital */
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
	.sensor_pwd	= 0,
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
	.sensor_pwd	= 0,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7china_s5k6a1gx_vreg_on,
	.camera_power_off = m7china_s5k6a1gx_vreg_off,
	.pdata	= &m7china_msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a1gx,
	.sensor_platform_info = &sensor_s5k6a1gx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif	//CONFIG_S5K6A1GX

#ifdef CONFIG_AR0260
static int m7china_ar0260_vreg_on(void)
{
	int rc;

	pr_info("%s\n", __func__);

	/* mclk switch */
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
		goto enable_mclk_switch_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 1);
	gpio_free(CAM_PIN_GPIO_CAM_SEL);
	mdelay(1);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		mdelay(1);
	}

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
		goto enable_io_fail_2;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(50);

	/* I/O */
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail_3;
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

	return rc;

enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail_3:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	}
enable_io_fail_2:
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
	}
enable_io_fail:
enable_mclk_switch_fail:

	return rc;
}

static int m7china_ar0260_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* analog */
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(1);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(1);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		mdelay(1);
	}

	// mclk switch
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
	}
	mdelay(1);

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
	.sensor_pwd	= 0,
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
	.sensor_pwd	= 0,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7china_ar0260_vreg_on,
	.camera_power_off = m7china_ar0260_vreg_off,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.pdata	= &m7china_msm_camera_csi_device_data[1],
#else
	.pdata	= &m7china_msm_camera_csi_device_data[0],
#endif
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
#else
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
#endif
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif	//CONFIG_AR0260


#ifdef CONFIG_OV2722
static int m7china_ov2722_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* reset high */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	pr_info("%s: CAM_PIN_GPIO_CAM2_RSTz (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_high_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(2);

	if (check_yushanII_flag() == 0) {
		/* I/O */
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		mdelay(1);
	}

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
		goto enable_io_fail_2;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(60);

	/* I/O */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail_3;
	}
	mdelay(5);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	/* reset low */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	pr_info("%s: CAM_PIN_GPIO_CAM2_RSTz (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_low_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(5);

	return rc;


reset_low_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail_3:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	}
enable_io_fail_2:
	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
	}
enable_io_fail:
reset_high_fail:

	return rc;
}

static int m7china_ov2722_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("%s: 8921_l8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(1);

	if (check_yushanII_flag() == 0) {
		rc = gpio_request(CAM_PIN_PMGPIO_V_RAW_1V8_EN, "rawchip_1v8");
		pr_info("%s: CAM_PIN_PMGPIO_V_RAW_1V8_EN (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_PMGPIO_V_RAW_1V8_EN);
		}
		mdelay(1);
	}

	return rc;
}

static struct msm_camera_csi_lane_params ov2722_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ov2722_board_info = {
	.mount_angle = 270,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_BG,	/* HTC_START steven pixel order select from board info 20121204 */
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= 0,
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
	.sensor_pwd	= 0,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7china_ov2722_vreg_on,
	.camera_power_off = m7china_ov2722_vreg_off,
	.camera_yushanii_probed = m7china_yushanii_probed,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.pdata	= &m7china_msm_camera_csi_device_data[1],
#else
	.pdata	= &m7china_msm_camera_csi_device_data[0],
#endif
	.flash_data	= &flash_ov2722,
	.sensor_platform_info = &sensor_ov2722_board_info,
	.gpio_conf = &ov2722_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
#else
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
#endif
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif	//CONFIG_OV2722


#endif	/* HTC_END synced */

#endif	// for pre-evt no camera
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};


#ifdef CONFIG_I2C
static struct i2c_board_info m7china_camera_i2c_boardinfo_imx135_ar0260[] = {
#ifdef CONFIG_IMX135
		{
		I2C_BOARD_INFO("imx135", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx135_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
		},
#endif
};

static struct i2c_board_info m7china_camera_i2c_boardinfo_imx135_ov2722[] = {
#ifdef CONFIG_IMX135
		{
		I2C_BOARD_INFO("imx135", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx135_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif
};

static struct i2c_board_info m7china_camera_i2c_boardinfo_vd6869_ar0260[] = {
#ifdef CONFIG_VD6869
		{
		I2C_BOARD_INFO("vd6869", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_vd6869_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
		},
#endif
};

static struct i2c_board_info m7china_camera_i2c_boardinfo_vd6869_ov2722[] = {
#ifdef CONFIG_VD6869
		{
		I2C_BOARD_INFO("vd6869", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_vd6869_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif
};

struct i2c_board_info m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722[] = {

#ifdef CONFIG_OV4688
		{
		I2C_BOARD_INFO("ov4688_0x6c", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov4688_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		}
#endif
};

struct i2c_board_info m7_camera_i2c_boardinfo_ov4688_0x20_ov2722[] = {

#ifdef CONFIG_OV4688
		{
		I2C_BOARD_INFO("ov4688_0x20", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov4688_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		}
#endif
};

#endif

enum camera_sensor_id {
	CAMERA_SENSOR_ID_ST_4M,
	CAMERA_SENSOR_ID_OV_4M,
	CAMERA_SENSOR_ID_SONY_13M,
};

enum front_camera_sensor_id {
	CAMERA_SENSOR_ID_AR0260_2M,
	CAMERA_SENSOR_ID_OV2722_2M,
};


int m7china_main_camera_id(void)
{
	int rc = 0;
	int main_camera_id = 0;
	int pull_high_value = 1;
	int pull_low_value = 0;

	struct pm_gpio cam_id_pmic_gpio_start = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_UP_30,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	struct pm_gpio cam_id_pmic_gpio_end = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_DN,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	struct pm_gpio cam_id_pmic_gpio_release = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_NO,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_failed;
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_CAMERA_ID, "CAM_PIN_CAMERA_ID");
	if (rc) {
		pr_err("request camera_id fail %d\n", rc);
		goto request_camid_failed;
	}

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_start);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_start=%d\n", __func__, rc);
		goto config_camid_failed;
	}
	mdelay(1);
	pull_high_value = gpio_get_value(CAM_PIN_CAMERA_ID);

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_end);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_end=%d\n", __func__, rc);
		goto config_camid_failed;
	}
	mdelay(1);
	pull_low_value = gpio_get_value(CAM_PIN_CAMERA_ID);

	if (pull_high_value == 0 && pull_low_value == 0)
		main_camera_id = CAMERA_SENSOR_ID_ST_4M;
	else if (pull_high_value == 1 && pull_low_value == 1)
		main_camera_id = CAMERA_SENSOR_ID_OV_4M;
	else
		main_camera_id = CAMERA_SENSOR_ID_SONY_13M;
	pr_info("pull_high_value = %d pull_low_value = %d main_camera id = %d\n",
		pull_high_value, pull_low_value, main_camera_id);

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_release);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_release=%d\n", __func__, rc);
		goto config_camid_failed;
	}

config_camid_failed:
	gpio_free(CAM_PIN_CAMERA_ID);
request_camid_failed:
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(1);
enable_io_failed:
	return main_camera_id;
}

#if 0
int m7china_front_camera_id(void)
{
	int rc = 0;
	int front_camera_id = 0;
	int read_value = 1;

	struct pm_gpio cam_id_pmic_gpio_start = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_UP_30,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	struct pm_gpio cam_id_pmic_gpio_release = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_NO,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_failed;
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_FRONT_CAMERA_ID, "CAM_PIN_FRONT_CAMERA_ID");
	if (rc) {
		pr_err("request front camera_id fail %d\n", rc);
		goto request_camid_failed;
	}
	mdelay(1);

	rc = pm8xxx_gpio_config(CAM_PIN_FRONT_CAMERA_ID, &cam_id_pmic_gpio_start);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_start=%d\n", __func__, rc);
		goto config_camid_failed;
	}
	mdelay(1);

	read_value = gpio_get_value(CAM_PIN_FRONT_CAMERA_ID);
	if (read_value == 1)
		front_camera_id = CAMERA_SENSOR_ID_OV2722_2M;
	else
		front_camera_id = CAMERA_SENSOR_ID_AR0260_2M;
	pr_info("read_value = %d , front_camera_id = %d\n", read_value, front_camera_id);

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_release);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_release=%d\n", __func__, rc);
		goto config_camid_failed;
	}

config_camid_failed:
	gpio_free(CAM_PIN_FRONT_CAMERA_ID);
request_camid_failed:
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(1);
enable_io_failed:
	return front_camera_id;
}
#endif

/* HTC_START - for HW VCM work-around */
#if defined(CONFIG_RUMBAS_ACT)
void m7china_vcm_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	rc = m7china_rawchip_vreg_on();
	if (rc < 0) {
		pr_err("%s m7china_rawchip_vreg_on failed\n", __func__);
		return;
	}

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
		goto enable_cam2_d1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(60);

	/* I/O */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(5);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2900000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: gpio_cam_d1v2_en (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	/* VCM */
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2850000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 3.1V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);
	return;

enable_vcm_fail:
	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
enable_digital_fail:
	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("%s: 8921_l8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
enable_analog_fail:
	/* I/O */
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
enable_io_fail:
	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	}
enable_cam2_d1v8_fail:
	return;
}

void m7china_vcm_vreg_off(void)
{
	int rc;
	int gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	pr_info("%s: 8921_l9 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	gpio_cam_d1v2_en = CAM_PIN_GPIO_MCAM_D1V2_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(10);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("%s: 8921_l8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	/* I/O */
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	/* 2nd cam D1V8 : V_CAM2_D1V8*/
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V8, "V_CAM2_1V8");
	pr_info("%s: CAM_PIN_GPIO_V_CAM2_D1V8 off (%d)\n", __func__, rc);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V8);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V8, 0);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V8);
	mdelay(1);

	m7china_rawchip_vreg_off();
}
#endif
/* HTC_END */

void __init m7china_init_cam(void)
{
	int main_camera_id = CAMERA_SENSOR_ID_ST_4M;
	int front_camera_id = CAMERA_SENSOR_ID_OV2722_2M;

	pr_info("%s", __func__);
	msm_gpiomux_install(m7china_cam_common_configs,
			ARRAY_SIZE(m7china_cam_common_configs));
	platform_device_register(&msm_camera_server);

	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);

	main_camera_id = m7china_main_camera_id();
#if 0
	front_camera_id = m7china_front_camera_id();
#endif
	pr_info("main_camera id = %d , front_camera_id=%d\n", main_camera_id, front_camera_id);

	pr_info("[CAM] engineerid=%d\n",engineerid);
#ifdef CONFIG_I2C
	if (system_rev == 0) {
		/* XA              : front cam only support OV2722 */
		if (main_camera_id == CAMERA_SENSOR_ID_ST_4M) {
			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7china_camera_i2c_boardinfo_vd6869_ov2722,
				ARRAY_SIZE(m7china_camera_i2c_boardinfo_vd6869_ov2722));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		} else {
			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7china_camera_i2c_boardinfo_imx135_ar0260,
				ARRAY_SIZE(m7china_camera_i2c_boardinfo_imx135_ar0260));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		}
	} else {
		/* XB and after    : front cam support AR0260 and OV2722 by checking FRONT_CAM_ID pin */
		if (main_camera_id == CAMERA_SENSOR_ID_ST_4M) {
			if (front_camera_id == CAMERA_SENSOR_ID_AR0260_2M) {
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7china_camera_i2c_boardinfo_vd6869_ar0260,
					ARRAY_SIZE(m7china_camera_i2c_boardinfo_vd6869_ar0260));
			} else {
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7china_camera_i2c_boardinfo_vd6869_ov2722,
					ARRAY_SIZE(m7china_camera_i2c_boardinfo_vd6869_ov2722));
			}

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		} else if(main_camera_id == CAMERA_SENSOR_ID_OV_4M){
			if(1){/*TODO: This rule need to be well-defined later*/
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7_camera_i2c_boardinfo_ov4688_0x20_ov2722,
					ARRAY_SIZE(m7_camera_i2c_boardinfo_ov4688_0x20_ov2722));
			}else{
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722,
					ARRAY_SIZE(m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722));
			}

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		} else {
			if (front_camera_id == CAMERA_SENSOR_ID_AR0260_2M) {
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7china_camera_i2c_boardinfo_imx135_ar0260,
					ARRAY_SIZE(m7china_camera_i2c_boardinfo_imx135_ar0260));
			}else{
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7china_camera_i2c_boardinfo_imx135_ov2722,
					ARRAY_SIZE(m7china_camera_i2c_boardinfo_imx135_ov2722));
			}

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		}
	}
#endif
}

#endif	//CONFIG_MSM_CAMERA



