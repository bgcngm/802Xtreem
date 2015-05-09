#include "msm_sensor.h"

#define SENSOR_NAME "vd6869"
#define PLATFORM_DRIVER_NAME "msm_camera_vd6869"
#define vd6869_obj vd6869_##obj

#define VD6869_REG_READ_MODE 0x0101
#define VD6869_READ_NORMAL_MODE 0x0000	/* without mirror/flip */
#define VD6869_READ_MIRROR 0x0001			/* with mirror */
#define VD6869_READ_FLIP 0x0002			/* with flip */
#define VD6869_READ_MIRROR_FLIP 0x0003	/* with mirror/flip */

#define REG_DIGITAL_GAIN_GREEN_R 0x020E
#define REG_DIGITAL_GAIN_RED 0x0210
#define REG_DIGITAL_GAIN_BLUE 0x0212
#define REG_DIGITAL_GAIN_GREEN_B 0x0214

DEFINE_MUTEX(vd6869_mut);
DEFINE_MUTEX(vd6869_sensor_init_mut);//CC120826,
static struct msm_sensor_ctrl_t vd6869_s_ctrl;

static struct msm_camera_i2c_reg_conf vd6869_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf vd6869_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf vd6869_groupoff_settings[] = {
	{0x104, 0x00},
};

#if 0
static struct msm_camera_i2c_reg_conf vd6869_mipi_settings[] = {
};

static struct msm_camera_i2c_reg_conf vd6869_pll_settings[] = {
};
#endif

static struct msm_camera_i2c_reg_conf vd6869_prev_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 696Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */ //woody
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2048 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x01}, /* x_start_addr = 320 */
	{0x345, 0x40},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x09}, /* x_addr_end = 2367 */
	{0x349, 0x3f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x08}, /* x_output_size = 2048 */
	{0x34d, 0x00},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_video_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 660Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2048 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x01}, /* x_start_addr = 320 */
	{0x345, 0x40},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x09}, /* x_addr_end = 2367 */
	{0x349, 0x3f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x08}, /* x_output_size = 2048 */
	{0x34d, 0x00},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_fast_video_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 696Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 1344 x 760 */
/*----------------------------------------------- */
	{0x344, 0x00}, /* x_start_addr = 0 */
	{0x345, 0x00},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x0a}, /* x_addr_end = 2687 */
	{0x349, 0x7f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x05}, /* x_output_size = 1344 */
	{0x34d, 0x40},
	{0x34e, 0x02}, /* y_output_size = 760 */
	{0x34f, 0xf8},
	{0x382, 0x00}, /* x_odd_inc = 3 */
	{0x383, 0x03},
	{0x386, 0x00}, /* y_odd_inc = 3 */
	{0x387, 0x03},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_snap_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 696Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2688 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x00}, /* x_start_addr = 0 */
	{0x345, 0x00},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x0a}, /* x_addr_end = 2687 */
	{0x349, 0x7f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x0a}, /* x_output_size = 2688 */
	{0x34d, 0x80},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_16_9_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 696Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2688 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x00}, /* x_start_addr = 0 */
	{0x345, 0x00},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x0a}, /* x_addr_end = 2687 */
	{0x349, 0x7f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x0a}, /* x_output_size = 2688 */
	{0x34d, 0x80},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_4_3_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 660Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2048 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x01}, /* x_start_addr = 320 */
	{0x345, 0x40},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x09}, /* x_addr_end = 2367 */
	{0x349, 0x3f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x08}, /* x_output_size = 2048 */
	{0x34d, 0x00},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_snap_wide_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 660Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2048 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x01}, /* x_start_addr = 320 */
	{0x345, 0x40},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x09}, /* x_addr_end = 2367 */
	{0x349, 0x3f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x08}, /* x_output_size = 2048 */
	{0x34d, 0x00},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

 /* TMP - Now it's same as FULL SIZE */
 static struct msm_camera_i2c_reg_conf vd6869_night_settings[] = {
/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x900, 0x00}, /* full res = 0, binning = 1 */

/*----------------------------------------------- */
/* Video timing - 250Mbps */
/*----------------------------------------------- */
	{0x300, 0x00}, /* vt_pix_clk_div = 5 */
	{0x301, 0x05},
	{0x302, 0x00}, /* vt_sys_clk_div = 1 */
	{0x303, 0x01},
	{0x30a, 0x00}, /* op_sys_clk_div = 1 */
	{0x30b, 0x01},

	{0x340, 0x03}, /* frame_length_lines = 795 = (1590/2) lines */
	{0x341, 0x17},
	{0x342, 0x0b}, /* line_length_pck = 2888 pcks */
	{0x343, 0x54},

/*----------------------------------------------- */
/* Output image size - 2048 x 1520 */
/*----------------------------------------------- */
	{0x344, 0x01}, /* x_start_addr = 320 */
	{0x345, 0x40},
	{0x346, 0x00}, /* y_start_addr = 0 */
	{0x347, 0x00},
	{0x348, 0x09}, /* x_addr_end = 2367 */
	{0x349, 0x3f},
	{0x34a, 0x05}, /* y_addr_end = 1519 */
	{0x34b, 0xef},
	{0x34c, 0x08}, /* x_output_size = 2048 */
	{0x34d, 0x00},
	{0x34e, 0x05}, /* y_output_size = 1520 */
	{0x34f, 0xf0},
	{0x382, 0x00}, /* x_odd_inc = 1 */
	{0x383, 0x01},
	{0x386, 0x00}, /* y_odd_inc = 1 */
	{0x387, 0x01},

/*----------------------------------------------- */
/* NonHDR / HDR settings */
/*----------------------------------------------- */
	{0x3339, 0x00}, /* vtiming long2short offset = 0 (To set only in HDR Staggered) */

