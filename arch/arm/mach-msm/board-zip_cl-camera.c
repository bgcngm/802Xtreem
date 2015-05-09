/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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
#include "board-8930.h"
#include "board-zip_cl.h"
#include <asm/setup.h>

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#define MSM_V_CAM_D1V8_EN 0// FIXME
#define CAM_PIN_GPIO_V_CAM_D1V8_EN	MSM_V_CAM_D1V8_EN

/* #define CAM_PIN_GPIO_V_CAM2_D1V2_EN	V_CAM2_D1V2_EN - use CAM_D1V8 instead */
/* #define CAM_PIN_GPIO_V_RAW_1V8_EN	V_RAW_1V8_EN - use CAM_D1V8 instead */

#define CAM_PIN_GPIO_V_RAW_1V2_EN	MSM_RAW_1V2_EN
#define CAM_PIN_GPIO_V_RAW_1V15_EN MSM_RAW_1V15_EN
#define CAM_PIN_GPIO_V_RAW_1V8_EN MSM_RAW_1V8_EN
#define CAM_PIN_GPIO_V_CAM_D1V2_EN	MSM_CAM_D1V2_EN

#define CAM_PIN_GPIO_RAW_RSTN	MSM_RAW_RSTN
#define CAM_PIN_GPIO_RAW_INTR0	MSM_RAW_INTR0
#define CAM_PIN_GPIO_RAW_INTR1	MSM_RAW_INTR1
#define CAM_PIN_GPIO_CAM_MCLK0	MSM_CAM_MCLK0
#define CAM_PIN_GPIO_CAM_MCLK1	MSM_CAM_MCLK1	/*CAMIF_MCLK*/

#define CAM_PIN_GPIO_CAM_I2C_DAT	MSM_CAM_I2C_SDA	/*CAMIF_I2C_DATA*/
#define CAM_PIN_GPIO_CAM_I2C_CLK	MSM_CAM_I2C_SCL	/*CAMIF_I2C_CLK*/

#define CAM_PIN_GPIO_MCAM_SPI_CLK	MSM_MCAM_SPI_CLK_CPU
#define CAM_PIN_GPIO_MCAM_SPI_CS0	MSM_MCAM_SPI_CS0
#define CAM_PIN_GPIO_MCAM_SPI_DI	MSM_MCAM_SPI_DI
#define CAM_PIN_GPIO_MCAM_SPI_DO	MSM_MCAM_SPI_DO


/* Board XA */
#define CAM_PIN_GPIO_CAM_PWDN_XA			MSM_CAM_PWDN
#define CAM_PIN_GPIO_V_CAMIO_1V8_EN_XA		MSM_CAMIO_1V8_EN
/* Default set to Board XA */
static int CAM_PIN_GPIO_V_CAMIO_1V8_EN = CAM_PIN_GPIO_V_CAMIO_1V8_EN_XA;


#define CAM_PIN_GPIO_CAM_VCM_PD		MSM_CAM_VCM_PD

#define CAM_PIN_GPIO_CAM2_RSTz		MSM_CAM2_RSTz
#define CAM_PIN_GPIO_CAM2_STANDBY	MSM_CAM2_STANDBY
#define CAM_PIN_GPIO_ID MSM_CAM_ID

#ifdef CONFIG_MSM_CAMERA_FLASH
#if defined(CONFIG_IMX175) || defined(CONFIG_IMX135) || defined(CONFIG_OV4688)|| defined(CONFIG_VD6869) || defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX)
int m7_flashlight_control(int mode)
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
	.camera_flash = m7_flashlight_control,
};
#endif
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
};


static struct msm_gpiomux_config msm8930_cam_common_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,	// 5
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,	// 4
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},

	/* I2C gsbi 4 */
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,	// 20
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,	// 21
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0,	// 69
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], /*I(NP) 2MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[8], /*I(L) 2MA*/
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1,	// 50
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

#ifdef CONFIG_MSM_CAMERA
#define VFE_CAMIF_TIMER1_GPIO 3
#define VFE_CAMIF_TIMER2_GPIO 1

#ifdef CONFIG_MSM_CAMERA_FLASH
/*
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = VFE_CAMIF_TIMER1_GPIO,
	._fsrc.ext_driver_src.led_flash_en = VFE_CAMIF_TIMER2_GPIO,
};
*/
#endif
#endif

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
		.ab  = 483063040,
		.ib  = 1832252160,
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
		.ab  = 700000000,
		.ib  = 3749488640U,
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
		.ab  = 200000000,
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


