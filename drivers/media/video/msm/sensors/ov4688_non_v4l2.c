#include "msm_sensor.h"

#define SENSOR_NAME "ov4688"
#define PLATFORM_DRIVER_NAME "msm_camera_ov4688"
#define ov4688_obj ov4688_##obj

#define OV4688_REG_READ_MODE 0x0101
#define OV4688_READ_NORMAL_MODE 0x0000	/* without mirror/flip */
#define OV4688_READ_MIRROR 0x0001			/* with mirror */
#define OV4688_READ_FLIP 0x0002			/* with flip */
#define OV4688_READ_MIRROR_FLIP 0x0003	/* with mirror/flip */

#define REG_DIGITAL_GAIN_GREEN_R 0x020E
#define REG_DIGITAL_GAIN_RED 0x0210
#define REG_DIGITAL_GAIN_BLUE 0x0212
#define REG_DIGITAL_GAIN_GREEN_B 0x0214

DEFINE_MUTEX(ov4688_mut);
DEFINE_MUTEX(ov4688_sensor_init_mut);//CC120826,
static struct msm_sensor_ctrl_t ov4688_s_ctrl;

static struct msm_camera_i2c_reg_conf ov4688_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov4688_stop_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_groupon_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_groupoff_settings[] = {

};

#if 0
static struct msm_camera_i2c_reg_conf ov4688_mipi_settings[] = {
};

static struct msm_camera_i2c_reg_conf ov4688_pll_settings[] = {
};
#endif