/*----------------------------------------------- */
/* Exposure and gains (normal mode) */
/*----------------------------------------------- */
	{0x202, 0x01}, /* coarse_integration_time = 256 */
	{0x203, 0x00},
	{0x204, 0x00}, /* gain 2.0 == 8 << 4 == 128 */
	{0x205, 0x80},
	{0x20e, 0x01}, /* digital_gain_greenR	= 1.0x	(fixed point 4.8) */
	{0x20f, 0x00},
	{0x210, 0x01}, /* digital_gain_red		= 1.0x	(fixed point 4.8) */
	{0x211, 0x00},
	{0x212, 0x01}, /* digital_gain_blue	= 1.0x	(fixed point 4.8) */
	{0x213, 0x00},
	{0x214, 0x01}, /* digital_gain_greenB	= 1.0x	(fixed point 4.8) */
	{0x215, 0x00},
};

static struct msm_camera_i2c_reg_conf vd6869_fw_settings[] = {
/*///////////////////////////////////////////////// */
/* Patch version: 1.28 */
/*///////////////////////////////////////////////// */
	{0x4440, 0x80},
	{0x4100, 0x03},
	{0x6000, 0x90},
	{0x6001, 0x33},
	{0x6002, 0x39},
	{0x6003, 0xe0},
	{0x6004, 0x90},
	{0x6005, 0x54},
	{0x6006, 0x5d},
	{0x6007, 0xf0},
	{0x6008, 0x02},
	{0x6009, 0x38},
	{0x600a, 0x52},
	{0x600b, 0xc3},
	{0x600c, 0x13},
	{0x600d, 0xc3},
	{0x600e, 0x13},
	{0x600f, 0x90},
	{0x6010, 0x54},
	{0x6011, 0xa8},
	{0x6012, 0x02},
	{0x6013, 0x20},
	{0x6014, 0xfa},
	{0x6015, 0xe0},
	{0x6016, 0xc3},
	{0x6017, 0x13},
	{0x6018, 0xc3},
	{0x6019, 0x13},
	{0x601a, 0x02},
	{0x601b, 0x21},
	{0x601c, 0x88},
	{0x601d, 0xc3},
	{0x601e, 0x13},
	{0x601f, 0xc3},
	{0x6020, 0x13},
	{0x6021, 0x90},
	{0x6022, 0x54},
	{0x6023, 0xa8},
	{0x6024, 0x02},
	{0x6025, 0x22},
	{0x6026, 0x3a},
	{0x6027, 0x90},
	{0x6028, 0x54},
	{0x6029, 0x00},
	{0x602a, 0xe0},
	{0x602b, 0xb4},
	{0x602c, 0x00},
	{0x602d, 0x0c},
	{0x602e, 0x90},
	{0x602f, 0x31},
	{0x6030, 0x47},
	{0x6031, 0xe0},
	{0x6032, 0xc3},
	{0x6033, 0x13},
	{0x6034, 0xff},
	{0x6035, 0xa3},
	{0x6036, 0xe0},
	{0x6037, 0x13},
	{0x6038, 0x80},
	{0x6039, 0x07},
	{0x603a, 0x90},
	{0x603b, 0x31},
	{0x603c, 0x47},
	{0x603d, 0xe0},
	{0x603e, 0xff},
	{0x603f, 0xa3},
	{0x6040, 0xe0},
	{0x6041, 0x02},
	{0x6042, 0x27},
	{0x6043, 0xa5},
	{0x6044, 0x02},
	{0x6045, 0x2e},
	{0x6046, 0x1d},
	{0x6047, 0x90},
	{0x6048, 0x55},
	{0x6049, 0xbc},
	{0x604a, 0xe0},
	{0x604b, 0xb4},
	{0x604c, 0x01},
	{0x604d, 0x17},
	{0x604e, 0x74},
	{0x604f, 0x3b},
	{0x6050, 0x29},
	{0x6051, 0xf8},
	{0x6052, 0xe6},
	{0x6053, 0x11},
	{0x6054, 0xc9},
	{0x6055, 0x70},
	{0x6056, 0x02},
	{0x6057, 0x05},
	{0x6058, 0x0b},
	{0x6059, 0x14},
	{0x605a, 0x78},
	{0x605b, 0x02},
	{0x605c, 0xc3},
	{0x605d, 0x33},
	{0x605e, 0xce},
	{0x605f, 0x33},
	{0x6060, 0xce},
	{0x6061, 0xd8},
	{0x6062, 0xf9},
	{0x6063, 0x80},
	{0x6064, 0x5a},
	{0x6065, 0xe9},
	{0x6066, 0x64},
	{0x6067, 0x02},
	{0x6068, 0x70},
	{0x6069, 0x40},
	{0x606a, 0x78},
	{0x606b, 0x3e},
	{0x606c, 0xe6},
	{0x606d, 0x25},
	{0x606e, 0xe0},
	{0x606f, 0xfe},
	{0x6070, 0x18},
	{0x6071, 0xe6},
	{0x6072, 0x33},
	{0x6073, 0x90},
	{0x6074, 0x33},
	{0x6075, 0x34},
	{0x6076, 0xf0},
	{0x6077, 0xa3},
	{0x6078, 0xce},
	{0x6079, 0xf0},
	{0x607a, 0x90},
	{0x607b, 0x33},
	{0x607c, 0x34},
	{0x607d, 0xe0},
	{0x607e, 0xa3},
	{0x607f, 0x11},
	{0x6080, 0xc9},
	{0x6081, 0x70},
	{0x6082, 0x02},
	{0x6083, 0x05},
	{0x6084, 0x0b},
	{0x6085, 0x14},
	{0x6086, 0x78},
	{0x6087, 0x02},
	{0x6088, 0xc3},
	{0x6089, 0x33},
	{0x608a, 0xce},
	{0x608b, 0x33},
	{0x608c, 0xce},
	{0x608d, 0xd8},
	{0x608e, 0xf9},
	{0x608f, 0x11},
	{0x6090, 0xd1},
	{0x6091, 0x90},
	{0x6092, 0x33},
	{0x6093, 0x34},
	{0x6094, 0xa3},
	{0x6095, 0x11},
	{0x6096, 0xc8},
	{0x6097, 0x70},
	{0x6098, 0x02},
	{0x6099, 0x05},
	{0x609a, 0x0b},
	{0x609b, 0x14},
	{0x609c, 0x78},
	{0x609d, 0x02},
	{0x609e, 0xc3},
	{0x609f, 0x33},
	{0x60a0, 0xce},
	{0x60a1, 0x33},
	{0x60a2, 0xce},
	{0x60a3, 0xd8},
	{0x60a4, 0xf9},
	{0x60a5, 0x11},
	{0x60a6, 0xd1},
	{0x60a7, 0x09},
	{0x60a8, 0x80},
	{0x60a9, 0x17},
	{0x60aa, 0x74},
	{0x60ab, 0x3b},
	{0x60ac, 0x29},
	{0x60ad, 0xf8},
	{0x60ae, 0xe6},
	{0x60af, 0x11},
	{0x60b0, 0xc9},
	{0x60b1, 0x70},
	{0x60b2, 0x02},
	{0x60b3, 0x05},
	{0x60b4, 0x0b},
	{0x60b5, 0x14},
	{0x60b6, 0x78},
	{0x60b7, 0x02},
	{0x60b8, 0xc3},
	{0x60b9, 0x33},
	{0x60ba, 0xce},
	{0x60bb, 0x33},
	{0x60bc, 0xce},
	{0x60bd, 0xd8},
	{0x60be, 0xf9},
	{0x60bf, 0x11},
	{0x60c0, 0xd1},
	{0x60c1, 0x02},
	{0x60c2, 0x09},
	{0x60c3, 0x94},
	{0x60c4, 0x00},
	{0x60c5, 0x00},
	{0x60c6, 0x00},
	{0x60c7, 0x00},
	{0x60c8, 0xe0},
	{0x60c9, 0xfb},
	{0x60ca, 0x05},
	{0x60cb, 0x0c},
	{0x60cc, 0xe5},
	{0x60cd, 0x0c},
	{0x60ce, 0xae},
	{0x60cf, 0x0b},
	{0x60d0, 0x22},
	{0x60d1, 0x12},
	{0x60d2, 0x0a},
	{0x60d3, 0x83},
	{0x60d4, 0x22},
	{0x60d5, 0x3e},
	{0x60d6, 0xf5},
	{0x60d7, 0x83},
	{0x60d8, 0xa3},
	{0x60d9, 0xa3},
	{0x60da, 0xe4},
	{0x60db, 0xf0},
	{0x60dc, 0x22},
	{0x60fa, 0x90},
	{0x60fb, 0x31},
	{0x60fc, 0x51},
	{0x60fd, 0xe0},
	{0x60fe, 0x44},
	{0x60ff, 0x50},
	{0x6100, 0xfe},
	{0x6101, 0xa3},
	{0x6102, 0xe0},
	{0x6103, 0xff},
	{0x6104, 0x02},
	{0x6105, 0x2e},
	{0x6106, 0x47},
	{0x6107, 0x00},
	{0x6108, 0x00},
	{0x6109, 0x00},
	{0x610a, 0x00},
	{0x610b, 0x00},
	{0x610c, 0x90},
	{0x610d, 0x39},
	{0x610e, 0x0b},
	{0x610f, 0xe0},
	{0x6110, 0xb4},
	{0x6111, 0x01},
	{0x6112, 0x04},
	{0x6113, 0x74},
	{0x6114, 0x05},
	{0x6115, 0x80},
	{0x6116, 0x02},
	{0x6117, 0x74},
	{0x6118, 0x06},
	{0x6119, 0x90},
	{0x611a, 0x55},
	{0x611b, 0x0c},
	{0x611c, 0xf0},
	{0x611d, 0x90},
	{0x611e, 0x55},
	{0x611f, 0x04},
	{0x6120, 0xf0},
	{0x6121, 0x90},
	{0x6122, 0x48},
	{0x6123, 0xa4},
	{0x6124, 0x74},
	{0x6125, 0x16},
	{0x6126, 0xf0},
	{0x6127, 0x90},
	{0x6128, 0x33},
	{0x6129, 0x37},
	{0x612a, 0x02},
	{0x612b, 0x38},
	{0x612c, 0x55},
	{0x6137, 0x90},
	{0x6138, 0x30},
	{0x6139, 0x46},
	{0x613a, 0xe0},
	{0x613b, 0x44},
	{0x613c, 0x50},
	{0x613d, 0xfe},
	{0x613e, 0xa3},
	{0x613f, 0xe0},
	{0x6140, 0xff},
	{0x6141, 0x02},
	{0x6142, 0x2e},
	{0x6143, 0x55},
	{0x6144, 0x90},
	{0x6145, 0x39},
	{0x6146, 0x0a},
	{0x6147, 0xe0},
	{0x6148, 0xff},
	{0x6149, 0x90},
	{0x614a, 0x38},
	{0x614b, 0xbb},
	{0x614c, 0xe0},
	{0x614d, 0x54},
	{0x614e, 0x0f},
	{0x614f, 0xb5},
	{0x6150, 0x07},
	{0x6151, 0x08},
	{0x6152, 0x74},
	{0x6153, 0x01},
	{0x6154, 0x90},
	{0x6155, 0x32},
	{0x6156, 0xfb},
	{0x6157, 0xf0},
	{0x6158, 0x80},
	{0x6159, 0x06},
	{0x615a, 0x24},
	{0x615b, 0x01},
	{0x615c, 0x90},
	{0x615d, 0x32},
	{0x615e, 0xfb},
	{0x615f, 0xf0},
	{0x6160, 0x90},
	{0x6161, 0x54},
	{0x6162, 0x70},
	{0x6163, 0x02},
	{0x6164, 0x1d},
	{0x6165, 0x87},
	{0x6166, 0x90},
	{0x6167, 0x32},
	{0x6168, 0xfb},
	{0x6169, 0x74},
	{0x616a, 0x01},
	{0x616b, 0xf0},
	{0x616c, 0x90},
	{0x616d, 0x38},
	{0x616e, 0xbb},
	{0x616f, 0x02},
	{0x6170, 0x0c},
	{0x6171, 0x93},
	{0x6172, 0x02},
	{0x6173, 0x36},
	{0x6174, 0x20},
	{0x6175, 0x02},
	{0x6176, 0x22},
	{0x6177, 0x28},
	{0x618a, 0x74},
	{0x618b, 0x19},
	{0x618c, 0x02},
	{0x618d, 0x0a},
	{0x618e, 0x09},
	{0x6190, 0x90},
	{0x6191, 0x45},
	{0x6192, 0x90},
	{0x6193, 0x74},
	{0x6194, 0x93},
	{0x6195, 0xf0},
	{0x6196, 0xa3},
	{0x6197, 0xa3},
	{0x6198, 0xa3},
	{0x6199, 0xa3},
	{0x619a, 0xf0},
	{0x619b, 0xa3},
	{0x619c, 0xa3},
	{0x619d, 0xa3},
	{0x619e, 0xa3},
	{0x619f, 0xf0},
	{0x61a0, 0xa3},
	{0x61a1, 0xa3},
	{0x61a2, 0xa3},
	{0x61a3, 0xa3},
	{0x61a4, 0xf0},
	{0x61a5, 0x02},
	{0x61a6, 0x10},
	{0x61a7, 0xd5},
	{0x61a8, 0x02},
	{0x61a9, 0x0c},
	{0x61aa, 0xc0},
	{0x61ac, 0x74},
	{0x61ad, 0x52},
	{0x61ae, 0x90},
	{0x61af, 0x48},
	{0x61b0, 0xf1},
	{0x61b1, 0xf0},
	{0x61b2, 0x74},
	{0x61b3, 0x6b},
	{0x61b4, 0x90},
	{0x61b5, 0x48},
	{0x61b6, 0xfd},
	{0x61b7, 0xf0},
	{0x61b8, 0x74},
	{0x61b9, 0x12},
	{0x61ba, 0x90},
	{0x61bb, 0x49},
	{0x61bc, 0x09},
	{0x61bd, 0xf0},
	{0x61be, 0x74},
	{0x61bf, 0x2b},
	{0x61c0, 0x90},
	{0x61c1, 0x49},
	{0x61c2, 0x15},
	{0x61c3, 0xf0},
	{0x61c4, 0x90},
	{0x61c5, 0x49},
	{0x61c6, 0xa5},
	{0x61c7, 0xf0},
	{0x61c8, 0x02},
	{0x61c9, 0x1a},
	{0x61ca, 0x6e},
	{0x61e9, 0x90},
	{0x61ea, 0x54},
	{0x61eb, 0x0c},
	{0x61ec, 0xe5},
	{0x61ed, 0x0c},
	{0x61ee, 0xf0},
	{0x61ef, 0x90},
	{0x61f0, 0x55},
	{0x61f1, 0x14},
	{0x61f2, 0xe5},
	{0x61f3, 0x0b},
	{0x61f4, 0xf0},
	{0x61f5, 0x90},
	{0x61f6, 0x55},
	{0x61f7, 0x10},
	{0x61f8, 0xe4},
	{0x61f9, 0xf0},
	{0x61fa, 0xa3},
	{0x61fb, 0x74},
	{0x61fc, 0x44},
	{0x61fd, 0xf0},
	{0x61fe, 0xf5},
	{0x61ff, 0x0a},
	{0x6200, 0x02},
	{0x6201, 0x37},
	{0x6202, 0xfd},
	{0x6219, 0x90},
	{0x621a, 0x01},
	{0x621b, 0x01},
	{0x621c, 0xe0},
	{0x621d, 0xfe},
	{0x621e, 0x54},
	{0x621f, 0x01},
	{0x6220, 0x00},
	{0x6221, 0x00},
	{0x6222, 0xb4},
	{0x6223, 0x00},
	{0x6224, 0x03},
	{0x6225, 0x02},
	{0x6226, 0x34},
	{0x6227, 0xa4},
	{0x6228, 0x02},
	{0x6229, 0x34},
	{0x622a, 0xab},
	{0x622b, 0x02},
	{0x622c, 0x0d},
	{0x622d, 0x51},
	{0x625c, 0xe5},
	{0x625d, 0x09},
	{0x625e, 0x70},
	{0x625f, 0x3f},
	{0x6260, 0x90},
	{0x6261, 0x39},
	{0x6262, 0x0b},
	{0x6263, 0xe0},
	{0x6264, 0xb4},
	{0x6265, 0x03},
	{0x6266, 0x06},
	{0x6267, 0x90},
	{0x6268, 0x55},
	{0x6269, 0x98},
	{0x626a, 0x02},
	{0x626b, 0x32},
	{0x626c, 0xbf},
	{0x626d, 0x90},
	{0x626e, 0x55},
	{0x626f, 0xa0},
	{0x6270, 0x12},
	{0x6271, 0x0f},
	{0x6272, 0x89},
	{0x6273, 0x12},
	{0x6274, 0x0f},
	{0x6275, 0x82},
	{0x6276, 0xee},
	{0x6277, 0xf0},
	{0x6278, 0xa3},
	{0x6279, 0xef},
	{0x627a, 0xf0},
	{0x627b, 0x90},
	{0x627c, 0x55},
	{0x627d, 0xa4},
	{0x627e, 0x12},
	{0x627f, 0x0f},
	{0x6280, 0x89},
	{0x6281, 0x8c},
	{0x6282, 0x83},
	{0x6283, 0x24},
	{0x6284, 0x04},
	{0x6285, 0xf5},
	{0x6286, 0x82},
	{0x6287, 0xe4},
	{0x6288, 0x3c},
	{0x6289, 0x12},
	{0x628a, 0x32},
	{0x628b, 0x17},
	{0x628c, 0x90},
	{0x628d, 0x55},
	{0x628e, 0x98},
	{0x628f, 0x12},
	{0x6290, 0x0f},
	{0x6291, 0x89},
	{0x6292, 0x12},
	{0x6293, 0x34},
	{0x6294, 0xe9},
	{0x6295, 0x3c},
	{0x6296, 0x12},
	{0x6297, 0x32},
	{0x6298, 0x17},
	{0x6299, 0x90},
	{0x629a, 0x55},
	{0x629b, 0x9c},
	{0x629c, 0x02},
	{0x629d, 0x32},
	{0x629e, 0xe8},
	{0x629f, 0x02},
	{0x62a0, 0x32},
	{0x62a1, 0xea},
	{0x62a2, 0x75},
	{0x62a3, 0x09},
	{0x62a4, 0x08},
	{0x62a5, 0x75},
	{0x62a6, 0x0a},
	{0x62a7, 0x44},
	{0x62a8, 0x02},
	{0x62a9, 0x38},
	{0x62aa, 0x41},
	{0x62b3, 0x90},
	{0x62b4, 0x54},
	{0x62b5, 0x0c},
	{0x62b6, 0x74},
	{0x62b7, 0x0e},
	{0x62b8, 0xf0},
	{0x62b9, 0x75},
	{0x62ba, 0x0c},
	{0x62bb, 0x0e},
	{0x62bc, 0x02},
	{0x62bd, 0x38},
	{0x62be, 0x70},
	{0x62c0, 0xe5},
	{0x62c1, 0x09},
	{0x62c2, 0x70},
	{0x62c3, 0x48},
	{0x62c4, 0x90},
	{0x62c5, 0x01},
	{0x62c6, 0x01},
	{0x62c7, 0xe0},
	{0x62c8, 0x54},
	{0x62c9, 0x02},
	{0x62ca, 0x03},
	{0x62cb, 0x70},
	{0x62cc, 0x07},
	{0x62cd, 0x90},
	{0x62ce, 0x39},
	{0x62cf, 0x0b},
	{0x62d0, 0xe0},
	{0x62d1, 0xb4},
	{0x62d2, 0x03},
	{0x62d3, 0x06},
	{0x62d4, 0x90},
	{0x62d5, 0x55},
	{0x62d6, 0x98},
	{0x62d7, 0x02},
	{0x62d8, 0x32},
	{0x62d9, 0xbf},
	{0x62da, 0x90},
	{0x62db, 0x55},
	{0x62dc, 0xa0},
	{0x62dd, 0x12},
	{0x62de, 0x0f},
	{0x62df, 0x89},
	{0x62e0, 0x12},
	{0x62e1, 0x0f},
	{0x62e2, 0x82},
	{0x62e3, 0xee},
	{0x62e4, 0xf0},
	{0x62e5, 0xa3},
	{0x62e6, 0xef},
	{0x62e7, 0xf0},
	{0x62e8, 0x90},
	{0x62e9, 0x55},
	{0x62ea, 0xa4},
	{0x62eb, 0x12},
	{0x62ec, 0x0f},
	{0x62ed, 0x89},
	{0x62ee, 0x8c},
	{0x62ef, 0x83},
	{0x62f0, 0x24},
	{0x62f1, 0x04},
	{0x62f2, 0xf5},
	{0x62f3, 0x82},
	{0x62f4, 0xe4},
	{0x62f5, 0x3c},
	{0x62f6, 0x12},
	{0x62f7, 0x32},
	{0x62f8, 0x17},
	{0x62f9, 0x90},
	{0x62fa, 0x55},
	{0x62fb, 0x98},
	{0x62fc, 0x12},
	{0x62fd, 0x0f},
	{0x62fe, 0x89},
	{0x62ff, 0x12},
	{0x6300, 0x34},
	{0x6301, 0xe9},
	{0x6302, 0x3c},
	{0x6303, 0x12},
	{0x6304, 0x32},
	{0x6305, 0x17},
	{0x6306, 0x90},
	{0x6307, 0x55},
	{0x6308, 0x9c},
	{0x6309, 0x02},
	{0x630a, 0x32},
	{0x630b, 0xe8},
	{0x630c, 0x00},
	{0x630d, 0x00},
	{0x630e, 0x00},
	{0x630f, 0x90},
	{0x6310, 0x01},
	{0x6311, 0x01},
	{0x6312, 0xe0},
	{0x6313, 0x54},
	{0x6314, 0x02},
	{0x6315, 0x03},
	{0x6316, 0x60},
	{0x6317, 0x07},
	{0x6318, 0x90},
	{0x6319, 0x39},
	{0x631a, 0x0b},
	{0x631b, 0xe0},
	{0x631c, 0xb4},
	{0x631d, 0x03},
	{0x631e, 0x06},
	{0x631f, 0x90},
	{0x6320, 0x55},
	{0x6321, 0xa8},
	{0x6322, 0x02},
	{0x6323, 0x32},
	{0x6324, 0xed},
	{0x6325, 0x90},
	{0x6326, 0x55},
	{0x6327, 0xb0},
	{0x6328, 0x12},
	{0x6329, 0x0f},
	{0x632a, 0x89},
	{0x632b, 0x12},
	{0x632c, 0x0f},
	{0x632d, 0x82},
	{0x632e, 0xee},
	{0x632f, 0xf0},
	{0x6330, 0xa3},
	{0x6331, 0xef},
	{0x6332, 0xf0},
	{0x6333, 0x90},
	{0x6334, 0x55},
	{0x6335, 0xb4},
	{0x6336, 0x12},
	{0x6337, 0x0f},
	{0x6338, 0x89},
	{0x6339, 0x8c},
	{0x633a, 0x83},
	{0x633b, 0x24},
	{0x633c, 0x04},
	{0x633d, 0xf5},
	{0x633e, 0x82},
	{0x633f, 0xe4},
	{0x6340, 0x3c},
	{0x6341, 0x12},
	{0x6342, 0x32},
	{0x6343, 0x17},
	{0x6344, 0x90},
	{0x6345, 0x55},
	{0x6346, 0xa8},
	{0x6347, 0x12},
	{0x6348, 0x0f},
	{0x6349, 0x89},
	{0x634a, 0x12},
	{0x634b, 0x34},
	{0x634c, 0xe9},
	{0x634d, 0x3c},
	{0x634e, 0x12},
	{0x634f, 0x32},
	{0x6350, 0x17},
	{0x6351, 0x90},
	{0x6352, 0x55},
	{0x6353, 0xac},
	{0x6354, 0x02},
	{0x6355, 0x33},
	{0x6356, 0x16},
	{0x6357, 0x02},
	{0x6358, 0x20},
	{0x6359, 0x8a},
	{0x635a, 0x90},
	{0x635b, 0x45},
	{0x635c, 0x90},
	{0x635d, 0xe0},
	{0x635e, 0x54},
	{0x635f, 0xcd},
	{0x6360, 0xf0},
	{0x6361, 0x90},
	{0x6362, 0x45},
	{0x6363, 0x94},
	{0x6364, 0xe0},
	{0x6365, 0x54},
	{0x6366, 0xcd},
	{0x6367, 0xf0},
	{0x6368, 0x90},
	{0x6369, 0x45},
	{0x636a, 0x98},
	{0x636b, 0xe0},
	{0x636c, 0x54},
	{0x636d, 0xcd},
	{0x636e, 0xf0},
	{0x636f, 0x90},
	{0x6370, 0x45},
	{0x6371, 0x9c},
	{0x6372, 0xe0},
	{0x6373, 0x54},
	{0x6374, 0xcd},
	{0x6375, 0xf0},
	{0x6376, 0x02},
	{0x6377, 0x11},
	{0x6378, 0xfa},
	{0x4104, 0xd4},
	{0x4105, 0x0a},
	{0x4108, 0x32},
	{0x410a, 0x00},
	{0x410c, 0x03},
	{0x4110, 0xdf},
	{0x4111, 0x0a},
	{0x4114, 0x8d},
	{0x4115, 0x00},
	{0x4118, 0x03},
	{0x411c, 0x45},
	{0x411d, 0x38},
	{0x4120, 0x00},
	{0x4121, 0x00},
	{0x4124, 0x02},
	{0x4128, 0xf7},
	{0x4129, 0x20},
	{0x412c, 0x0b},
	{0x412d, 0x00},
	{0x4130, 0x02},
	{0x4134, 0x87},
	{0x4135, 0x21},
	{0x4138, 0x15},
	{0x4139, 0x00},
	{0x413c, 0x02},
	{0x4140, 0x37},
	{0x4141, 0x22},
	{0x4144, 0x1d},
	{0x4145, 0x00},
	{0x4148, 0x02},
	{0x414c, 0x9e},
	{0x414d, 0x27},
	{0x4150, 0x27},
	{0x4151, 0x00},
	{0x4154, 0x02},
	{0x4158, 0x3b},
	{0x4159, 0x38},
	{0x415c, 0xa2},
	{0x415d, 0x02},
	{0x4160, 0x02},
	{0x4164, 0x78},
	{0x4165, 0x09},
	{0x4168, 0x47},
	{0x4169, 0x00},
	{0x416c, 0x02},
	{0x4170, 0xb8},
	{0x4171, 0x32},
	{0x4174, 0xc0},
	{0x4175, 0x02},
	{0x4178, 0x02},
	{0x417c, 0x40},
	{0x417d, 0x2e},
	{0x4180, 0xfa},
	{0x4181, 0x00},
	{0x4184, 0x02},
	{0x4188, 0xea},
	{0x4189, 0x32},
	{0x418c, 0x0f},
	{0x418d, 0x03},
	{0x4190, 0x02},
	{0x4194, 0xf6},
	{0x4195, 0x11},
	{0x4198, 0x5a},
	{0x4199, 0x03},
	{0x419c, 0x02},
	{0x41a0, 0x4f},
	{0x41a1, 0x2e},
	{0x41a4, 0x37},
	{0x41a5, 0x01},
	{0x41a8, 0x02},
	{0x41ac, 0x64},
	{0x41ad, 0x21},
	{0x41b0, 0x25},
	{0x41b1, 0x00},
	{0x41b4, 0x03},
	{0x41b8, 0x84},
	{0x41b9, 0x1d},
	{0x41bc, 0x44},
	{0x41bd, 0x01},
	{0x41c0, 0x02},
	{0x41c4, 0x90},
	{0x41c5, 0x0c},
	{0x41c8, 0x66},
	{0x41c9, 0x01},
	{0x41cc, 0x02},
	{0x41d0, 0x1a},
	{0x41d1, 0x36},
	{0x41d4, 0x72},
	{0x41d5, 0x01},
	{0x41d8, 0x02},
	{0x41dc, 0x22},
	{0x41dd, 0x22},
	{0x41e0, 0x75},
	{0x41e1, 0x01},
	{0x41e4, 0x02},
	{0x41e8, 0x7f},
	{0x41e9, 0x20},
	{0x41ec, 0x57},
	{0x41ed, 0x03},
	{0x41f0, 0x02},
	{0x41f4, 0xef},
	{0x41f5, 0x0a},
	{0x41f8, 0x5b},
	{0x41f9, 0x00},
	{0x41fc, 0x03},
	{0x4200, 0xfb},
	{0x4201, 0x0a},
	{0x4204, 0x31},
	{0x4205, 0x00},
	{0x4208, 0x03},
	{0x420c, 0x04},
	{0x420d, 0x0a},
	{0x4210, 0x8a},
	{0x4211, 0x01},
	{0x4214, 0x02},
	{0x4218, 0xfa},
	{0x4219, 0x37},
	{0x421c, 0xe9},
	{0x421d, 0x01},
	{0x4220, 0x02},
	{0x4224, 0xa1},
	{0x4225, 0x34},
	{0x4228, 0x19},
	{0x4229, 0x02},
	{0x422c, 0x02},
	{0x4230, 0x6d},
	{0x4231, 0x38},
	{0x4234, 0xb6},
	{0x4235, 0x02},
	{0x4238, 0x02},
	{0x423c, 0x46},
	{0x423d, 0x23},
	{0x4240, 0x29},
	{0x4241, 0x00},
	{0x4244, 0x03},
	{0x4248, 0xaa},
	{0x4249, 0x14},
	{0x424c, 0x85},
	{0x424d, 0x00},
	{0x4250, 0x03},
	{0x4254, 0x61},
	{0x4255, 0x1a},
	{0x4258, 0xac},
	{0x4259, 0x01},
	{0x425c, 0x02},
	{0x4260, 0xc5},
	{0x4261, 0x10},
	{0x4264, 0x90},
	{0x4265, 0x01},
	{0x4268, 0x02},
	{0x426c, 0x35},
	{0x426d, 0x0d},
	{0x4270, 0x2b},
	{0x4271, 0x02},
	{0x4274, 0x02},
	{0x4278, 0x52},
	{0x4279, 0x38},
	{0x427c, 0x0c},
	{0x427d, 0x01},
	{0x4280, 0x02},
	{0x4100, 0x01},
	{0x4440, 0x7f},
};