#if defined(CONFIG_S5K4E5YX) || defined(CONFIG_MT9V113) || defined(CONFIG_OV5693)|| defined(CONFIG_VD6869) || defined (CONFIG_OV4688)\
	|| defined(CONFIG_S5K6A2YA)
static struct regulator *reg_8038_l2;	/* VREG_MIPI_1V2 */
static struct regulator *reg_8038_l17;	/* V_CAM_VCM2V8 */
static struct regulator *reg_8038_l8;	/* V_CAM_A2V8 */

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

	if (strcmp(power, "8038_l17") == 0) {
		regulator_set_optimum_mode(*sensor_power, 10000);
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
#endif

static int msm8930_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8038_l2", 1200000, &reg_8038_l2);
}

static int msm8930_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8038_l2);
}

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = msm8930_csi_vreg_on,
		.camera_csi_off = msm8930_csi_vreg_off,
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
		.camera_csi_on = msm8930_csi_vreg_on,
		.camera_csi_off = msm8930_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

int gpio_get(int gpio, int* value)
{
    int rc=0;
	rc = gpio_request(gpio, "gpio");
    if (rc < 0) {
		pr_err("get gpio(%d) fail", gpio);
        return rc;
	}
	*value = gpio_get_value(gpio);
	gpio_free(gpio);

    return rc;
}

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

// HTC_START pg 20130208 raw2 init
#ifdef CONFIG_RAWCHIPII

static int msm8930_rawchip_vreg_on(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V8_EN,1);
	if (rc<0)
	    return rc;

	mdelay(5);

	rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V2_EN,1);
	if (rc<0)
        goto RAW_FAIL_1V2;


    rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V15_EN,1);
    if (rc<0)
        goto RAW_FAIL_1V15;

    return rc;

RAW_FAIL_1V15:
    gpio_set (CAM_PIN_GPIO_V_RAW_1V2_EN,0);

RAW_FAIL_1V2:
    gpio_set (CAM_PIN_GPIO_V_CAM_D1V8_EN,0);


	return rc;
}

static int msm8930_rawchip_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V8_EN,0);
	rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V2_EN,0);
	rc = gpio_set (CAM_PIN_GPIO_V_RAW_1V15_EN,0);

	return rc;
}

static struct msm_camera_rawchip_info msm8930_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = msm8930_rawchip_vreg_on,
	.camera_rawchip_power_off = msm8930_rawchip_vreg_off,
};

struct platform_device msm8930_msm_rawchip_device = {
	.name	= "yushanII",
	.dev	= {
		.platform_data = &msm8930_msm_rawchip_board_info,
	},
};
#endif
// HTC_END pg 20130208 raw2 init

#ifdef CONFIG_RAWCHIP
static int msm8930_use_ext_1v2(void)
{
	return 1;
}

static int msm8930_rawchip_vreg_on(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* V_RAW_1V8 - use D1V8 instead */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("rawchip 1v8 gpio_request, %d rc(%d)\n", CAM_PIN_GPIO_V_CAM_D1V8_EN, rc);
	if (rc < 0) {
		pr_err("rawchip on (\"gpio %d\", 1.8V) FAILED", CAM_PIN_GPIO_V_CAM_D1V8_EN);
		goto enable_rawchip_1v8_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	mdelay(1);

	/* V_RAW_1V2 */
	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V2_EN, "_V_RAW_1V2_EN");
	pr_info("rawchip 1v2 gpio_request, %d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V2_EN, rc);
	if (rc < 0) {
		pr_err("rawchip on (\"gpio %d\", 1.2V) FAILED", CAM_PIN_GPIO_V_RAW_1V2_EN);
		goto enable_rawchip_1v2_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_RAW_1V2_EN);

	return rc;

enable_rawchip_1v2_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	if (rc < 0)
		pr_err("rawchip off (\"gpio %d\", 1.8V) FAILED", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
enable_rawchip_1v8_fail:
	return rc;

}

static int msm8930_rawchip_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V2_EN, "V_RAW_1V2_EN");
	pr_info("rawchip 1v2 gpio_request, %d\n", CAM_PIN_GPIO_V_RAW_1V2_EN);
	if (rc < 0)
		pr_err("rawchip off (\"gpio %d\", 1.2V) FAILED", CAM_PIN_GPIO_V_RAW_1V2_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V2_EN);
	}
	udelay(50);

	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("rawchip 1v8 gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0)
		pr_err("rawchip off (\"gpio %d\", 1.8V) FAILED", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}

	return rc;
}

static struct msm_camera_rawchip_info zip_cl_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = msm8930_rawchip_vreg_on,
	.camera_rawchip_power_off = msm8930_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = msm8930_use_ext_1v2,
};

struct platform_device zip_cl_msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &zip_cl_msm_rawchip_board_info,
	},
};
#endif

