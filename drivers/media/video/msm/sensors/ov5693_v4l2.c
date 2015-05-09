#include "msm_sensor.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "ov5693"
#define PLATFORM_DRIVER_NAME "msm_camera_ov5693"
#define ov5693_obj ov5693_##obj

//#define OV5693_REG_READ_MODE 0x0101

//#define OV5693_READ_NORMAL_MODE 0x0000	/* without mirror/flip */
//#define OV5693_READ_MIRROR 0x0001			/* with mirror */
//#define OV5693_READ_FLIP 0x0002			/* with flip */
//#define OV5693_READ_MIRROR_FLIP 0x0003	/* with mirror/flip */

#define OV5693_REG_FLIP 0x3820
#define OV5693_REG_MIRROR 0x3821

DEFINE_MUTEX(ov5693_mut);
DEFINE_MUTEX(ov5693_sensor_init_mut); //CC140207,
static struct msm_sensor_ctrl_t ov5693_s_ctrl;

static struct msm_camera_i2c_reg_conf ov5693_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov5693_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf ov5693_groupon_settings[] = {
	{0x3208, 0x00},

};

static struct msm_camera_i2c_reg_conf ov5693_groupoff_settings[] = {
	{0x3208, 0x10},
	{0x3208, 0xA0},
};

static struct msm_camera_i2c_reg_conf ov5693_snap_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0x00}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x07}, /* {380a,380b} Final output V size */
	{0x380b, 0xa0}, /* {380a,380b} Final output V size */

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */
	{0x380f, 0xc8}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_prev_4_3_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0x00}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x07}, /* {380a,380b} Final output V size */
	{0x380b, 0xa0}, /* {380a,380b} Final output V size */

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */
	{0x380f, 0xc8}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

/* HTC_START steven 20130701 ov5693 HD 60fps sensor setting */
static struct msm_camera_i2c_reg_conf ov5693_prev_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x10}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0xf6}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x2f}, /* {3804,3805} Array X end */
	{0x3806, 0x06}, /* {3806,3807} Array Y end */
	{0x3807, 0xab}, /* {3806,3807} Array Y end */

	{0x3808, 0x05}, /* {3808,3809} Final output H size */
	{0x3809, 0x00}, /* {3808,3809} Final output H size */

	{0x380a, 0x02}, /* {380a,380b} Final output V size */
	{0x380b, 0xd0}, /* {380a,380b} Final output V size */

	{0x380c, 0x05}, /* {380c,380d} HTS */ //0x06
	{0x380d, 0xBE}, /* {380c,380d} HTS */ //0xd8
	{0x380e, 0x07}, /* {380e,380f} VTS */ //0x02
	{0x380f, 0x4e}, /* {380e,380f} VTS */ //0xf8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x04}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x31}, /* X subsample control */
	{0x3815, 0x31}, /* Y subsample control */

	{0x3820, 0x04}, /* FLIP/Binnning control */
	{0x3821, 0x1f}, /* MIRROR control */
};
/* HTC_END steven 20130701 ov5693 HD 60fps sensor setting */

static struct msm_camera_i2c_reg_conf ov5693_video_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0x00}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x07}, /* {380a,380b} Final output V size */
	{0x380b, 0xa0}, /* {380a,380b} Final output V size */

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */
	{0x380f, 0xc8}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

/* HFR 1280 x 720 */
static struct msm_camera_i2c_reg_conf ov5693_fast_video_16_9_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x10}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0xf6}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x2f}, /* {3804,3805} Array X end */
	{0x3806, 0x06}, /* {3806,3807} Array Y end */
	{0x3807, 0xab}, /* {3806,3807} Array Y end */

	{0x3808, 0x05}, /* {3808,3809} Final output H size */
	{0x3809, 0x00}, /* {3808,3809} Final output H size */

	{0x380a, 0x02}, /* {380a,380b} Final output V size */
	{0x380b, 0xd0}, /* {380a,380b} Final output V size */

	{0x380c, 0x05}, /* {380c,380d} HTS */ //0x06
	{0x380d, 0xBE}, /* {380c,380d} HTS */ //0xd8
	{0x380e, 0x03}, /* {380e,380f} VTS */ //0x02
	{0x380f, 0xA7}, /* {380e,380f} VTS */ //0xf8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x04}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x31}, /* X subsample control */
	{0x3815, 0x31}, /* Y subsample control */

	{0x3820, 0x04}, /* FLIP/Binnning control */
	{0x3821, 0x1f}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_prev_16_9_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0xF8}, /* {3802,3803} Array Y start */ //0x00

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x05}, /* {380a,380b} Final output V size */ //0x07
	{0x380b, 0xb0}, /* {380a,380b} Final output V size */ //0xa0

	{0x380c, 0x0b}, /* {380c,380d} HTS */
	{0x380d, 0x4c}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */ //0x07
	{0x380f, 0x55}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_fast_video_5_3_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x10}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0xf6}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x2f}, /* {3804,3805} Array X end */
	{0x3806, 0x06}, /* {3806,3807} Array Y end */
	{0x3807, 0xab}, /* {3806,3807} Array Y end */

	{0x3808, 0x04}, /* {3808,3809} Final output H size */
	{0x3809, 0xb0}, /* {3808,3809} Final output H size */

	{0x380a, 0x02}, /* {380a,380b} Final output V size */
	{0x380b, 0xd0}, /* {380a,380b} Final output V size */

	{0x380c, 0x05}, /* {380c,380d} HTS */ //0x06
	{0x380d, 0xBE}, /* {380c,380d} HTS */ //0xd8
	{0x380e, 0x03}, /* {380e,380f} VTS */ //0x02
	{0x380f, 0xA7}, /* {380e,380f} VTS */ //0xf8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x04}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x31}, /* X subsample control */
	{0x3815, 0x31}, /* Y subsample control */

	{0x3820, 0x04}, /* FLIP/Binnning control */
	{0x3821, 0x1f}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_prev_5_3_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0xF8}, /* {3802,3803} Array Y start */ //0x00

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x06}, /* {380a,380b} Final output V size */ //0x07
	{0x380b, 0x10}, /* {380a,380b} Final output V size */ //0xa0

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */ //0x07
	{0x380f, 0x60}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_zoe_settings[] = {
	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0x00}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x07}, /* {380a,380b} Final output V size */
	{0x380b, 0xa0}, /* {380a,380b} Final output V size */

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */
	{0x380f, 0xc8}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3820, 0x00}, /* FLIP/Binnning control */
	{0x3821, 0x1e}, /* MIRROR control */
};