static struct msm_camera_i2c_reg_conf vd6869_version_settings[] = {
	{0x3bfe, 0x01},
	{0x3bff, 0x1c},
};

static struct msm_camera_i2c_reg_conf vd6869_general_settings[] = {
/*///////////////////////////////////////////////// */
/* Sensor General Settings */
/*///////////////////////////////////////////////// */

/*----------------------------------------------- */
/* Pre-streaming internal configuration */
/*----------------------------------------------- */
	{0x33be, 0x0B}, /* Output formatting */

/*----------------------------------------------- */
/* Sensor configuration */
/*----------------------------------------------- */
	{0x101, 0x03}, /* image_orientation = flip + mirror */
	{0x114, 0x03}, /* csi_lane_mode = quad lane */
	{0x121, 0x18}, /* extclk_frequency_mhz = 24.0Mhz */
	{0x122, 0x00},

/*----------------------------------------------- */
/* Video timing - PLL clock = 660MHz */
/*----------------------------------------------- */
	{0x304, 0x00}, /* pre_pll_clk_div = 2 */
	{0x305, 0x02},
	{0x306, 0x00}, /* pll_multiplier = 58 */
	{0x307, 0x3a},

/*----------------------------------------------- */
/* HTC module CSI lane mapping */
/*----------------------------------------------- */
/* Silicon Lane1(0) -> Module Lane3(2) (0x489c[1:0]=b10) */
/* Silicon Lane2(1) -> Module Lane1(0) (0x489c[3:2]=b00) */
/* Silicon Lane3(2) -> Module Lane2(1) (0x489c[5:4]=b01) */
/* Silicon Lane4(3) -> Module Lane4(3) (0x489c[7:6]=b11) */
/*----------------------------------------------- */
	{0x489c, 0xd2},

};