static uint16_t msm_cam_gpio_tbl[] = {
	CAM_PIN_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	CAM_PIN_GPIO_CAM_MCLK1,
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

static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};

#ifdef CONFIG_S5K4E5YX
static int msm8930_s5k4e5yx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8038_l17", 2800000, &reg_8038_l17);
	pr_info("vcm sensor_power_enable(\"8038_l17\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l17\", 2.8V) FAILED %d\n", rc);
		goto enable_s5k4e5yx_vcm_fail;
	}
	udelay(50);

	/* digital - Rawchip init already */
	/*rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
		goto enable_s5k4e5yx_digital_fail;
	}

	gpio_tlmm_config(
		GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	udelay(50); */

	/* analog */
	rc = camera_sensor_power_enable("8038_l8", 2800000, &reg_8038_l8);
	pr_info("analog sensor_power_enable(\"8038_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_s5k4e5yx_analog_fail;
	}
	udelay(50);

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
		goto enable_s5k4e5yx_io_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	udelay(50);

	return rc;

enable_s5k4e5yx_io_fail:
	camera_sensor_power_disable(reg_8038_l8);
enable_s5k4e5yx_analog_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
/*enable_s5k4e5yx_digital_fail:
	camera_sensor_power_disable(reg_8038_l17);*/
enable_s5k4e5yx_vcm_fail:
	return rc;
}

static int msm8930_s5k4e5yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8038_l8);
	pr_info("sensor_power_disable(\"8038_l8\") == %d\n", rc);
	if (rc < 0)
		pr_err("sensor_power_disable\(\"8038_l8\") FAILED %d\n", rc);
	udelay(50);

	/* digital - Close by rawchip */
	/*rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_tlmm_config(
			GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
	udelay(900); */

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}
	udelay(50);

	/* VCM */
	if (reg_8038_l17 != NULL)
		regulator_set_optimum_mode(reg_8038_l17, 9000);
	rc = camera_sensor_power_disable(reg_8038_l17);
	pr_info("sensor_power_disable(\"8038_l17\") == %d\n", rc);
	if (rc < 0)
		pr_err("sensor_power_disable\(\"8038_l17\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_S5K4E5YX_ACT
static struct i2c_board_info s5k4e5yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k4e5yx_act", 0x0C),
};

static struct msm_actuator_info s5k4e5yx_actuator_info = {
	.board_info     = &s5k4e5yx_actuator_i2c_info,
	.bus_id         = MSM_8930_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params s5k4e5yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_s5k4e5yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN_XA,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &s5k4e5yx_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_s5k4e5yx_led_table[] = {
	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 50,
		.max_step = 70
	},*/
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
	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 23,//25,
		.max_step = 29//26,
	},*/
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},

	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL7,
		.current_ma = 700,
		.lumen_value = 700,
		.min_step = 21,
		.max_step = 22
	},*/
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
	},
};


static struct camera_led_info msm_camera_sensor_s5k4e5yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k4e5yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k4e5yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k4e5yx_led_info,
	.led_est_table = msm_camera_sensor_s5k4e5yx_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_s5k4e5yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info			= &msm_camera_sensor_s5k4e5yx_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

#ifdef CONFIG_MSM_CAMERA_FLASH
int msm8930_flashlight_control_s5k4e5yx(int mode)
{
pr_info("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_camera_flash_src_s5k4e5yx = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = msm8930_flashlight_control_s5k4e5yx,
};
#endif

static struct msm_camera_sensor_flash_data flash_s5k4e5yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src_s5k4e5yx,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e5yx_data = {
	.sensor_name	= "s5k4e5yx",
	.camera_power_on = msm8930_s5k4e5yx_vreg_on,
	.camera_power_off = msm8930_s5k4e5yx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k4e5yx,
	.sensor_platform_info = &sensor_s5k4e5yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_S5K4E5YX_ACT
	.actuator_info = &s5k4e5yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_s5k4e5yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_S5K4E5YX

#ifdef CONFIG_MT9V113
static int msm8930_mt9v113_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8038_l17", 2800000, &reg_8038_l17);
	pr_info("sensor_power_enable(\"8038_l17\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l17\", 2.8V) FAILED %d\n", rc);
		goto enable_mt9v113_vcm_fail;
	}
	udelay(50);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "mt9v113");
	pr_info("reset pin gpio_request, %d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto enable_mt9v113_rst_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
		goto enable_mt9v113_digital_fail;
	}
	/*gpio_tlmm_config(
		GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);*/
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);

	mdelay(1);

	/* analog */
	rc = camera_sensor_power_enable("8038_l8", 2800000, &reg_8038_l8);
	pr_info("sensor_power_enable(\"8038_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_mt9v113_analog_fail;
	}
	udelay(50);

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
		goto enable_mt9v113_io_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	udelay(50);

	/* VCM PD */ /* TODO */
	rc = gpio_request(CAM_PIN_GPIO_CAM_VCM_PD, "CAM_VCM_PD");
	pr_info("vcm pd gpio_request, %d\n", CAM_PIN_GPIO_CAM_VCM_PD);
	if (rc < 0 && rc != -EBUSY) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM_VCM_PD);
		goto enable_mt9v113_vcm_pd_fail;
	} else {
		gpio_direction_output(CAM_PIN_GPIO_CAM_VCM_PD, 1);
		gpio_free(CAM_PIN_GPIO_CAM_VCM_PD);
		rc = 0;
	}

	return rc;