static struct msm_camera_i2c_reg_conf ov5693_recommend_settings[] = {
	/* software sleep/standby */
//	{0x0100, 0x00}, /* <0> sleep mode */
//	{0x0102, 0x01}, /* <0> fast sleep */


	/* system control */
	{0x0103, 0x01}, /* SW reset */
	{0x3001, 0x0a}, /* I/O control */
	{0x3002, 0x80}, /* I/O Input enable */
	{0x3006, 0x00}, /* I/O control */
	{0x3011, 0x21}, /* MIPI control */
	{0x3012, 0x09}, /* MIPI control */
	{0x3013, 0x10}, /* MIPI control */
	{0x3014, 0x00}, /* MIPI control */
	{0x3015, 0x08}, /* MIPI control */
	{0x3016, 0xf0}, /* Debug Mode */
	{0x3017, 0xf0}, /* Debug Mode */ 
	{0x3018, 0xf0}, /* Debug Mode */
	{0x301b, 0xb4}, /* System Clock Control */
	{0x301d, 0x02}, /* FREX mode mask control */
	{0x3021, 0x00}, /* Power Control ( Internal DVDD ) */
	{0x3022, 0x01}, /* Power Control */
	{0x3028, 0x44}, /* Pump Clock Div */


	/* PLL control */
	{0x3090, 0x02}, /* PLL3 control */
	{0x3091, 0x0e}, /* PLL3 control */
	{0x3092, 0x00}, /* PLL3 control */
	{0x3093, 0x00}, /* PLL3 control */
	{0x3098, 0x03}, /* PLL3 control */
	{0x3099, 0x1f}, /* PLL3 control */
	{0x309a, 0x02}, /* PLL3 control */
	{0x309b, 0x01}, /* PLL3 control */
	{0x309c, 0x00}, /* PLL3 control */
	{0x30a0, 0xd2}, /* PLL debug option */
	{0x30a2, 0x01}, /* PLL debug option */
	{0x30b2, 0x00}, /* PLL1 control */
	{0x30b3, 0x68}, /* PLL1 control */ //0x64
	{0x30b4, 0x03}, /* PLL1 control */
	{0x30b5, 0x04}, /* PLL1 control */
	{0x30b6, 0x01}, /* PLL1 control */
	{0x3104, 0x21}, /* PLL debug option */
	{0x3106, 0x00}, /* PLL debug option */

	/* MWB control */
	{0x3400, 0x04}, /* MWB R gain MSB */
	{0x3401, 0x00}, /* MWB R gain MSB */
	{0x3402, 0x04}, /* MWB G gain MSB */
	{0x3403, 0x00}, /* MWB G gain MSB */
	{0x3404, 0x04}, /* MWB B gain MSB */
	{0x3405, 0x00}, /* MWB B gain MSB */
	{0x3406, 0x01}, /* MWB control */

	/* Gain/Exposure control */
	{0x3500, 0x00}, /* long exposure integration time */
	{0x3501, 0x7b}, /* long exposure integration time */
	{0x3502, 0x80}, /* long exposure integration time */
#if 1
	{0x3503, 0x07}, /* gain/exposure control */
#else
	{0x3503, 0x20}, /* gain/exposure control */  /* test : sensor gain auto */
#endif

	{0x3504, 0x00}, /* Gain/exposure debug */
	{0x3505, 0x00}, /* Gain/exposure debug */
	{0x3506, 0x00}, /* short exposure integration time */
	{0x3507, 0x02}, /* short exposure integration time */
	{0x3508, 0x00}, /* short exposure integration time */
	{0x3509, 0x10}, /* Gain debug control */
	{0x350a, 0x00}, /* Gain debug */
	{0x350b, 0x10}, /* Manual Gain */


