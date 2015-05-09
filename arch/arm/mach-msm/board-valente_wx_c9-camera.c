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
#include "board-valente_wx_c9.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4	//board-8960.h
static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power);
static int camera_sensor_power_disable(struct regulator *sensor_power);
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

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
		.ab  = 96215040,
		.ib  = 378224640,
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
		.ab  = 342150912,
		.ib  = 1361968128,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 207747072,
		.ib  = 489756672,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 60318720,
		.ib  = 150796800,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 147045888,
		.ib  = 588183552,
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
		.ab  = 263678976,
		.ib  = 659197440,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 319044096,
		.ib  = 1271531520,
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
		.ab  = 239708160,
		.ib  = 599270400,
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

static int valente_wx_c9_csi_vreg_on(void);
static int valente_wx_c9_csi_vreg_off(void);

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = valente_wx_c9_csi_vreg_on,
		.camera_csi_off = valente_wx_c9_csi_vreg_off,
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
		.camera_csi_on = valente_wx_c9_csi_vreg_on,
		.camera_csi_off = valente_wx_c9_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
int flashlight_control(int mode)
{
#ifdef CONFIG_FLASHLIGHT_TPS61310
/* HTC_START Turn off backlight when flash on */
	int	rc;
	static int brightness = 255;
	static int backlight_off = 0;

	pr_info("[CAM] %s, linear led, mode %d backlight_off %d", __func__, mode, backlight_off);

	if (mode != FL_MODE_PRE_FLASH && mode != FL_MODE_OFF) {
		if (!backlight_off) {
			/* restore backlight brightness value first */
			brightness = led_brightness_value_get("lcd-backlight");
			if (brightness >= 0 && brightness <= 255) {
				pr_info("[CAM] %s, Turn off backlight before flashlight, brightness %d", __func__, brightness);
				led_brightness_value_set("lcd-backlight", 0);
				backlight_off = 1;
			} else
				pr_err("[CAM] %s, Invalid brightness value!! brightness %d", __func__, brightness);
		}
	}

	rc = tps61310_flashlight_control(mode);

	if (mode == FL_MODE_PRE_FLASH || mode == FL_MODE_OFF) {
		if(backlight_off) {
			pr_info("[CAM] %s, Turn on backlight after flashlight, brightness %d", __func__, brightness);
			led_brightness_value_set("lcd-backlight", brightness);
			backlight_off = 0;
		}
	}

	return rc;
/* HTC_END */
#else
	return 0;
#endif
}


static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};
#endif  //CONFIG_MSM_CAMERA_FLASH
#ifdef CONFIG_RAWCHIP
static int valente_wx_c9_use_ext_1v2(void)
{
		return 1;
}

#define VALENTE_WX_C9_SYS_V_RAW_1V2_EN PM8921_GPIO_PM_TO_SYS(VALENTE_WX_C9_PMGPIO_V_RAW_1V2_EN)
#define VALENTE_WX_C9_SYS_V_RAW_1V8_EN PM8921_GPIO_PM_TO_SYS(VALENTE_WX_C9_PMGPIO_V_RAW_1V8_EN)
static struct regulator *reg_8921_l2 = NULL;
static struct regulator *reg_8921_l9 = NULL;
static struct regulator *reg_8921_l17 = NULL;
static struct regulator *reg_8921_lvs6 = NULL;

static int valente_wx_c9_rawchip_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);
	/* PM8921_GPIO_PM_TO_SYS(VALENTE_WX_C9_V_RAW_1V8_EN) 1800000 */
	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("[CAM] rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V8_EN, rc);
		goto enable_1v8_fail;
	}
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, 1);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V8_EN);

	mdelay(5);

	/* PM8921_GPIO_PM_TO_SYS(VALENTE_WX_C9_SYS_V_RAW_1V2_EN) 1200000 */
	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, "V_RAW_1V2_EN");
	if (rc) {
		pr_err("[CAM] rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V2_EN, rc);
		goto enable_1v2_fail;
	}
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, 1);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V2_EN);

	//if (system_rev >= 1) { /* for XB */
		if (valente_wx_c9_use_ext_1v2()) { /* use external 1v2 for HW workaround */
			mdelay(1);

			rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, "rawchip");
			pr_info("[CAM]rawchip external 1v2 gpio_request,%d\n", VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
			if (rc < 0) {
				pr_err("GPIO(%d) request failed", VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
				goto enable_ext_1v2_fail;
			}
			gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, 1);
			gpio_free(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
		}
	//}

	return rc;

enable_ext_1v2_fail:
	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, "V_RAW_1V2_EN");
	if (rc)
		pr_err("[CAM] rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V2_EN, rc);
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, 0);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V2_EN);
enable_1v2_fail:
	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("[CAM] rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V8_EN, rc);
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, 0);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V8_EN);
enable_1v8_fail:
	return rc;
}