enable_mt9v113_vcm_pd_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}
enable_mt9v113_io_fail:
	camera_sensor_power_disable(reg_8038_l8);
enable_mt9v113_analog_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
enable_mt9v113_digital_fail:
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "mt9v113");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
enable_mt9v113_rst_fail:
	camera_sensor_power_disable(reg_8038_l17);
enable_mt9v113_vcm_fail:
	return rc;

}

static int msm8930_mt9v113_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}
	udelay(50);

	/* analog */
	pr_info("sensor_power_disable(\"8038_l8\") == %d\n", rc);
	rc = camera_sensor_power_disable(reg_8038_l8);
	if (rc < 0)
		pr_err("sensor_power_disable(\"reg_8038_l8\") FAILED %d\n", rc);
	udelay(50);

	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		/*gpio_tlmm_config(
			GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);*/
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
	udelay(50);

#if 1
	/* VCM PD */ /* TODO */
	rc = gpio_request(CAM_PIN_GPIO_CAM_VCM_PD, "CAM_VCM_PD");
	pr_info("vcm pd gpio_request, %d\n", CAM_PIN_GPIO_CAM_VCM_PD);
	if (rc < 0 && rc != -EBUSY) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM_VCM_PD);
	} else {
		gpio_direction_output(CAM_PIN_GPIO_CAM_VCM_PD, 0);
		gpio_free(CAM_PIN_GPIO_CAM_VCM_PD);
		rc = 0;
	}
#endif

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "mt9v113");
	pr_info("reset pin gpio_request, %d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	udelay(50);

	/* VCM */
	pr_info("sensor_power_disable(\"reg_8038_l17\") == %d\n", rc);
	rc = camera_sensor_power_disable(reg_8038_l17);
	if (rc < 0)
		pr_err("sensor_power_disable(\"reg_8038_l17\") FAILED %d\n", rc);

	return rc;
}

static struct msm_camera_csi_lane_params mt9v113_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_mt9v113_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	/*.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY, No need */
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 0,
	.csi_lane_params = &mt9v113_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	= "mt9v113",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	/*.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY, No need */
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.camera_power_on = msm8930_mt9v113_vreg_on,
	.camera_power_off = msm8930_mt9v113_vreg_off,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_mt9v113,
	.sensor_platform_info = &sensor_mt9v113_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
	.use_rawchip = RAWCHIP_DISABLE,
};
#endif	//CONFIG_MT9V113



#ifdef CONFIG_OV5693
static int msm8930_ov5693_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8038_l17", 2800000, &reg_8038_l17);
	pr_info("vcm sensor_power_enable(\"8038_l17\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l17\", 2.8V) FAILED %d\n", rc);
		goto enable_ov5693_vcm_fail;
	}
	udelay(50);

	/* digital - Rawchip init already */
	/*rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
		goto enable_ov5693_digital_fail;
	}

	gpio_tlmm_config(
		GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	udelay(50); */

	/* analog */
	rc = camera_sensor_power_enable("8038_l8", 2800000, &reg_8038_l8);
	pr_info("analog sensor_power_enable(\"8038_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8038_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_ov5693_analog_fail;
	}
	udelay(50);

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
		goto enable_ov5693_io_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	udelay(50);

	return rc;

enable_ov5693_io_fail:
	camera_sensor_power_disable(reg_8038_l8);
enable_ov5693_analog_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
/*enable_ov5693_digital_fail:
	camera_sensor_power_disable(reg_8038_l17);*/
enable_ov5693_vcm_fail:
	return rc;
}