	/* Analog control */
	{0x3601, 0x0a}, /* Analog control */
	{0x3602, 0x18}, /* Analog control */
	{0x3612, 0x80}, /* Analog control */
	{0x3620, 0x54}, /* Analog control */
	{0x3621, 0xc7}, /* Analog control */
	{0x3622, 0x0f}, /* Analog control */
	{0x3625, 0x10}, /* Analog control */
	{0x3630, 0x55}, /* Analog control */
	{0x3631, 0xf4}, /* Analog control */
	{0x3632, 0x00}, /* Analog control */
	{0x3633, 0x34}, /* Analog control */
	{0x3634, 0x02}, /* Analog control */
	{0x364d, 0x0d}, /* Analog control */
	{0x364f, 0xdd}, /* Analog control */
	{0x3660, 0x04}, /* Analog control */
	{0x3662, 0x10}, /* Analog control */
	{0x3663, 0xf1}, /* Analog control */
	{0x3665, 0x00}, /* Analog control */
	{0x3666, 0x20}, /* Analog control */
	{0x3667, 0x00}, /* Analog control */
	{0x366a, 0x80}, /* Analog control */
	{0x3680, 0xe0}, /* Analog control */
	{0x3681, 0x00}, /* Analog control */

	/* Power save mode */
	{0x3620, 0x44},
	{0x3621, 0xb5},
	{0x3622, 0x0c},
	{0x3600, 0xbc},

	/* Sensor timing control */
	{0x3700, 0x42}, /* sensor timing control */
	{0x3701, 0x14}, /* sensor timing control */
	{0x3702, 0xa0}, /* sensor timing control */
	{0x3703, 0xd8}, /* sensor timing control */
	{0x3704, 0x78}, /* sensor timing control */
	{0x3705, 0x02}, /* sensor timing control */
	{0x3708, 0xe2}, /* sensor timing control */
	{0x3709, 0xc3}, /* sensor timing control */
	{0x370a, 0x00}, /* sensor timing control */
	{0x370b, 0x20}, /* sensor timing control */
	{0x370c, 0x0c}, /* sensor timing control */
	{0x370d, 0x11}, /* sensor timing control */
	{0x370e, 0x00}, /* sensor timing control */
	{0x370f, 0x40}, /* sensor timing control */
	{0x3710, 0x00}, /* sensor timing control */
	{0x371a, 0x1c}, /* sensor timing control */
	{0x371b, 0x05}, /* sensor timing control */
	{0x371c, 0x01}, /* sensor timing control */
	{0x371e, 0xa1}, /* sensor timing control */
	{0x371f, 0x0c}, /* sensor timing control */
	{0x3721, 0x00}, /* sensor timing control */
	{0x3726, 0x00}, /* sensor timing control */
	{0x372a, 0x01}, /* sensor timing control */
	{0x3730, 0x10}, /* sensor timing control */
	{0x3738, 0x22}, /* sensor timing control */
	{0x3739, 0xe5}, /* sensor timing control */
	{0x373a, 0x50}, /* sensor timing control */
	{0x373b, 0x02}, /* sensor timing control */
	{0x373c, 0x41}, /* sensor timing control */
	{0x373f, 0x02}, /* sensor timing control */
	{0x3740, 0x42}, /* sensor timing control */
	{0x3741, 0x02}, /* sensor timing control */
	{0x3742, 0x18}, /* sensor timing control */
	{0x3743, 0x01}, /* sensor timing control */
	{0x3744, 0x02}, /* sensor timing control */
	{0x3747, 0x10}, /* sensor timing control */
	{0x374c, 0x04}, /* sensor timing control */
	{0x3751, 0xf0}, /* sensor timing control */
	{0x3752, 0x00}, /* sensor timing control */
	{0x3753, 0x00}, /* sensor timing control */
	{0x3754, 0xc0}, /* sensor timing control */
	{0x3755, 0x00}, /* sensor timing control */
	{0x3756, 0x1a}, /* sensor timing control */
	{0x3758, 0x00}, /* sensor timing control */
	{0x3759, 0x0f}, /* sensor timing control */
	{0x376b, 0x44}, /* sensor timing control */
	{0x375c, 0x04}, /* sensor timing control */
	{0x3776, 0x00}, /* sensor timing control */
	{0x377f, 0x08}, /* sensor timing control */

	/* PSRAM control */
	{0x3780, 0x22}, /* PSRAM control */
	{0x3781, 0x0c}, /* PSRAM control */
	{0x3784, 0x2c}, /* PSRAM control */
	{0x3785, 0x1e}, /* PSRAM control */
	{0x378f, 0xf5}, /* PSRAM control */
	{0x3791, 0xb0}, /* PSRAM control */
	{0x3795, 0x00}, /* PSRAM control */
	{0x3796, 0x64}, /* PSRAM control */
	{0x3797, 0x11}, /* PSRAM control */
	{0x3798, 0x30}, /* PSRAM control */
	{0x3799, 0x41}, /* PSRAM control */
	{0x379a, 0x07}, /* PSRAM control */
	{0x379b, 0xb0}, /* PSRAM control */
	{0x379c, 0x0c}, /* PSRAM control */