static int valente_wx_c9_rawchip_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	//if (system_rev >= 1) { /* for XB */
		if (valente_wx_c9_use_ext_1v2()) { /* use external 1v2 for HW workaround */
			rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, "rawchip");
			if (rc)
				pr_err("[CAM] rawchip off(\
					\"gpio %d\", 1.2V) FAILED %d\n",
					VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, rc);
			gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, 0);
			gpio_free(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);

			mdelay(1);
		}
	//}

	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, "V_RAW_1V2_EN");
	if (rc)
		pr_err("[CAM] rawchip off(\
			\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V2_EN, rc);
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V2_EN, 0);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V2_EN);

	mdelay(5);

	rc = gpio_request(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("[CAM] rawchip off\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_SYS_V_RAW_1V8_EN, rc);
	gpio_direction_output(VALENTE_WX_C9_SYS_V_RAW_1V8_EN, 0);
	gpio_free(VALENTE_WX_C9_SYS_V_RAW_1V8_EN);

       return rc;
}

static struct msm_camera_rawchip_info msm_rawchip_board_info = {
	.rawchip_reset	= VALENTE_WX_C9_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(VALENTE_WX_C9_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(VALENTE_WX_C9_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = valente_wx_c9_rawchip_vreg_on,
	.camera_rawchip_power_off = valente_wx_c9_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = valente_wx_c9_use_ext_1v2,
};

struct platform_device msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &msm_rawchip_board_info,
	},
};

#endif  //CONFIG_RAWCHIP

static uint16_t msm_cam_gpio_tbl[] = {
	VALENTE_WX_C9_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	VALENTE_WX_C9_GPIO_CAM_MCLK1,
	VALENTE_WX_C9_GPIO_RAW_INTR0,
	VALENTE_WX_C9_GPIO_RAW_INTR1,
	VALENTE_WX_C9_GPIO_MCAM_SPI_CLK,
	VALENTE_WX_C9_GPIO_MCAM_SPI_CS0,
	VALENTE_WX_C9_GPIO_MCAM_SPI_DI,
	VALENTE_WX_C9_GPIO_MCAM_SPI_DO,
};


static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};

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

static int valente_wx_c9_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int valente_wx_c9_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}


#ifdef CONFIG_S5K3H2YX
static int valente_wx_c9_s5k3h2yx_vreg_on(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l17", 2850000, &reg_8921_l17);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l17\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	/* analog */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	/* digital */
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc) {
		pr_err("[CAM] sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l17);
enable_vcm_fail:
	return rc;
}

static int valente_wx_c9_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);
	mdelay(1);

	/* digital */
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(VALENTE_WX_C9_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	#if 0
	if (system_rev >= 1) {
		rc = camera_sensor_power_disable(reg_8921_l17);
		if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
				(\"8921_l17\") FAILED %d\n", rc);
	}
	#endif
	return rc;
}

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = VALENTE_WX_C9_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params s5k3h2yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};
#define VALENTE_WX_C9_SYS_CAM_PWDN PM8921_GPIO_PM_TO_SYS(VALENTE_WX_C9_PMGPIO_CAM_PWDN)
static struct msm_camera_sensor_platform_info sensor_s5k3h2yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= VALENTE_WX_C9_SYS_CAM_PWDN,
	.vcm_pwd	= VALENTE_WX_C9_GPIO_CAM_VCM_PD,
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
		.min_step = 58,//23,  //mk0210
		.max_step = 256
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 54,
		.max_step = 57
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 50,
		.max_step = 53
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
		.lumen_value = 625,//600,
		.min_step = 52,//23,
		.max_step = 55//24
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
		.max_step = 45    //mk0210
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
	.max_led_current_ma = 750,
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
	.flash_src	= &msm_flash_src,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = valente_wx_c9_s5k3h2yx_vreg_on,
	.camera_power_off = valente_wx_c9_s5k3h2yx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
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