static struct msm_camera_i2c_reg_conf vd6869_analog_settings_clearbit[] = {
	{0x3337, 0x10}, /* bit 4 */
};

static struct msm_camera_i2c_reg_conf vd6869_analog_settings[] = {
	{0x5800, 0xC0}, /* vtminor_0 */
	{0x5804, 0x42}, /* vtminor_1 */
	{0x5808, 0x4B}, /* vtminor_2 */
	{0x580c, 0x00}, /* vtminor_3 */
	{0x5810, 0x04}, /* vtminor_4 */
	{0x5814, 0x70}, /* vtminor_5 */
	{0x5818, 0x01}, /* vtminor_6 */
	{0x581c, 0x80}, /* vtminor_7 */
	{0x5820, 0x0C}, /* vtminor_8 */

	{0x5872, 0x1f}, /* streaming option */
	{0x5896, 0xa4}, /* streaming option */
	{0x5800, 0x00}, /* streaming option */

/* Darkcal management */
	{0x4a20, 0x0f}, /* bMode = auto with offset 64 */
	{0x4a22, 0x00}, /* */
	{0x4a23, 0x40},
	{0x4a24, 0x00}, /*  */
	{0x4a25, 0x40},
	{0x4a26, 0x00}, /*  */
	{0x4a27, 0x40},
	{0x4a28, 0x00}, /*  */
	{0x4a29, 0x40},
	{0x33a4, 0x07}, /*  */
	{0x33a5, 0xC0},
	{0x33a6, 0x00}, /*  */
	{0x33a7, 0x40},