	/* FREX control */
	{0x37c5, 0x00}, /* FREX mode exposure time */
	{0x37c6, 0x00}, /* FREX mode exposure time */
	{0x37c7, 0x00}, /* FREX mode exposure time */

	{0x37c9, 0x00}, /* STROBE width control */
	{0x37ca, 0x00}, /* STROBE width control */
	{0x37cb, 0x00}, /* STROBE width control */

	{0x37de, 0x00}, /* FREX debug mode */
	{0x37df, 0x00}, /* FREX mode control */

	/* Resolution Control */
	{0x3800, 0x00}, /* {3800,3801} Array X start */
	{0x3801, 0x00}, /* {3800,3801} Array X start */
	{0x3802, 0x00}, /* {3802,3803} Array Y start */
	{0x3803, 0x00}, /* {3802,3803} Array Y start */

	{0x3804, 0x0a}, /* {3804,3805} Array X end */
	{0x3805, 0x3f}, /* {3804,3805} Array X end */
	{0x3806, 0x07}, /* {3806,3807} Array Y end */
	{0x3807, 0xa3}, /* {3806,3807} Array Y end */

	{0x3808, 0x0a}, /* {3808,3809} Final output H size */
	{0x3809, 0x20}, /* {3808,3809} Final output H size */

	{0x380a, 0x07}, /* {380a,380b} Final output V size */
	{0x380b, 0xa0}, /* {380a,380b} Final output V size */

	{0x380c, 0x0a}, /* {380c,380d} HTS */
	{0x380d, 0xde}, /* {380c,380d} HTS */ //0x84
	{0x380e, 0x07}, /* {380e,380f} VTS */
	{0x380f, 0xc8}, /* {380e,380f} VTS */ //0xc8

	{0x3810, 0x00}, /* {3810,3811} windowing X offset */
	{0x3811, 0x02}, /* {3810,3811} windowing X offset */
	{0x3812, 0x00}, /* {3812,3813} windowing Y offset */
	{0x3813, 0x02}, /* {3812,3813} windowing Y offset */

	{0x3814, 0x11}, /* X subsample control */
	{0x3815, 0x11}, /* Y subsample control */

	{0x3823, 0x00}, /* timing control */
	{0x3824, 0x00}, /* timing control */	
	{0x3825, 0x00}, /* timing control */
	{0x3826, 0x00}, /* timing control */
	{0x3827, 0x00}, /* timing control */
	{0x382a, 0x04}, /* timing control */


	/* STROBE control */
	{0x3a04, 0x06}, /* Reserved */
	{0x3a05, 0x14}, /* Reserved */
	{0x3a06, 0x00}, /* Reserved */
	{0x3a07, 0xfe}, /* Reserved */
	{0x3b00, 0x00}, /* STROBE mode/polarity/request/xenon width */
	{0x3b02, 0x00}, /* {3b02,3b03} dummy line in LED1/LED2 */
	{0x3b03, 0x00}, /* {3b02,3b03} dummy line in LED1/LED2 */
	{0x3b04, 0x00}, /* Strobe debug/delay */
	{0x3b05, 0x00}, /* strobe pulse width control */


	/* OTP control */
	{0x3d00, 0x00}, /* OTP control */
	{0x3d01, 0x00}, /* OTP control */
	{0x3d02, 0x00}, /* OTP control */
	{0x3d03, 0x00}, /* OTP control */
	{0x3d04, 0x00}, /* OTP control */
	{0x3d05, 0x00}, /* OTP control */
	{0x3d06, 0x00}, /* OTP control */
	{0x3d07, 0x00}, /* OTP control */
	{0x3d08, 0x00}, /* OTP control */
	{0x3d09, 0x00}, /* OTP control */
	{0x3d0a, 0x00}, /* OTP control */
	{0x3d0b, 0x00}, /* OTP control */
	{0x3d0c, 0x00}, /* OTP control */
	{0x3d0d, 0x00}, /* OTP control */
	{0x3d0e, 0x00}, /* OTP control */
	{0x3d0f, 0x00}, /* OTP control */
	{0x3d80, 0x00}, /* OTP control */
	{0x3d81, 0x00}, /* OTP control */
	{0x3d84, 0x00}, /* OTP control */
	{0x3e07, 0x20}, /* OTP control */


	/* BLC ocntrol */
	{0x4000, 0x08}, /* BLC control */
	{0x4001, 0x04}, /* BLC start line */
	{0x4002, 0x45}, /* BLC control */
	{0x4004, 0x08}, /* BLC line select */
	{0x4005, 0x18}, /* BLC control */
	{0x4006, 0x20}, /* BLC debug */
	{0x4008, 0x24}, /* BLC control */
	{0x4009, 0x10}, /* BLC target */
	{0x400c, 0x00},
	{0x400d, 0x00},
	{0x4058, 0x00},

	/*          */
	{0x4101, 0xb2}, /* analog reference */
	{0x4303, 0x00}, /* analog reference */
	{0x4304, 0x08}, /* analog reference */
	{0x4307, 0x30}, /* embedded line control */
	{0x4311, 0x04},
	{0x4315, 0x01}, /* VSYNC delay control */