#ifdef CONFIG_IMX175_2LANE
static int valente_wx_c9_imx175_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l17", 2850000, &reg_8921_l17);

	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

    /* VANA */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vana_fail;
	}
	mdelay(1);

	/* VDIG */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_vdig_fail;
	}

	return rc;

enable_vdig_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vana_fail:
enable_vcm_fail:
	return rc;
}

static int valente_wx_c9_imx175_vreg_off(void)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);

	/* VANA */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);
	mdelay(1);

	/* VDIG */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* VCM */
	if (valente_wx_c9_use_ext_1v2() == 0)/*XA only*/
		rc = camera_sensor_power_disable(reg_8921_l9);

	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX175_ACT
static struct i2c_board_info imx175_actuator_i2c_info = {
	I2C_BOARD_INFO("imx175_act", 0x11),
};

static struct msm_actuator_info imx175_actuator_info = {
	.board_info     = &imx175_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = VALENTE_WX_C9_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params imx175_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx175_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= VALENTE_WX_C9_SYS_CAM_PWDN,
	.vcm_pwd	= VALENTE_WX_C9_GPIO_CAM_VCM_PD,
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
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 58,//23,  //mk0210
		.max_step = 256
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 54,
		.max_step = 57
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 50,
		.max_step = 53
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 46,
		.max_step = 49
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 45    //mk0210
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
	.low_cap_limit		= 14,
	.low_cap_limit_dual		= 30, //for power restrict in dual mode
	.flash_info             = &msm_camera_sensor_imx175_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_imx175 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = valente_wx_c9_imx175_vreg_on,
	.camera_power_off = valente_wx_c9_imx175_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_IMX175_ACT
	.actuator_info = &imx175_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};
#endif	//CONFIG_IMX175_2LANE

#ifdef CONFIG_S5K6AAFX
static int valente_wx_c9_s5k6aafx_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);
	/* analog */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}

	mdelay(1);
	/* digital */
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, "V_CAM_D1V8");
	if (rc) {
		pr_err("[CAM] sensor_power_enable\
			(\"gpio %d\", 1.8V) FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, 1);
	gpio_free(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN);
	mdelay(1);
	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);
	 /*STANDBY */
	rc = gpio_request(VALENTE_WX_C9_GPIO_CAM2_STBz, "CAM2_STB#");
	if (rc) {
		pr_err("[CAM] sensor_power_enable\
			(\"gpio %d\") FAILED %d\n",
			VALENTE_WX_C9_GPIO_CAM2_STBz, rc);
		goto enable_standby_fail;
	}
	gpio_direction_output(VALENTE_WX_C9_GPIO_CAM2_STBz, 1);
	gpio_free(VALENTE_WX_C9_GPIO_CAM2_STBz);

	return rc;

enable_standby_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_io_fail:
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, "V_CAM_D1V8");
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"gpio %d\", 1.8V) FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, rc);
	else {
		gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, 0);
		gpio_free(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_analog_fail:
	return rc;
}

static int valente_wx_c9_s5k6aafx_vreg_off(void)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);
	/* Standby */
	rc = gpio_request(VALENTE_WX_C9_GPIO_CAM2_STBz, "CAM2_STB#");
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"gpio %d\") FAILED %d\n",
			VALENTE_WX_C9_GPIO_CAM2_STBz, rc);
	else {
		gpio_direction_output(VALENTE_WX_C9_GPIO_CAM2_STBz, 0);
		gpio_free(VALENTE_WX_C9_GPIO_CAM2_STBz);
	}
	mdelay(1);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	/* Digital*/
	rc = gpio_request(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, "V_CAM_D1V8");
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"gpio %d\") FAILED %d\n",
			VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, rc);
	else {
		gpio_direction_output(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN, 0);
		gpio_free(VALENTE_WX_C9_GPIO_V_CAM2IO_1V8_EN);
	}
	mdelay(1);

	/* Analog */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);
	return rc;
}