	{0x4d20, 0x0f}, /* bMode = auto with offset 64 */
	{0x4d22, 0x00}, /* */
	{0x4d23, 0x40},
	{0x4d24, 0x00}, /*  */
	{0x4d25, 0x40},
	{0x4d26, 0x00}, /*  */
	{0x4d27, 0x40},
	{0x4d28, 0x00}, /*  */
	{0x4d29, 0x40},
	{0x3399, 0x07}, /*  */
	{0x339a, 0xC0},
	{0x339b, 0x00}, /* */
	{0x339c, 0x40},
};

static struct v4l2_subdev_info vd6869_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array vd6869_init_conf[] = {
	{&vd6869_fw_settings[0],
	ARRAY_SIZE(vd6869_fw_settings), 50, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_version_settings[0],
	ARRAY_SIZE(vd6869_version_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_general_settings[0],
	ARRAY_SIZE(vd6869_general_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_analog_settings_clearbit[0],
	ARRAY_SIZE(vd6869_analog_settings_clearbit), 0, MSM_CAMERA_I2C_UNSET_BYTE_MASK},
	{&vd6869_analog_settings[0],
	ARRAY_SIZE(vd6869_analog_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array vd6869_confs[] = {
	{&vd6869_snap_settings[0],
	ARRAY_SIZE(vd6869_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_prev_settings[0],
	ARRAY_SIZE(vd6869_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_video_settings[0],
	ARRAY_SIZE(vd6869_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_fast_video_settings[0],
	ARRAY_SIZE(vd6869_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_16_9_settings[0],
	ARRAY_SIZE(vd6869_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_4_3_settings[0],
	ARRAY_SIZE(vd6869_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_night_settings[0],
	ARRAY_SIZE(vd6869_night_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&vd6869_snap_wide_settings[0],
	ARRAY_SIZE(vd6869_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t vd6869_dimensions[] = {
	{/*full size*/
		.x_output = 0xA80, /* 2688 */
		.y_output = 0x5F0, /* 1520 */
		.line_length_pclk = 0xB54, /* 2900 */
		.frame_length_lines = 0x62E, /* 1582 */
		.vt_pixel_clk = 139200000,
		.op_pixel_clk = 278400000,
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

static struct msm_camera_csid_vc_cfg vd6869_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params vd6869_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 4,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = vd6869_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 4,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *vd6869_csi_params_array[] = {
	&vd6869_csi_params,
	&vd6869_csi_params,
	&vd6869_csi_params,
	&vd6869_csi_params,
	&vd6869_csi_params,
	&vd6869_csi_params
};

static struct msm_sensor_output_reg_addr_t vd6869_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t vd6869_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0365,
};
#define SENSOR_REGISTER_MAX_LINECOUNT 0xffff
#define SENSOR_VERT_OFFSET 25

static struct msm_sensor_exp_gain_info_t vd6869_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = SENSOR_VERT_OFFSET,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
	.sensor_max_linecount = SENSOR_REGISTER_MAX_LINECOUNT-SENSOR_VERT_OFFSET, /* sensor max linecount = max unsigned value of linecount register size - vert_offset */ /* HTC ben 20120229 */
};

int32_t vd6869_set_dig_gain(struct msm_sensor_ctrl_t *s_ctrl, uint16_t dig_gain)
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

static int vd6869_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	if (data->sensor_platform_info)
		vd6869_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;
	/* move setting mirror_flip after sensor size config */

	return rc;
}

static const char *vd6869Vendor = "st";
static const char *vd6869NAME = "vd6869";
static const char *vd6869Size = "cinesensor";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	CDBG("%s called\n", __func__);

	sprintf(buf, "%s %s %s\n", vd6869Vendor, vd6869NAME, vd6869Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_vd6869;

static int vd6869_sysfs_init(void)
{
	int ret ;
	pr_info("%s: vd6869:kobject creat and add\n", __func__);

	android_vd6869 = kobject_create_and_add("android_camera", NULL);
	if (android_vd6869 == NULL) {
		pr_info("vd6869_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("vd6869:sysfs_create_file\n");
	ret = sysfs_create_file(android_vd6869, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("vd6869_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_vd6869);
	}

	return 0 ;
}

int32_t vd6869_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);

	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		vd6869_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id vd6869_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&vd6869_s_ctrl},
	{ }
};

static struct i2c_driver vd6869_i2c_driver = {
	.id_table = vd6869_i2c_id,
	.probe  = vd6869_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client vd6869_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t vd6869_power_up(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
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

	vd6869_sensor_open_init(sdata);
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

int32_t vd6869_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
	pr_info("vd6869 %s\n", __func__);
	return i2c_add_driver(&vd6869_i2c_driver);
}

static struct v4l2_subdev_core_ops vd6869_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops vd6869_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops vd6869_subdev_ops = {
	.core = &vd6869_subdev_core_ops,
	.video  = &vd6869_subdev_video_ops,
};

static int vd6869_read_fuseid(struct sensor_cfg_data *cdata,
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

	pr_info("vd6869: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("vd6869: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("vd6869: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("vd6869: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
#endif
	return 0;

}

static int vd6869_read_VCM_driver_IC_info(	struct msm_sensor_ctrl_t *s_ctrl)
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

void vd6869_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
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
	if (vd6869_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		write_data = 0x00;
	else
		write_data = 0x01;
}

int32_t vd6869_write_exp_gain1_ex_nonHDR(struct msm_sensor_ctrl_t *s_ctrl,
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
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	/* HTC_START pg digi gain 20100710 */
	if (s_ctrl->func_tbl->sensor_set_dig_gain)
		s_ctrl->func_tbl->sensor_set_dig_gain(s_ctrl, dig_gain);
	/* HTC_END pg digi gain 20100710 */
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	return 0;
}

int vd6869_write_output_settings_specific(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	int rc = 0;
	uint16_t value = 0;

	/* Apply sensor mirror/flip */
	if (vd6869_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = VD6869_READ_MIRROR_FLIP;
	else if (vd6869_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = VD6869_READ_MIRROR;
	else if (vd6869_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = VD6869_READ_FLIP;
	else
		value = VD6869_READ_NORMAL_MODE;
	rc = msm_camera_i2c_write(vd6869_s_ctrl.sensor_i2c_client,
		VD6869_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s set mirror_flip failed\n", __func__);
		return rc;
	}

	return rc;
}

static struct msm_sensor_fn_t vd6869_func_tbl = {
	.sensor_start_stream = vd6869_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = vd6869_write_exp_gain1_ex_nonHDR,
	.sensor_write_snapshot_exp_gain_ex = vd6869_write_exp_gain1_ex_nonHDR,
/* HTC_START 20121013 */
/* TMP : disable digital gain - waiting for gain issue fix from ST */
#if 1
	.sensor_set_dig_gain = vd6869_set_dig_gain,
#endif
/* HTC_END */
	.sensor_setting = msm_sensor_setting_parallel,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = vd6869_power_up,
	.sensor_power_down = vd6869_power_down,
	.sensor_i2c_read_fuseid = vd6869_read_fuseid,
	.sensor_i2c_read_vcm_driver_ic = vd6869_read_VCM_driver_IC_info,
	.sensor_write_output_settings_specific = vd6869_write_output_settings_specific,
};

static struct msm_sensor_reg_t vd6869_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = vd6869_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(vd6869_start_settings),
	.stop_stream_conf = vd6869_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(vd6869_stop_settings),
	.group_hold_on_conf = vd6869_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(vd6869_groupon_settings),
	.group_hold_off_conf = vd6869_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(vd6869_groupoff_settings),
	.init_settings = &vd6869_init_conf[0],
	.init_size = ARRAY_SIZE(vd6869_init_conf),
	.mode_settings = &vd6869_confs[0],
	.output_settings = &vd6869_dimensions[0],
	.num_conf = ARRAY_SIZE(vd6869_confs),
};

static struct msm_sensor_ctrl_t vd6869_s_ctrl = {
	.msm_sensor_reg = &vd6869_regs,
	.sensor_i2c_client = &vd6869_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &vd6869_reg_addr,
	.sensor_id_info = &vd6869_id_info,
	.sensor_exp_gain_info = &vd6869_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &vd6869_csi_params_array[0],
	.msm_sensor_mutex = &vd6869_mut,
	.sensor_i2c_driver = &vd6869_i2c_driver,
	.sensor_v4l2_subdev_info = vd6869_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(vd6869_subdev_info),
	.sensor_v4l2_subdev_ops = &vd6869_subdev_ops,
	.func_tbl = &vd6869_func_tbl,
	.sensor_first_mutex = &vd6869_sensor_init_mut, //CC120826,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("ST 4MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