	{0x4511, 0x05}, /* timing delay */
	{0x4512, 0x01}, /* timing delay */


	/* MIPI control */
	{0x4806, 0x00}, /* MIPI control */
	{0x4816, 0x52},
	{0x481f, 0x30},
	{0x4826, 0x2c}, /* MIPI control */

	{0x4831, 0x64}, /* ui_hs_prepare */

	{0x4d00, 0x04}, /* Temperature monitor */
	{0x4d01, 0x71}, /* Temperature monitor */
	{0x4d02, 0xfd}, /* Temperature monitor */
	{0x4d03, 0xf5}, /* Temperature monitor */
	{0x4d04, 0x0c}, /* Temperature monitor */
	{0x4d05, 0xcc}, /* Temperature monitor */
	{0x4837, 0x0a}, /* pperiod of PCLK2x */

	/* ISP control */
	{0x5000, 0x06}, /* LENC/DPC control */
	{0x5001, 0x01}, /* Manual WB enable */
	{0x5002, 0x00}, /* scale enable */
	{0x5003, 0x20}, /* DPC control */
	{0x5046, 0x0a},
	{0x5013, 0x00}, /* ISP LSB enable */
	{0x5046, 0x0a},
	{0x5780, 0x1c}, /* DPC control */
	{0x5786, 0x20}, /* DPC control */
	{0x5787, 0x10}, /* DPC control */
	{0x5788, 0x18}, /* DPC control */
	{0x578a, 0x04}, /* DPC control */
	{0x578b, 0x02}, /* DPC control */
	{0x578c, 0x02}, /* DPC control */
	{0x578e, 0x06}, /* DPC control */
	{0x578f, 0x02}, /* DPC control */
	{0x5790, 0x02}, /* DPC control */
	{0x5791, 0xff}, /* DPC control */

	/* LENC Factor Contorl */
	{0x5842, 0x01}, /* Lenc B/R H Scale Factor */
	{0x5843, 0x2b}, /* Lenc B/R H Scale Factor */
	{0x5844, 0x01}, /* Lenc B/R V Scale Factor */
	{0x5845, 0x92}, /* Lenc B/R V Scale Factor */
	{0x5846, 0x01}, /* Lenc G H Scale Factor */
	{0x5847, 0x8f}, /* Lenc G H Scale Factor */
	{0x5848, 0x01}, /* Lenc G V Scale Factor */
	{0x5849, 0x0c}, /* Lenc G V Scale Factor */

#if 1
	{0x5e00, 0x00}, /* color bar control */
#else
	{0x5e00, 0x80}, /* color bar control */  /* test : color bar */
#endif

	{0x5e10, 0x0c}, /* Mirror/flip auto color control */
	//{0x000c, 0x02}, /* for OV FPGA board only */
	//{0x000c, 0x00}, /* for OV FPGA board only */


//test : 15 FPS
#if 0
	{0x0102, 0x01},
	{0x0100, 0x00}, 
	{0x309A, 0x05},
	{0x309B, 0x01},
	{0x30B6, 0x02},
	{0x0102, 0x01},
	{0x0100, 0x00}, 
#endif

	/* software wakeup */
//	{0x0100, 0x01}, /* streaming enable */

};