static int msm8930_ov5693_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* analog */
	rc = camera_sensor_power_disable(reg_8038_l8);
	pr_info("sensor_power_disable(\"8038_l8\") == %d\n", rc);
	if (rc < 0)
		pr_err("sensor_power_disable\(\"8038_l8\") FAILED %d\n", rc);
	udelay(50);

	/* digital - Close by rawchip */
	/*rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_tlmm_config(
			GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
	udelay(900); */

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}
	udelay(50);

	/* VCM */
	if (reg_8038_l17 != NULL)
		regulator_set_optimum_mode(reg_8038_l17, 9000);
	rc = camera_sensor_power_disable(reg_8038_l17);
	pr_info("sensor_power_disable(\"8038_l17\") == %d\n", rc);
	if (rc < 0)
		pr_err("sensor_power_disable\(\"8038_l17\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_OV5693_ACT
static struct i2c_board_info ov5693_actuator_i2c_info = {
	I2C_BOARD_INFO("ov5693_act", 0x0C),
};

static struct msm_actuator_info ov5693_actuator_info = {
	.board_info     = &ov5693_actuator_i2c_info,
	.bus_id         = MSM_8930_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params ov5693_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_ov5693_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN_XA,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &ov5693_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
//150 mA FL_MODE_FLASH_LEVEL1
//200 mA FL_MODE_FLASH_LEVEL2
//300 mA FL_MODE_FLASH_LEVEL3
//400 mA FL_MODE_FLASH_LEVEL4
//500 mA FL_MODE_FLASH_LEVEL5
//600 mA FL_MODE_FLASH_LEVEL6
//700 mA FL_MODE_FLASH_LEVEL7
static struct camera_led_est msm_camera_sensor_ov5693_led_table[] = {
	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 50,
		.max_step = 70
	},*/
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 181,//29,//23,  //mk0210
		.max_step = 256
	},
	{
		.enable = 0,//1,
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
		.min_step = 131,
		.max_step = 180
	},
	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 23,//25,
		.max_step = 29//26,
	},*/
	{
		.enable = 0,//1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},

	/*{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL7,
		.current_ma = 700,
		.lumen_value = 700,
		.min_step = 21,
		.max_step = 22
	},*/
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217  //mk0221
		.min_step = 0,
		.max_step = 130
	},
	{
		.enable = 0,//2,
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


static struct camera_led_info msm_camera_sensor_ov5693_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_ov5693_led_table),
};

static struct camera_flash_info msm_camera_sensor_ov5693_flash_info = {
	.led_info = &msm_camera_sensor_ov5693_led_info,
	.led_est_table = msm_camera_sensor_ov5693_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_ov5693_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.low_cap_limit_dual = 0,
	.flash_info			= &msm_camera_sensor_ov5693_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

#ifdef CONFIG_MSM_CAMERA_FLASH
int msm8930_flashlight_control_ov5693(int mode)
{
pr_info("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_camera_flash_src_ov5693 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = msm8930_flashlight_control_ov5693,
};
#endif

static struct msm_camera_sensor_flash_data flash_ov5693 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src_ov5693,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_ov5693_data = {
	.sensor_name	= "ov5693",
	.camera_power_on = msm8930_ov5693_vreg_on,
	.camera_power_off = msm8930_ov5693_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov5693,
	.sensor_platform_info = &sensor_ov5693_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_OV5693_ACT
	.actuator_info = &ov5693_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_ov5693_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_OV5693
// HTC_START pg 20130220 lc898212 act enable
#ifdef CONFIG_LC898212_ACT
static struct i2c_board_info lc898212_actuator_i2c_info = {
	I2C_BOARD_INFO("lc898212_act", 0x11),
};

static struct msm_actuator_info lc898212_actuator_info = {
	.board_info     = &lc898212_actuator_i2c_info,
	.cam_name       = MSM_ACTUATOR_MAIN_CAM_1, // HTC - set ACTUATOR_MAIN_CAM_1 for closeloop af config
	.bus_id         = MSM_8930_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
// HTC_END pg 20130220 lc898212 act enable
#ifdef CONFIG_VD6869

static int msm8930_vd6869_vreg_on(void)
{
	int rc =0;
	pr_info("%s\n", __func__);

	// vcm
	rc = camera_sensor_power_enable("8038_l17", 2850000, &reg_8038_l17);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l17\", 2.85V) FAILED %d\n", rc);
		goto enable_vd6869_vcm_fail;
	}
	udelay(50);

    // vcm pd
    rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,1);
    if (rc<0) {
        goto enable_vd6869_vcm_pd_fail;
    }

	// analog
	rc = camera_sensor_power_enable("8038_l8", 2900000, &reg_8038_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_vd6869_l8_fail;
	}
	udelay(50);

    // io 1.8v
	rc = gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,1);
    if (rc<0)
        goto enable_vd6869_io1v8_fail;