static struct msm_camera_csi_lane_params s5k6aafx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_s5k6aafx_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= VALENTE_WX_C9_GPIO_CAM2_RSTz,
	.sensor_pwd	= VALENTE_WX_C9_GPIO_CAM2_STBz,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &s5k6aafx_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_s5k6aafx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6aafx_data = {
	.sensor_name	= "s5k6aafx",
	.sensor_reset	= VALENTE_WX_C9_GPIO_CAM2_RSTz,
	.sensor_pwd	= VALENTE_WX_C9_GPIO_CAM2_STBz,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = valente_wx_c9_s5k6aafx_vreg_on,
	.camera_power_off = valente_wx_c9_s5k6aafx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6aafx,
	.sensor_platform_info = &sensor_s5k6aafx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE, /* HTC_START_Simon.Ti_Liu_20120712_Enhance_bypass */
	.mirror_mode = 1,
	.power_down_disable = false, /* true: disable pwd down function */
	.full_size_preview = true, /* true: use full size preview */
};
#endif

static void config_cam_id(int status)
{
	static uint32_t cam_id_gpio_start[] = {
		GPIO_CFG(VALENTE_WX_C9_GPIO_MAIN_CAM_ID, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	};

	static uint32_t cam_id_gpio_end[] = {
		GPIO_CFG(VALENTE_WX_C9_GPIO_MAIN_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	};
	pr_info("config_cam_id(): status=%d\n",status);
	if(status)
		gpio_tlmm_config(cam_id_gpio_start[0], GPIO_CFG_ENABLE);
	else
		gpio_tlmm_config(cam_id_gpio_end[0], GPIO_CFG_ENABLE);
}

#ifdef CONFIG_I2C
static struct i2c_board_info msm_camera_boardinfo[] = {
#ifdef CONFIG_S5K3H2YX
	{
	I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
	.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
#endif
#ifdef CONFIG_S5K6AAFX
	{
	I2C_BOARD_INFO("s5k6aafx", 0x5a >> 1), /* COB type */
	.platform_data = &msm_camera_sensor_s5k6aafx_data,
	},
#endif
};

struct msm_camera_board_info camera_board_info = {
	.board_info = msm_camera_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm_camera_boardinfo),
};

static struct i2c_board_info msm_camera_boardinfo_2nd[] = {
#ifdef CONFIG_IMX175_2LANE
	{
	I2C_BOARD_INFO("imx175", 0x20 >> 1),
	.platform_data = &msm_camera_sensor_imx175_data,
	},
#endif
#ifdef CONFIG_S5K6AAFX
	{
	I2C_BOARD_INFO("s5k6aafx", 0x5a >> 1), /* COB type */
	.platform_data = &msm_camera_sensor_s5k6aafx_data,
	},
#endif
};

struct msm_camera_board_info camera_board_info_2nd = {
	.board_info = msm_camera_boardinfo_2nd,
	.num_i2c_board_info = ARRAY_SIZE(msm_camera_boardinfo_2nd),
};

#endif  //CONFIG_I2C

void __init msm8960_init_cam(void)
{
	pr_info("%s", __func__);

	config_cam_id(1); /* detect camera sensor start*/
	msleep(2);

	if (gpio_get_value(VALENTE_WX_C9_GPIO_MAIN_CAM_ID) == 0){
		i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
			camera_board_info.board_info,
			camera_board_info.num_i2c_board_info);
	}else{ /* for 2nd Source */
		i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
			camera_board_info_2nd.board_info,
			camera_board_info_2nd.num_i2c_board_info);
	}
	config_cam_id(0); /* detect camera sensor end*/

	platform_device_register(&msm_rawchip_device);

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}


#endif	//CONFIG_MSM_CAMERA