static struct v4l2_subdev_info ov5693_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov5693_init_conf[] = {
	{&ov5693_recommend_settings[0],
	ARRAY_SIZE(ov5693_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov5693_confs[] = {
	{&ov5693_snap_settings[0],
	ARRAY_SIZE(ov5693_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_prev_settings[0],
	ARRAY_SIZE(ov5693_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_video_settings[0],
	ARRAY_SIZE(ov5693_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_fast_video_16_9_settings[0],
	ARRAY_SIZE(ov5693_fast_video_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_prev_16_9_settings[0],
	ARRAY_SIZE(ov5693_prev_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_prev_4_3_settings[0],
	ARRAY_SIZE(ov5693_prev_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_fast_video_5_3_settings[0],
	ARRAY_SIZE(ov5693_fast_video_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_prev_5_3_settings[0],
	ARRAY_SIZE(ov5693_prev_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_zoe_settings[0],
	ARRAY_SIZE(ov5693_zoe_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov5693_dimensions[] = {
	{
		/* Snapshot - full size */
		.x_output = 0xA20,	/* 0x0a20 : 2592 */
		.y_output = 0x7A0,	/* 0x07A0 : 1952 */
		.line_length_pclk = 0xade, //0xA84, 	/* 2688 */
		.frame_length_lines = 0x7c8, //0x7C8,	/* 1992 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		/* HTC_START steven 20130701 ov5693 HD 60fps sensor setting */
		/*Q size 720P 60fps*/
		.x_output = 0x500,	/* 1280 */
		.y_output = 0x2D0,	/* 720 */
		.line_length_pclk = 0x5BE, //0x6D8,		/* 1752 */
		.frame_length_lines = 0x74e, //0x308,	/* 776 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_addr_end = 0x9FF,
		.y_addr_end = 0x59F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
		/* HTC_END steven 20130701 ov5693 HD 60fps sensor setting */
	},
	{
		/* Video - full size */
		.x_output = 0xA20,	/* 0x0a20 : 2592 */
		.y_output = 0x7A0,	/* 0x07A0 : 1952 */
		.line_length_pclk = 0xade, //0xA84,		/* 2688 */
		.frame_length_lines = 0x7c8, //0x7C8,	/* 1992 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		/* Fast Video 1280 x 720 */
		.x_output = 0x500,	/* 1280 */
		.y_output = 0x2D0,	/* 720 */
		.line_length_pclk = 0x5BE, //0x6D8,		/* 1752 */
		.frame_length_lines = 0x3A7, //0x308,	/* 776 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_addr_end = 0x9FF,
		.y_addr_end = 0x59F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		/* Preview - full size (16:9) */
		.x_output = 0xA20,	/* 0x0a20 : 2592 */
		.y_output = 0x5B0,  /* 0x05B0 : 1456 */		//0x7A0,	/* 0x07A0 : 1952 */
		.line_length_pclk = 0xb4c,		/* 2688 */ //0xA84
		.frame_length_lines = 0x755,	/* 1877 */ //0x7C8
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x5AF,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		/* 4:3 - full size */
		.x_output = 0xA20,	/* 0x0a20 : 2592 */
		.y_output = 0x7A0,	/* 0x07A0 : 1952 */
		.line_length_pclk = 0xade, //0xA84, 	/* 2688 */
		.frame_length_lines = 0x7c8, //0x7C8,	/* 1992 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		/* Fast Video 1200 x 720 */
		.x_output = 0x4b0,	/* 1200 */
		.y_output = 0x2d0,	/* 720 */
		.line_length_pclk = 0x5BE,		/* 1672 */
		.frame_length_lines = 0x3A7,	/* 760 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_addr_end = 0x95F,
		.y_addr_end = 0x59F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		/* Preview - full size (5:3) */
		.x_output = 0xA20,	/* 0x0A20 : 2592 */
		.y_output = 0x610,	/* 0x0610 : 1552 */
		.line_length_pclk = 0xade,		/* 2692 */
		.frame_length_lines = 0x760,	/* 1980 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x60F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		/* zoe - full size */
		.x_output = 0xA20,	/* 0x0a20 : 2592 */
		.y_output = 0x7A0,	/* 0x07A0 : 1952 */
		.line_length_pclk = 0xade, //0xA84, 	/* 2688 */
		.frame_length_lines = 0x7c8, //0x7C8,	/* 1992 */
		.vt_pixel_clk = 166400000,
		.op_pixel_clk = 166400000,
		.binning_factor = 1,
		/* Rawchip */
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

static struct msm_camera_csid_vc_cfg ov5693_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov5693_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ov5693_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *ov5693_csi_params_array[] = {
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov5693_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380a,
	.line_length_pclk = 0x380c,
	.frame_length_lines = 0x380e,
};

static struct msm_sensor_id_info_t ov5693_id_info = {
	.sensor_id_reg_addr = 0x300A,
//	.sensor_id = 0x5693,

	.sensor_id = 0x5690,
////	.sensor_id = 0x5681,
};
#define SENSOR_REGISTER_MAX_LINECOUNT 0x7fff
#define SENSOR_VERT_OFFSET 4

static struct msm_sensor_exp_gain_info_t ov5693_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,//0x3501,
	.global_gain_addr = 0x350A,
	.vert_offset = SENSOR_VERT_OFFSET,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
	.sensor_max_linecount = SENSOR_REGISTER_MAX_LINECOUNT-SENSOR_VERT_OFFSET, /* VTS: 15bit: 2^15 - (vert_offset) */
};

// HTC_START
#define MAX_FUSE_ID_INFO 17
unsigned short fuse_id[MAX_FUSE_ID_INFO] = {0};
unsigned short OTP[10] = {0};
static int fuseid_read_once = 0;

static int ov5693_read_fuseid_once(void)
{
	struct msm_camera_i2c_client *ov5693_msm_camera_i2c_client = ov5693_s_ctrl.sensor_i2c_client;
	int i;
	unsigned short bank_addr;
	int count = 0, dirty = 0;

	/* Get OTP fuse id for OV sensor */
	msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x0100, 0x01);

	for (i = 0; i < 5; i++) {
		bank_addr = 0xC0 + (0x14 - i); /* Read from Bank 20(0x14) to Bank 16(0x10) */
		pr_info("ov5693_read_fuseid_once  bank_addr=0x%x\n", bank_addr);

		msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x3d84, bank_addr);
		msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x3d81, 0x01);
		msleep(3);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d00, &fuse_id[0]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d01, &fuse_id[1]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d02, &fuse_id[2]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d03, &fuse_id[3]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d04, &fuse_id[4]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d05, &fuse_id[5]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d06, &fuse_id[6]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d07, &fuse_id[7]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d08, &fuse_id[8]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d09, &fuse_id[9]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d0A, &fuse_id[10]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d0B, &fuse_id[11]);

		for (count = 0; count < MAX_FUSE_ID_INFO; count++) {
			if (fuse_id[count] != 0) {
				dirty = 1;
				break;
			}
		}

		if (dirty) {
			OTP[0] = fuse_id[0]; /* Module Vendor */
			OTP[1] = fuse_id[1]; /* Lens */
			OTP[2] = fuse_id[4]; /* Module Fuse ID A */
			OTP[3] = fuse_id[5]; /* Module Fuse ID B */
			OTP[4] = fuse_id[6]; /* Module Fuse ID C */
			OTP[5] = fuse_id[7]; /* Module Fuse ID D */
			OTP[6] = fuse_id[8]; /* Module Fuse ID E */
			OTP[7] = fuse_id[9]; /* Module Fuse ID F */
			fuseid_read_once = 1;
			break;
		}
	}

	msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x0100, 0x00);

	return 0;
}

static int ov5693_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("ov5693_read_fuseid\n");

	if (fuseid_read_once == 0)
		ov5693_read_fuseid_once();

	if (fuseid_read_once) {
		cdata->cfg.fuse.fuse_id_word1 = (uint32_t) (OTP[0]<<8) | (OTP[1]);
		cdata->cfg.fuse.fuse_id_word2 = (uint32_t) (OTP[2]<<8) | (OTP[3]);
		cdata->cfg.fuse.fuse_id_word3 = (uint32_t) (OTP[4]<<8) | (OTP[5]);
		cdata->cfg.fuse.fuse_id_word4 = (uint32_t) (OTP[6]<<8) | (OTP[7]);
	}

	pr_info("ov5693: fuse->fuse_id_word1:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("ov5693: fuse->fuse_id_word2:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("ov5693: fuse->fuse_id_word3:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("ov5693: fuse->fuse_id_word4:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;
}
// HTC_END


static int ov5693_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	pr_info(" %s\n", __func__);

// HTC_START
	if (fuseid_read_once == 0)
		ov5693_read_fuseid_once();
// HTC_END

	return rc;
}

static const char *ov5693Vendor = "Omnivision";
static const char *ov5693NAME = "ov5693";
static const char *ov5693Size = "5M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", ov5693Vendor, ov5693NAME, ov5693Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ov5693;

static int ov5693_sysfs_init(void)
{
	int ret ;
	pr_info("ov5693:kobject creat and add\n");
	android_ov5693 = kobject_create_and_add("android_camera", NULL);
	if (android_ov5693 == NULL) {
		pr_info("ov5693_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ov5693:sysfs_create_file\n");
	ret = sysfs_create_file(android_ov5693, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ov5693_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ov5693);
	}

	return 0 ;
}

static struct msm_camera_i2c_client ov5693_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


int32_t ov5693_power_up(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	struct sensor_cfg_data cdata;  //CC140211
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info(" %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err(" %s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_err(" %s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err(" %s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_set_power_up(s_ctrl);//(sdata);
	if (rc < 0) {
		pr_err(" %s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	ov5693_sensor_open_init(sdata);
//CC140211
	if (s_ctrl->func_tbl->sensor_i2c_read_fuseid == NULL) {
		rc = -EFAULT;
		return rc;
	}
	rc = s_ctrl->func_tbl->sensor_i2c_read_fuseid(&cdata, s_ctrl);
	if (rc < 0) {
		return rc;
	}
//CC140211~
	pr_info(" %s end\n", __func__);

	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_err("sensor platform_data didnt register\n");
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;//msm_sensor_power_up(sdata);
}

int32_t ov5693_power_down(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
{
	int rc = 0;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info(" %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err(" %s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err(" %s: sensor platform_data didn't register\n", __func__);
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err(" %s failed to disable power\n", __func__);
	}
	rc = msm_sensor_set_power_down(s_ctrl);//(sdata);
	if (rc < 0)
		pr_err("%s msm_sensor_power_down failed\n", __func__);

	if (!sdata->use_rawchip) {
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err(" %s: msm_camio_clk_disable failed:%d\n",
				 __func__, rc);
	}

	return rc;//msm_sensor_power_down(sdata);
}


int32_t ov5693_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info(" %s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ov5693_sysfs_init();
	pr_info(" %s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ov5693_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov5693_s_ctrl},
	{ }
};

static struct i2c_driver ov5693_i2c_driver = {
	.id_table = ov5693_i2c_id,
	.probe  = ov5693_i2c_probe,//msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&ov5693_i2c_driver);
}


static struct v4l2_subdev_core_ops ov5693_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov5693_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov5693_subdev_ops = {
	.core = &ov5693_subdev_core_ops,
	.video  = &ov5693_subdev_video_ops,
};


int32_t ov5693_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) /* HTC Angie 20111019 - Fix FPS */
{
	uint32_t fl_lines;
	uint8_t offset;
	uint8_t line_p0,line_p1,line_p2;
	uint8_t fl_lines_h,fl_lines_l;
	uint8_t gain_h,gain_l;


/* HTC_START Angie 20111019 - Fix FPS */
	uint32_t fps_divider = Q10;


	CDBG(" ov5693_write_exp_gain1_ex line=0x%x gain=0x%x", line, gain);
	CDBG(" ov5693_write_exp_gain1_ex line=%d gain=%d", line, gain);

	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

/* HTC_START ben 20120229 */
	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;
/* HTC_END */

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;
/* HTC_END */

	fl_lines_h = (uint8_t)(fl_lines/256);
	fl_lines_l = (uint8_t)(fl_lines%256);
	gain_h = (uint8_t)(gain/256);
	gain_l = (uint8_t)(gain%256);

	line_p0 = (uint8_t)(line/(256*16));
	line_p1 = (uint8_t)((line%(256*16))/16);
	line_p2 = (uint8_t)((line%16)*16);

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines_h,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines+1, fl_lines_l,
		MSM_CAMERA_I2C_BYTE_DATA);


	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line_p0,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, line_p1,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+2, line_p2,
		MSM_CAMERA_I2C_BYTE_DATA);


	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain_h,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr+1, gain_l,
		MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t ov5693_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res) {

	int rc = 0;
	int rc1 = 0, rc2 = 0;
	unsigned short f_value = 0;	// mirror setting
	unsigned short m_value = 0;	// flip setting
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info(" %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err(" %s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	ov5693_s_ctrl.mirror_flip = CAMERA_SENSOR_NONE;

//CC140207
//	rc = msm_sensor_setting(s_ctrl, update_type, res);
	rc = msm_sensor_setting_parallel_ov(s_ctrl, update_type, res);
//CC140207~
	if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		/* Apply sensor mirror/flip */
		rc1 = msm_camera_i2c_read_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_FLIP, &f_value);
		if (rc1 < 0)
			pr_info("%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_FLIP);
		rc2 = msm_camera_i2c_read_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_MIRROR, &m_value);
		if (rc2 < 0)
			pr_info("%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_MIRROR);

		pr_info(" f_value = 0x%x m_value = 0x%x", f_value, m_value);

		/* Flip on 0x42, off 0x40; Mirror on 0x1E, off 0x18*/
		if (sdata->sensor_platform_info->mirror_flip == CAMERA_SENSOR_MIRROR_FLIP) {
			pr_info(" CAMERA_SENSOR_MIRROR_FLIP");
			f_value |= 0x42;
			m_value |= 0x1E;
		} else if (sdata->sensor_platform_info->mirror_flip == CAMERA_SENSOR_MIRROR) {
			pr_info(" CAMERA_SENSOR_MIRROR");
			f_value &= 0xFD;
			m_value |= 0x1E;
		} else if (sdata->sensor_platform_info->mirror_flip == CAMERA_SENSOR_FLIP) {
			pr_info(" CAMERA_SENSOR_FLIP");
			f_value |= 0x42;
			m_value &= 0xF9;
		} else {
			pr_info(" CAMERA_SENSOR_NONE");
			f_value &= 0xFD;
			m_value &= 0xF9;
		}

		pr_info(" New f_value = 0x%x m_value = 0x%x", f_value, m_value);

		rc1 = msm_camera_i2c_write_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_FLIP, f_value);
		if (rc1 < 0)
			pr_info("%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_FLIP);

		rc2 = msm_camera_i2c_write_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_MIRROR, m_value);
		if (rc2 < 0)
			pr_info("%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_MIRROR);
	}

	return rc;
}


static struct msm_sensor_fn_t ov5693_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_exp_gain_ex = ov5693_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain_ex = ov5693_write_exp_gain1_ex,
	.sensor_setting = ov5693_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov5693_power_up,
	.sensor_power_down = ov5693_power_down,
	.sensor_config = msm_sensor_config,
	.sensor_i2c_read_fuseid = ov5693_read_fuseid,	//HTC
};

static struct msm_sensor_reg_t ov5693_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov5693_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov5693_start_settings),
	.stop_stream_conf = ov5693_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov5693_stop_settings),
	.group_hold_on_conf = ov5693_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov5693_groupon_settings),
	.group_hold_off_conf = ov5693_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov5693_groupoff_settings),
	.init_settings = &ov5693_init_conf[0],
	.init_size = ARRAY_SIZE(ov5693_init_conf),
	.mode_settings = &ov5693_confs[0],
	.output_settings = &ov5693_dimensions[0],
	.num_conf = ARRAY_SIZE(ov5693_confs),
};

static struct msm_sensor_ctrl_t ov5693_s_ctrl = {
	.msm_sensor_reg = &ov5693_regs,
	.sensor_i2c_client = &ov5693_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_output_reg_addr = &ov5693_reg_addr,
	.sensor_id_info = &ov5693_id_info,
	.sensor_exp_gain_info = &ov5693_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov5693_csi_params_array[0],
	.msm_sensor_mutex = &ov5693_mut,
	.sensor_i2c_driver = &ov5693_i2c_driver,
	.sensor_v4l2_subdev_info = ov5693_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov5693_subdev_info),
	.sensor_v4l2_subdev_ops = &ov5693_subdev_ops,
	.func_tbl = &ov5693_func_tbl,
	.sensor_first_mutex = &ov5693_sensor_init_mut,  //CC140207
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision 5 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