	// d 1.2v
    rc = gpio_set (CAM_PIN_GPIO_V_CAM_D1V2_EN,1);
    if (rc<0)
        goto enable_vd6869_d1v2_fail;

    return rc;

enable_vd6869_d1v2_fail:
    gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,0);

enable_vd6869_io1v8_fail:
    camera_sensor_power_disable(reg_8038_l8);

enable_vd6869_l8_fail:
    gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);

enable_vd6869_vcm_pd_fail:
    camera_sensor_power_disable(reg_8038_l17);

enable_vd6869_vcm_fail:

	return rc;
}

static int msm8930_vd6869_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

    gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,0);
    camera_sensor_power_disable(reg_8038_l8);
    gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);
    //gpio_set (CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
	camera_sensor_power_disable(reg_8038_l17);
    gpio_set (CAM_PIN_GPIO_V_CAM_D1V2_EN,0); // HTC pg 20130412 vd6869 power sequence

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
	.mirror_flip = CAMERA_SENSOR_NONE, // HTC pg 20130219 sensor angle
#endif
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM_PWDN_XA,
	.sensor_pwd	= 0,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &vd6869_csi_lane_params,
	.sensor_mount_angle = ANGLE_90,
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
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_vd6869_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_vd6869 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data = {
	.sensor_name	= "vd6869",
	.camera_power_on = msm8930_vd6869_vreg_on,
	.camera_power_off = msm8930_vd6869_vreg_off,
	.camera_yushanii_probed = 0,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_vd6869,
	.sensor_platform_info = &sensor_vd6869_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
// HTC_START pg 20130220 lc898212 act enable
#ifdef CONFIG_LC898212_ACT
    .actuator_info = &lc898212_actuator_info,
#endif
// HTC_END pg 20130220 lc898212 act enable
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_vd6869_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif	//CONFIG_VD6869


#ifdef CONFIG_OV4688

static int msm8930_ov4688_vreg_on(void)
{
	int rc =0;
	pr_info("%s\n", __func__);

	// vcm
	rc = camera_sensor_power_enable("8038_l17", 2850000, &reg_8038_l17);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l17\", 2.85V) FAILED %d\n", rc);
		goto enable_ov4688_vcm_fail;
	}
	msleep(1); // HTC pg 20130409 ov4688 power sequence

    // vcm pd
    rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,1);
    if (rc<0) {
        goto enable_ov4688_vcm_pd_fail;
    }

	// analog
	rc = camera_sensor_power_enable("8038_l8", 2900000, &reg_8038_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_ov4688_l8_fail;
	}
	udelay(50);

    // io 1.8v
	rc = gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,1);
    if (rc<0)
        goto enable_ov4688_io1v8_fail;

	// d 1.2v
    rc = gpio_set (CAM_PIN_GPIO_V_CAM_D1V2_EN,1);
    if (rc<0)
        goto enable_ov4688_d1v2_fail;

    return rc;

enable_ov4688_d1v2_fail:
    gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,0);

enable_ov4688_io1v8_fail:
    camera_sensor_power_disable(reg_8038_l8);

enable_ov4688_l8_fail:
    gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);

enable_ov4688_vcm_pd_fail:
    camera_sensor_power_disable(reg_8038_l17);

enable_ov4688_vcm_fail:

	return rc;
}

static int msm8930_ov4688_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

    gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,0);
    camera_sensor_power_disable(reg_8038_l8);
    gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);
    //gpio_set (CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
	camera_sensor_power_disable(reg_8038_l17);
    gpio_set (CAM_PIN_GPIO_V_CAM_D1V2_EN,0); // HTC pg 20130409 ov4688 power sequence

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
	.mirror_flip = CAMERA_SENSOR_NONE, // HTC pg 20130219 sensor angle
#endif
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM_PWDN_XA,
	.sensor_pwd	= 0,
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

static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data = {
	.sensor_name	= "ov4688",
	.camera_power_on = msm8930_ov4688_vreg_on,
	.camera_power_off = msm8930_ov4688_vreg_off,
	.camera_yushanii_probed = 0,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov4688,
	.sensor_platform_info = &sensor_ov4688_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
// HTC_START pg 20130220 lc898212 act enable
#ifdef CONFIG_LC898212_ACT
    .actuator_info = &lc898212_actuator_info,
#endif
// HTC_END pg 20130220 lc898212 act enable
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_ov4688_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

#endif//end ov4688


#ifdef CONFIG_S5K6A2YA
static int msm8930_s5k6a2ya_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8038_l17", 2850000, &reg_8038_l17);
	pr_info("[CAM] sensor_power_enable(\"8038_l17\", 2.85V) == %d\n", rc);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l17\", 2.85V) FAILED %d\n", rc);
		goto enable_s5k6a2ya_vcm_fail;
	}
	udelay(50);