static struct msm_camera_i2c_reg_conf ov4688_prev_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_video_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_fast_video_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_snap_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_16_9_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_4_3_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_snap_wide_settings[] = {

};

 /* TMP - Now it's same as FULL SIZE */
 static struct msm_camera_i2c_reg_conf ov4688_night_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_fw_settings[] = {
/*///////////////////////////////////////////////// */
/* OV4688 */
/*///////////////////////////////////////////////// */
{0x4440, 0x80},
{0x4100, 0x03},
{0x6000, 0x90},
{0x0103, 0x01},
{0x3105, 0x31},
{0x3638, 0x00},
{0x0300, 0x02},
{0x0302, 0x3a},
{0x0304, 0x03},
{0x030b, 0x00},
{0x030d, 0x1e},
{0x030e, 0x04},
{0x030f, 0x02},
{0x0312, 0x01},
{0x031e, 0x00},
{0x3000, 0x20},
{0x3002, 0x00},
{0x3018, 0x72},
{0x3020, 0x93},
{0x3022, 0x01},
{0x3031, 0x0a},
{0x3305, 0xf1},
{0x3307, 0x04},
{0x3500, 0x00},
{0x3501, 0x60},
{0x3502, 0x00},
{0x3503, 0x00},
{0x3504, 0x00},
{0x3505, 0x80},
{0x3506, 0x00},
{0x3507, 0x00},
{0x3508, 0x10},
{0x3509, 0x80},
{0x350a, 0x00},
{0x350b, 0x00},
{0x350c, 0x00},
{0x350d, 0x00},
{0x350e, 0x00},
{0x350f, 0x80},
{0x3510, 0x00},
{0x3511, 0x00},
{0x3512, 0x00},
{0x3513, 0x00},
{0x3514, 0x00},
{0x3515, 0x80},
{0x3516, 0x00},
{0x3517, 0x00},
{0x3518, 0x00},
{0x3519, 0x00},
{0x351a, 0x00},
{0x351b, 0x80},
{0x351c, 0x00},
{0x351d, 0x00},
{0x351e, 0x00},
{0x351f, 0x00},
{0x3520, 0x00},
{0x3521, 0x80},
{0x352a, 0x08},
{0x3602, 0x00},
{0x3605, 0x00},
{0x3606, 0x00},
{0x3607, 0x00},
{0x3609, 0x10},
{0x360a, 0x00},
{0x360c, 0x08},
{0x360f, 0xe5},
{0x3611, 0x00},
{0x3619, 0x99},
{0x361b, 0x60},
{0x361e, 0x79},
{0x361f, 0x02},
{0x3632, 0x00},
{0x3633, 0x10},
{0x3634, 0x10},
{0x3635, 0x10},
{0x3636, 0x10},
{0x3646, 0x86},
{0x364a, 0x0b},
{0x3700, 0x17},
{0x3705, 0x00},
{0x3706, 0x56},
{0x3709, 0x3c},
{0x370b, 0x01},
{0x370c, 0x00},
{0x3710, 0x24},
{0x3711, 0x0c},
{0x3716, 0x00},
{0x3729, 0x7b},
{0x372a, 0x84},
{0x372b, 0xbb},
{0x372c, 0xbe},
{0x372e, 0x52},
{0x3743, 0x10},
{0x3744, 0x80},
{0x374a, 0x43},
{0x374c, 0x00},
{0x3751, 0x8c},
{0x3754, 0x8f},
{0x3756, 0x5c},
{0x375c, 0x00},
{0x3760, 0x00},
{0x3761, 0x00},
{0x3762, 0x00},
{0x3763, 0x00},
{0x3764, 0x00},
{0x3767, 0x04},
{0x3768, 0x04},
{0x3769, 0x08},
{0x376a, 0x08},
{0x376b, 0x20},
{0x376c, 0x00},
{0x376d, 0x00},
{0x376e, 0x00},
{0x3773, 0x80},
{0x3774, 0x10},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x04},
{0x3804, 0x0a},
{0x3805, 0x9f},
{0x3806, 0x05},
{0x3807, 0xfb},
{0x3808, 0x0a},
{0x3809, 0x80},
{0x380a, 0x05},
{0x380b, 0xf0},
{0x380c, 0x03}, /* for line length pck */
{0x380d, 0x54}, /* for line length pck */
{0x380e, 0x06}, /* for frame length lines */
{0x380f, 0x22}, /* for frame length lines */
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x04},
{0x3814, 0x01},
{0x3815, 0x01},
{0x3819, 0x01},
{0x3820, 0x06}, /* for mirror */
{0x3821, 0x00}, /* for mirror */
{0x3829, 0x00},
{0x382a, 0x01},
{0x382b, 0x01},
{0x382d, 0x7f},
{0x3830, 0x04},
{0x3836, 0x01},
{0x3841, 0x02},
{0x3846, 0x08},
{0x3847, 0x07},
{0x3d85, 0x36},
{0x4000, 0xf1},
{0x4001, 0x40},
{0x4002, 0x04},
{0x4003, 0x14},
{0x4004, 0x01}, /* bor BLC */
{0x4005, 0x00}, /* bor BLC */
{0x400e, 0x00},
{0x4011, 0x00},
{0x401a, 0x00},
{0x401b, 0x00},
{0x401c, 0x00},
{0x401d, 0x00},
{0x401f, 0x00},
{0x4020, 0x02},
{0x4021, 0x40},
{0x4022, 0x08},
{0x4023, 0x3f},
{0x4024, 0x07},
{0x4025, 0xc0},
{0x4026, 0x08},
{0x4027, 0xbf},
{0x4028, 0x00},
{0x4029, 0x02},
{0x402a, 0x06},
{0x402b, 0x04},
{0x402c, 0x02},
{0x402d, 0x02},
{0x402e, 0x0e},
{0x402f, 0x04},
{0x4302, 0xff},
{0x4303, 0xff},
{0x4304, 0x00},
{0x4305, 0x00},
{0x4306, 0x00},
{0x4308, 0x02},
{0x4500, 0x6c},
{0x4502, 0x40},
{0x4503, 0x02},
{0x4601, 0x04},
{0x4800, 0x04},
{0x4813, 0x00},
{0x481f, 0x40},
{0x4829, 0x78},
{0x4837, 0x0b},
{0x4b00, 0x2a},
{0x4b0d, 0x00},
{0x4d00, 0x04},
{0x4d01, 0x18},
{0x4d02, 0xc3},
{0x4d03, 0xff},
{0x4d04, 0xff},
{0x4d05, 0xff},
{0x5000, 0x83},
{0x5001, 0x11},
{0x5004, 0x00},
{0x500a, 0x00},
{0x500b, 0x00},
{0x5032, 0x00},
{0x5040, 0x00},
{0x5050, 0x0c},
{0x8000, 0x00},
{0x8001, 0x00},
{0x8002, 0x00},
{0x8003, 0x00},
{0x8004, 0x00},
{0x8005, 0x00},
{0x8006, 0x00},
{0x8007, 0x00},
{0x8008, 0x00},
{0x3105, 0x11},
// HTC_START mike.cy_lin
#if 0
{0x3841, 0x03}, /* default: 0x02, hdr enable: 0x03 */
{0x3846, 0x08},
{0x3847, 0x02}, /* default: 0x07, long only: 0x04, middle only: 0x02 */
{0x4800, 0x0C}, /* default: 0x04, hdr enable: 0x0C */
{0x4813, 0x08}, /* default: 0x00, hdr enable: 0x08 */
{0x376e, 0x01}, /* default: 0x00, hdr enable: 0x01 */
{0x350a, 0x00},
{0x350b, 0x01}, /* middle exposure - default: 0x00 */
{0x350c, 0x00},
{0x3500, 0x00},
{0x3511, 0x00}, /* short exposure - default: 0x00 */
{0x3512, 0x00},
#endif
// HTC_END
};
#if 0 // mike.cy_lin: temp close to bypass kernel warning
static struct msm_camera_i2c_reg_conf ov4688_version_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_general_settings[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_analog_settings_clearbit[] = {

};

static struct msm_camera_i2c_reg_conf ov4688_analog_settings[] = {

};
#endif
static struct v4l2_subdev_info ov4688_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov4688_init_conf[] = {
	{&ov4688_fw_settings[0],
	ARRAY_SIZE(ov4688_fw_settings), 50, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array ov4688_confs[] = {
	{&ov4688_snap_settings[0],
	ARRAY_SIZE(ov4688_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_prev_settings[0],
	ARRAY_SIZE(ov4688_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_video_settings[0],
	ARRAY_SIZE(ov4688_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_fast_video_settings[0],
	ARRAY_SIZE(ov4688_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_16_9_settings[0],
	ARRAY_SIZE(ov4688_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_4_3_settings[0],
	ARRAY_SIZE(ov4688_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_night_settings[0],
	ARRAY_SIZE(ov4688_night_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_snap_wide_settings[0],
	ARRAY_SIZE(ov4688_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov4688_dimensions[] = {
	{/*full size*/
		.x_output = 0xA80, /* 2688 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xEA0, /* 3744 */
		.frame_length_lines = 0xB22, /* 2850 */
		.vt_pixel_clk = 320000000,
		.op_pixel_clk = 320000000,
		.binning_factor = 1,
	},
	{/*Q size*/
		.x_output = 0x800, /* 2048 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
	{/*video size*/
		.x_output = 0x800, /* 2048 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
	{/*fast video size*/
		.x_output = 0x540, /* 1344 */
		.y_output = 0x2F8, /* 760 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x317, /* 791 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 2,
	},
	{/*16:9*/
		.x_output = 0xA80, /* 2688 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
	{/*4:3*/
		.x_output = 0x800, /* 2048 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
	{/*night mode size*/ /* TMP - Now it's same as FULL SIZE */
		.x_output = 0x800, /* 2048 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
	{/*wide full size*/
		.x_output = 0x800, /* 2048 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg ov4688_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov4688_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 4,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ov4688_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 4,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *ov4688_csi_params_array[] = {
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params
};

static struct msm_sensor_output_reg_addr_t ov4688_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};

static struct msm_sensor_id_info_t ov4688_id_info = {
	.sensor_id_reg_addr = 0x300A,
	.sensor_id = 0x4688,
};
#define SENSOR_REGISTER_MAX_LINECOUNT 0xffff
#define SENSOR_VERT_OFFSET 25

static struct msm_sensor_exp_gain_info_t ov4688_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x3508,
	.vert_offset = SENSOR_VERT_OFFSET,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
	.sensor_max_linecount = SENSOR_REGISTER_MAX_LINECOUNT-SENSOR_VERT_OFFSET, /* sensor max linecount = max unsigned value of linecount register size - vert_offset */ /* HTC ben 20120229 */
};

int32_t ov4688_set_dig_gain(struct msm_sensor_ctrl_t *s_ctrl, uint16_t dig_gain)
{
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_R, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_RED, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_BLUE, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_B, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);

	return 0;
}

static int ov4688_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	if (data->sensor_platform_info)
		ov4688_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;
	/* move setting mirror_flip after sensor size config */

	return rc;
}

static const char *ov4688Vendor = "ov";
static const char *ov4688NAME = "ov4688";
static const char *ov4688Size = "4M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	CDBG("%s called\n", __func__);

	sprintf(buf, "%s %s %s\n", ov4688Vendor, ov4688NAME, ov4688Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ov4688;

static int ov4688_sysfs_init(void)
{
	int ret ;
	pr_info("%s: ov4688:kobject creat and add\n", __func__);

	android_ov4688 = kobject_create_and_add("android_camera", NULL);
	if (android_ov4688 == NULL) {
		pr_info("ov4688_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ov4688:sysfs_create_file\n");
	ret = sysfs_create_file(android_ov4688, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ov4688_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ov4688);
	}

	return 0 ;
}

int32_t ov4688_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);

	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ov4688_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ov4688_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov4688_s_ctrl},
	{ }
};

static struct i2c_driver ov4688_i2c_driver = {
	.id_table = ov4688_i2c_id,
	.probe  = ov4688_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov4688_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t ov4688_power_up(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s called\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_info("%s: failed to sensor platform_data didnt register\n", __func__);
		return -EIO;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_info("%s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_info("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_set_power_up(s_ctrl);//(sdata);
	if (rc < 0) {
		pr_info("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	mdelay(5);

	ov4688_sensor_open_init(sdata);
	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_info("%s: failed to sensor platform_data didnt register\n", __func__);
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t ov4688_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s called\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("%s: failed to sensor platform_data didnt register\n", __func__);
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0)
		pr_info("%s: failed to disable power\n", __func__);

	rc = msm_sensor_set_power_down(s_ctrl);
	if (rc < 0)
		pr_info("%s: msm_sensor_power_down failed\n", __func__);

	if (!sdata->use_rawchip) {
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_info("%s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	return rc;
}

static int __init msm_sensor_init_module(void)
{
	pr_info("ov4688 %s\n", __func__);
	return i2c_add_driver(&ov4688_i2c_driver);
}

static struct v4l2_subdev_core_ops ov4688_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov4688_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov4688_subdev_ops = {
	.core = &ov4688_subdev_core_ops,
	.video  = &ov4688_subdev_video_ops,
};

static int ov4688_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
#if 0
	int32_t rc = 0;
	int i;
	uint16_t read_data = 0;
	static uint8_t OTP[10] = {0,0,0,0,0,0,0,0,0,0};
	static int first=true;
	CDBG("%s called\n", __func__);

	if (first) {
		first = false;
		
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3400, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)
			pr_err("%s: msm_camera_i2c_write 0x3400 failed\n", __func__);

		/* Set Page 1 */
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3402, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)
			pr_err("%s: msm_camera_i2c_write 0x3402 failed\n", __func__);

		for (i = 0; i < 10; i++) {
			rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (0x3404 + i), &read_data, MSM_CAMERA_I2C_BYTE_DATA);
			if (rc < 0)
				pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, (0x3404 + i));

			OTP[i] = (uint8_t)(read_data&0x00FF);
			read_data = 0;
		}
	}
	
	pr_info("%s: VenderID=%x,LensID=%x,SensorID=%02x%02x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("%s: ModuleFuseID= %02x%02x%02x%02x%02x%02x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

	cdata->cfg.fuse.fuse_id_word1 = 0;
	cdata->cfg.fuse.fuse_id_word2 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word3 =
		(OTP[4]<<24) |
		(OTP[5]<<16) |
		(OTP[6]<<8) |
		(OTP[7]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[8]<<8) |
		(OTP[9]);

	pr_info("ov4688: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("ov4688: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("ov4688: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("ov4688: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
#endif
	return 0;

}

static int ov4688_read_VCM_driver_IC_info(	struct msm_sensor_ctrl_t *s_ctrl)
{
#if 0
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP = 0;
	struct msm_camera_i2c_client *msm_camera_i2c_client = s_ctrl->sensor_i2c_client;
	struct msm_camera_sensor_info *sdata;

	pr_info("%s: sensor OTP information:\n", __func__);

	s_ctrl = get_sctrl(&s_ctrl->sensor_v4l2_subdev);
	sdata = (struct msm_camera_sensor_info *) s_ctrl->sensordata;

	if ((sdata->actuator_info_table == NULL) || (sdata->num_actuator_info_table <= 1))
	{
		pr_info("%s: failed to actuator_info_table == NULL or num_actuator_info_table <= 1 return 0\n", __func__);
		return 0;
	}

	//Set Sensor to SW-Standby
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x0100, 0x00);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x0100 fail\n", __func__);

	//Set Input clock freq.(24MHz)
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3368, 0x18);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3368 fail\n", __func__);

	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3369, 0x00);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3369 fail\n", __func__);

	//set read mode
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3400, 0x01);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3400 fail\n", __func__);

	mdelay(4);

	//select information index, Driver ID at 10th index
	info_index = 10;
	/*Read page 3 to Page 0*/
	for (page = 3; page >= 0; page--) {
		//Select page
		rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3402, page);
		if (rc < 0)
			pr_info("%s: i2c_write_b 0x3402 (select page %d) fail\n", __func__, page);

		//Select information index. Driver ID at 10th index
		//for formal sample
		rc = msm_camera_i2c_read_b(msm_camera_i2c_client, (0x3410 + info_index), &info_value);
		if (rc < 0)
			pr_info("%s: i2c_read_b 0x%x fail\n", __func__, (0x3410 + info_index));

		/* some values of fuseid are maybe zero */
		if (((info_value & 0x0F) != 0) || page < 0)
			break;

		//for first sample
		rc = msm_camera_i2c_read_b(msm_camera_i2c_client, (0x3404 + info_index), &info_value);
		if (rc < 0)
			pr_info("%s: i2c_read_b 0x%x fail\n", __func__, (0x3404 + info_index));

		/* some values of fuseid are maybe zero */
		if (((info_value & 0x0F) != 0) || page < 0)
			break;

	}

	OTP = (short)(info_value&0x0F);
	pr_info("%s: OTP=%d\n", __func__, OTP);

	if (sdata->num_actuator_info_table > 1)
	{
		if (OTP == 1) //AD5816
			sdata->actuator_info = &sdata->actuator_info_table[2][0];
		else if (OTP == 2) //TI201
			sdata->actuator_info = &sdata->actuator_info_table[1][0];

		pr_info("%s: sdata->actuator_info->board_info->type=%s", __func__, sdata->actuator_info->board_info->type);
		pr_info("%s: sdata->actuator_info->board_info->addr=0x%x", __func__, sdata->actuator_info->board_info->addr);
	}

	/* interface disable */
#endif
	return 0;
}

void ov4688_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	//uint16_t read_data;
	uint16_t write_data;
	CDBG("%s: called\n", __func__);

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf,
		s_ctrl->msm_sensor_reg->start_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
	mdelay(50);
#if 0
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30CC, 0x0,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* clear 0x3300 bit 4 */
	msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x3300, &read_data,
		MSM_CAMERA_I2C_BYTE_DATA);
	read_data &= 0xEF;
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x3300, read_data,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x4424, 0x07,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30CC, 0x1,
		MSM_CAMERA_I2C_BYTE_DATA);
#endif
	if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		write_data = 0x00;
	else
		write_data = 0x01;
}

int32_t ov4688_write_exp_gain1_ex_nonHDR(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) /* HTC Angie 20111019 - Fix FPS */
{
	uint32_t fl_lines;
	uint8_t offset;

/* HTC_START Angie 20111019 - Fix FPS */
	uint32_t fps_divider = Q10;
	CDBG("%s: called\n", __func__);

	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

/* HTC_START ben 20120229 */
	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;
/* HTC_END */

	fl_lines = s_ctrl->curr_frame_length_lines / 2;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;
/* HTC_END */

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	//msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	//	s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
	//	MSM_CAMERA_I2C_WORD_DATA);

	// 3500[3:0]+3501[7:0]+3502[7:4]
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3500, line>>12, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3501, (line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3502, (line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	/* HTC_START pg digi gain 20100710 */
	//if (s_ctrl->func_tbl->sensor_set_dig_gain)
	//	s_ctrl->func_tbl->sensor_set_dig_gain(s_ctrl, dig_gain);
	/* HTC_END pg digi gain 20100710 */
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	return 0;
}

int ov4688_write_output_settings_specific(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	int rc = 0;
	uint16_t value = 0;

	/* Apply sensor mirror/flip */
	if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = OV4688_READ_MIRROR_FLIP;
	else if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = OV4688_READ_MIRROR;
	else if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = OV4688_READ_FLIP;
	else
		value = OV4688_READ_NORMAL_MODE;
	rc = msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client,
		OV4688_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s set mirror_flip failed\n", __func__);
		return rc;
	}

	return rc;
}

static struct msm_sensor_fn_t ov4688_func_tbl = {
	.sensor_start_stream = ov4688_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = ov4688_write_exp_gain1_ex_nonHDR,
	.sensor_write_snapshot_exp_gain_ex = ov4688_write_exp_gain1_ex_nonHDR,
/* HTC_START 20121013 */
/* TMP : disable digital gain - waiting for gain issue fix from ST */
#if 0
	.sensor_set_dig_gain = ov4688_set_dig_gain,
#endif
/* HTC_END */
	.sensor_setting = msm_sensor_setting_parallel,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov4688_power_up,
	.sensor_power_down = ov4688_power_down,
	.sensor_i2c_read_fuseid = ov4688_read_fuseid,
	.sensor_i2c_read_vcm_driver_ic = ov4688_read_VCM_driver_IC_info,
	.sensor_write_output_settings_specific = ov4688_write_output_settings_specific,
};

static struct msm_sensor_reg_t ov4688_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov4688_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov4688_start_settings),
	.stop_stream_conf = ov4688_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov4688_stop_settings),
	.group_hold_on_conf = ov4688_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov4688_groupon_settings),
	.group_hold_off_conf = ov4688_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov4688_groupoff_settings),
	.init_settings = &ov4688_init_conf[0],
	.init_size = ARRAY_SIZE(ov4688_init_conf),
	.mode_settings = &ov4688_confs[0],
	.output_settings = &ov4688_dimensions[0],
	.num_conf = ARRAY_SIZE(ov4688_confs),
};

static struct msm_sensor_ctrl_t ov4688_s_ctrl = {
	.msm_sensor_reg = &ov4688_regs,
	.sensor_i2c_client = &ov4688_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &ov4688_reg_addr,
	.sensor_id_info = &ov4688_id_info,
	.sensor_exp_gain_info = &ov4688_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov4688_csi_params_array[0],
	.msm_sensor_mutex = &ov4688_mut,
	.sensor_i2c_driver = &ov4688_i2c_driver,
	.sensor_v4l2_subdev_info = ov4688_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov4688_subdev_info),
	.sensor_v4l2_subdev_ops = &ov4688_subdev_ops,
	.func_tbl = &ov4688_func_tbl,
	.sensor_first_mutex = &ov4688_sensor_init_mut, //CC120826,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("OV 4MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