#if 0
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("[CAM] digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
		goto enable_s5k6a2ya_digital_fail;
	}
	gpio_tlmm_config(
		GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
#endif

	/* VCM PD */
	rc = gpio_request(CAM_PIN_GPIO_CAM_VCM_PD, "CAM_VCM_PD");
	pr_info("[CAM] vcm pd gpio_request, %d\n", CAM_PIN_GPIO_CAM_VCM_PD);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM_VCM_PD);
		goto enable_s5k6a2ya_vcm_pd;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_VCM_PD, 1);
	gpio_free(CAM_PIN_GPIO_CAM_VCM_PD);

	udelay(500);

	/* analog */
	rc = camera_sensor_power_enable("8038_l8", 2900000, &reg_8038_l8);
	pr_info("[CAM] sensor_power_enable(\"8038_l8\", 2.9V) == %d\n", rc);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_s5k6a2ya_analog_fail;
	}
	udelay(50);

	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("[CAM] cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
		goto enable_s5k6a2ya_io_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	udelay(50);
#if 0
	/* 2nd cam digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "V_CAM2_D1V2_EN");
	pr_info("[CAM] 2nd cam digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		goto enable_s5k6a2ya_2nd_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	udelay(50);
#endif
	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a2ya");
	pr_info("[CAM] reset pin gpio_request, %d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto enable_s5k6a2ya_rst_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	return rc;
#if 0
enable_s5k6a2ya_2nd_digital_fail:
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a2ya");
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
#endif
enable_s5k6a2ya_rst_fail:
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}

enable_s5k6a2ya_io_fail:
	camera_sensor_power_disable(reg_8038_l8);
enable_s5k6a2ya_analog_fail:
	rc = gpio_request(CAM_PIN_GPIO_CAM_VCM_PD, "CAM_VCM_PD");
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM_VCM_PD);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM_VCM_PD, 0);
		gpio_free(CAM_PIN_GPIO_CAM_VCM_PD);
	}
enable_s5k6a2ya_vcm_pd:
#if 0
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
enable_s5k6a2ya_digital_fail:
#endif
	camera_sensor_power_disable(reg_8038_l17);
enable_s5k6a2ya_vcm_fail:

	return rc;
}


static int msm8930_s5k6a2ya_vreg_off(void)
{
	int rc = 0;
	pr_info("[CAM] %s\n", __func__);

	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a2ya");
	pr_info("[CAM] reset pin gpio_request, %d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	udelay(50);

	/* analog */
	pr_info("[CAM] sensor_power_disable(\"8038_l8\") == %d\n", rc);
	rc = camera_sensor_power_disable(reg_8038_l8);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"reg_8038_l8\") FAILED %d\n", rc);
	udelay(50);
#if 0
	/* 2nd digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "V_CAM2_D1V2_EN");
	pr_info("[CAM] 2nd cam digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	}
	msleep(1);
#endif
#if 0
	/* digital */
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V8_EN, "V_CAM_D1V8_EN");
	pr_info("[CAM] digital gpio_request, %d\n", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM_D1V8_EN);
	else {
		gpio_tlmm_config(
			GPIO_CFG(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V8_EN);
	}
	udelay(50);
#endif
	/* IO */
	rc = gpio_request(CAM_PIN_GPIO_V_CAMIO_1V8_EN, "V_CAMIO_1V8_EN");
	pr_info("[CAM] cam io gpio_request, %d\n", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAMIO_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAMIO_1V8_EN);
	}

	/* VCM PD */
	rc = gpio_request(CAM_PIN_GPIO_CAM_VCM_PD, "CAM_VCM_PD");
	pr_info("[CAM] vcm pd gpio_request, %d\n", CAM_PIN_GPIO_CAM_VCM_PD);
	if (rc < 0)
		pr_err("[CAM] GPIO(%d) request failed", CAM_PIN_GPIO_CAM_VCM_PD);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM_VCM_PD, 0);
		gpio_free(CAM_PIN_GPIO_CAM_VCM_PD);
	}
	udelay(50);

	/* VCM */
	pr_info("[CAM] sensor_power_disable(\"reg_8038_l17\") == %d\n", rc);
	rc = camera_sensor_power_disable(reg_8038_l17);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"reg_8038_l17\") FAILED %d\n", rc);

	return rc;
}

static struct msm_camera_csi_lane_params s5k6a2ya_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_s5k6a2ya_board_info = {
	.mount_angle = 270,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	//.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &s5k6a2ya_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_s5k6a2ya = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a2ya_data = {
	.sensor_name	= "s5k6a2ya",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	//.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = msm8930_s5k6a2ya_vreg_on,
	.camera_power_off = msm8930_s5k6a2ya_vreg_off,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.pdata	= &msm_camera_csi_device_data[1],
#else
	.pdata	= &msm_camera_csi_device_data[0],
#endif
	.flash_data	= &flash_s5k6a2ya,
	.sensor_platform_info = &sensor_s5k6a2ya_board_info,
	.gpio_conf = &gpio_conf,
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
#endif	//CONFIG_S5K6A2YA

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

int camera_id(int* id)
{
	int rc=0;

    // io 1.8v
	rc = gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,1);
    if (rc<0)
        return rc;

    msleep(100);
    rc = gpio_get(CAM_PIN_GPIO_ID,id);
    if (rc<0)
        return rc;

    rc = gpio_set (CAM_PIN_GPIO_V_CAMIO_1V8_EN,0);
    
    return rc;
}

#ifdef CONFIG_I2C
struct i2c_board_info zip_cl_camera_i2c_boardinfo_vd6869_s5k6a2ya[] = {

#ifdef CONFIG_VD6869
    {
    I2C_BOARD_INFO("vd6869", 0x20 >> 1),
    .platform_data = &msm_camera_sensor_vd6869_data,
	},
#endif

#ifdef CONFIG_S5K6A2YA
	{
	 I2C_BOARD_INFO("s5k6a2ya", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_s5k6a2ya_data,
	}
#endif
};

struct i2c_board_info zip_cl_camera_i2c_boardinfo_ov4688_0x6c_s5k6a2ya[] = {

#ifdef CONFIG_OV4688
    {
    I2C_BOARD_INFO("ov4688_0x6c", 0x20 >> 1), // to let ov4688 and s5k6a2ya can be reisgtered by i2c driver
    .platform_data = &msm_camera_sensor_ov4688_data,
	},
#endif
#ifdef CONFIG_S5K6A2YA
	{
	 I2C_BOARD_INFO("s5k6a2ya", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_s5k6a2ya_data,
	}
#endif
};

struct i2c_board_info zip_cl_camera_i2c_boardinfo_ov4688_0x20_s5k6a2ya[] = {

#ifdef CONFIG_OV4688
    {
    I2C_BOARD_INFO("ov4688_0x20", 0x20 >> 1),
    .platform_data = &msm_camera_sensor_ov4688_data,
	},
#endif
#ifdef CONFIG_S5K6A2YA
	{
	 I2C_BOARD_INFO("s5k6a2ya", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_s5k6a2ya_data,
	}
#endif
};

struct msm_camera_board_info zip_cl_camera_board_info ={
    .board_info = zip_cl_camera_i2c_boardinfo_ov4688_0x6c_s5k6a2ya,
    .num_i2c_board_info = ARRAY_SIZE(zip_cl_camera_i2c_boardinfo_ov4688_0x6c_s5k6a2ya),
};

#endif
extern unsigned int engineerid;
extern unsigned int system_rev;

void __init zip_cl_init_cam(void)
{
    int rc=0,id=0;

	pr_info("msm8930_cam_common_configs");

	msm_gpiomux_install(msm8930_cam_common_configs,
			ARRAY_SIZE(msm8930_cam_common_configs));

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);

    rc = camera_id(&id);
    if (rc<0) {
        pr_err("can't read camera id\n");
        return;
    }
    pr_info("camera id=%d system_rev=%d engineerid=%d\n",id,system_rev,engineerid);
    if (id) {
        if (system_rev) {
            zip_cl_camera_board_info.board_info = zip_cl_camera_i2c_boardinfo_ov4688_0x20_s5k6a2ya;
            zip_cl_camera_board_info.num_i2c_board_info = ARRAY_SIZE(zip_cl_camera_i2c_boardinfo_ov4688_0x20_s5k6a2ya);
        }
        else {
	        zip_cl_camera_board_info.board_info = zip_cl_camera_i2c_boardinfo_ov4688_0x6c_s5k6a2ya;
	        zip_cl_camera_board_info.num_i2c_board_info = ARRAY_SIZE(zip_cl_camera_i2c_boardinfo_ov4688_0x6c_s5k6a2ya);
	    }
	}
	else {
	    zip_cl_camera_board_info.board_info = zip_cl_camera_i2c_boardinfo_vd6869_s5k6a2ya;
	    zip_cl_camera_board_info.num_i2c_board_info = ARRAY_SIZE(zip_cl_camera_i2c_boardinfo_vd6869_s5k6a2ya);        
	}
}
