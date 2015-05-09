#include "msm_sensor.h"
#define SENSOR_NAME "ar0260"
#define PLATFORM_DRIVER_NAME "msm_camera_ar0260"
#define ar0260_obj ar0260_##obj

#ifdef CONFIG_RAWCHIPII
#include "yushanII.h"
#include "ilp0100_ST_api.h"
#include "ilp0100_customer_sensor_config.h"
#endif

#define USE_MCLK_780MHZ 1

DEFINE_MUTEX(ar0260_mut);
DEFINE_MUTEX(ar0260_sensor_init_mut); //CC120826,
static struct msm_sensor_ctrl_t ar0260_s_ctrl;

static struct msm_camera_i2c_reg_conf ar0260_start_settings[] = {
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

//#ifdef USE_MCLK_780MHZ
#if 0
	{0xffff,50},
	{0x3C40, 0xAC36,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
	{0x301C, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

	{0xffff,50},
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
#endif
};

static struct msm_camera_i2c_reg_conf ar0260_start_settings_yushanii[] = {
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_stop_settings[] = {
	{0x301C, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_stop_settings_yushanii[] = {
	{0x301C, 0x0002,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x3C40, 0xAC36,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_groupon_settings[] = {
	{0x3022, 0x0100,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_groupoff_settings[] = {
	{0x3022, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_prev_settings[] = {

};

static struct msm_camera_i2c_reg_conf ar0260_snap_settings[] = {

};


#ifdef USE_MCLK_780MHZ

//POLL_{ 0x0018, 0x4000, ==0, DELAY=10, TIMEOUT=100; pg while (0x18==0) ;
//POLL_{ 0x001C, 0xFF00, !=6, DELAY=10, TIMEOUT=100
//POLL_{ 0x001C, 0xFF00, <0x3C, DELAY=10, TIMEOUT=100
//POLL_{ 0x4334, 0xFFFF, !=0, DELAY=1, TIMEOUT=100
//POLL_{ 0x0014, 0x8000, ==0, DELAY=10, TIMEOUT=100


static struct msm_camera_i2c_reg_conf ar0260_recommend_settings[] = {

// checking whether sensor is in FW standby mode
// 
//POLL_{ 0x0018, 0x4000, ==0, DELAY=10, TIMEOUT=100; pg while (0x18==0)
{0x0018, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x4000},

// load FW patch
{ 0x001A, 0x0005}, 	// RESET_AND_MISC_CONTROL
{ 0x001C, 0x000C}, 	// MCU_BOOT_MODE
{ 0x001A, 0x0014}, 	// RESET_AND_MISC_CONTROL
// polling MCU status
//POLL_{ 0x001C, 0xFF00, !=6, DELAY=10, TIMEOUT=100
{0x001C, 6<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFF00},
//{0xffff,50},


// Load FW Patch 11ff
{ 0x0982, 0x0001}, 	// ACCESS_CTL_STAT
{ 0x098A, 0x6000}, 	// PHYSICAL_ADDRESS_ACCESS
{ 0xE000, 0xC0F1},
{ 0xE002, 0x0C72},
{ 0xE004, 0x0760},
{ 0xE006, 0xD81D},
{ 0xE008, 0x0ABA},
{ 0xE00A, 0x0780},
{ 0xE00C, 0xE080},
{ 0xE00E, 0x0064},
{ 0xE010, 0x0001},
{ 0xE012, 0x0916},
{ 0xE014, 0x0860},
{ 0xE016, 0xD802},
{ 0xE018, 0xD900},
{ 0xE01A, 0x70CF},
{ 0xE01C, 0xFF00},
{ 0xE01E, 0x31B0},
{ 0xE020, 0xB038},
{ 0xE022, 0x1CFC},
{ 0xE024, 0xB388},
{ 0xE026, 0x76CF},
{ 0xE028, 0xFF00},
{ 0xE02A, 0x33CC},
{ 0xE02C, 0x200A},
{ 0xE02E, 0x0F80},
{ 0xE030, 0xFFFF},
{ 0xE032, 0xE048},
{ 0xE034, 0x1CFC},
{ 0xE036, 0xB008},
{ 0xE038, 0x2022},
{ 0xE03A, 0x0F80},
{ 0xE03C, 0x0000},
{ 0xE03E, 0xFC3C},
{ 0xE040, 0x2020},
{ 0xE042, 0x0F80},
{ 0xE044, 0x0000},
{ 0xE046, 0xEA34},
{ 0xE048, 0x1404},
{ 0xE04A, 0x340E},
{ 0xE04C, 0xD801},
{ 0xE04E, 0x71CF},
{ 0xE050, 0xFF00},
{ 0xE052, 0x33CC},
{ 0xE054, 0x190A},
{ 0xE056, 0x801C},
{ 0xE058, 0x208A},
{ 0xE05A, 0x0004},
{ 0xE05C, 0x1964},
{ 0xE05E, 0x8004},
{ 0xE060, 0x0C12},
{ 0xE062, 0x0760},
{ 0xE064, 0xD83C},
{ 0xE066, 0x0E5E},
{ 0xE068, 0x0880},
{ 0xE06A, 0x216F},
{ 0xE06C, 0x003F},
{ 0xE06E, 0xF1FD},
{ 0xE070, 0x0C02},
{ 0xE072, 0x0760},
{ 0xE074, 0xD81E},
{ 0xE076, 0xC0D1},
{ 0xE078, 0x7EE0},
{ 0xE07A, 0x78E0},
{ 0xE07C, 0xC0F1},
{ 0xE07E, 0xE889},
{ 0xE080, 0x70CF},
{ 0xE082, 0xFF00},
{ 0xE084, 0x0000},
{ 0xE086, 0x900E},
{ 0xE088, 0xB8E7},
{ 0xE08A, 0x0F78},
{ 0xE08C, 0xFFC1},
{ 0xE08E, 0xD800},
{ 0xE090, 0xF1F3},

// Overwrite Fixtable 10ff
{ 0x098E, 0x0000}, 	// LOGICAL_ADDRESS_ACCESS
{ 0x098A, 0x5F38}, 	// PHYSICAL_ADDRESS_ACCESS
{ 0x0990, 0xFFFF},
{ 0x0992, 0xE07C},

// Release Processor from Pause
{ 0x001C, 0x0600}, 	// MCU_BOOT_MODE

// check MCU code
//POLL_{ 0x001C, 0xFF00, <0x3C, DELAY=10, TIMEOUT=100
	{0x001C, 0x3C<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_LESS,0xFF00},

	//{0xffff,50},

// Sensor recommended -- 110211
{ 0x3E00, 0x042D}, 	// DYNAMIC_SEQRAM_00
{ 0x3E02, 0x39FF}, 	// DYNAMIC_SEQRAM_02
{ 0x3E04, 0x49FF}, 	// DYNAMIC_SEQRAM_04
{ 0x3E06, 0xFFFF}, 	// DYNAMIC_SEQRAM_06
{ 0x3E08, 0x8071}, 	// DYNAMIC_SEQRAM_08
{ 0x3E0A, 0x7211}, 	// DYNAMIC_SEQRAM_0A
{ 0x3E0C, 0xE040}, 	// DYNAMIC_SEQRAM_0C
{ 0x3E0E, 0xA840}, 	// DYNAMIC_SEQRAM_0E
{ 0x3E10, 0x4100}, 	// DYNAMIC_SEQRAM_10
{ 0x3E12, 0x1846}, 	// DYNAMIC_SEQRAM_12
{ 0x3E14, 0xA547}, 	// DYNAMIC_SEQRAM_14
{ 0x3E16, 0xAD57}, 	// DYNAMIC_SEQRAM_16
{ 0x3E18, 0x8149}, 	// DYNAMIC_SEQRAM_18
{ 0x3E1A, 0x9D49}, 	// DYNAMIC_SEQRAM_1A
{ 0x3E1C, 0x9F46}, 	// DYNAMIC_SEQRAM_1C
{ 0x3E1E, 0x8000}, 	// DYNAMIC_SEQRAM_1E
{ 0x3E20, 0x1842}, 	// DYNAMIC_SEQRAM_20
{ 0x3E22, 0x4180}, 	// DYNAMIC_SEQRAM_22
{ 0x3E24, 0x0018}, 	// DYNAMIC_SEQRAM_24
{ 0x3E26, 0x8149}, 	// DYNAMIC_SEQRAM_26
{ 0x3E28, 0x9C49}, 	// DYNAMIC_SEQRAM_28
{ 0x3E2A, 0x9347}, 	// DYNAMIC_SEQRAM_2A
{ 0x3E2C, 0x804D}, 	// DYNAMIC_SEQRAM_2C
{ 0x3E2E, 0x804A}, 	// DYNAMIC_SEQRAM_2E
{ 0x3E30, 0x100C}, 	// DYNAMIC_SEQRAM_30
{ 0x3E32, 0x8000}, 	// DYNAMIC_SEQRAM_32
{ 0x3E34, 0x1841}, 	// DYNAMIC_SEQRAM_34
{ 0x3E36, 0x4280}, 	// DYNAMIC_SEQRAM_36
{ 0x3E38, 0x0018}, 	// DYNAMIC_SEQRAM_38
{ 0x3E3A, 0x9710}, 	// DYNAMIC_SEQRAM_3A
{ 0x3E3C, 0x0C80}, 	// DYNAMIC_SEQRAM_3C
{ 0x3E3E, 0x4DA2}, 	// DYNAMIC_SEQRAM_3E
{ 0x3E40, 0x4BA0}, 	// DYNAMIC_SEQRAM_40
{ 0x3E42, 0x4A00}, 	// DYNAMIC_SEQRAM_42
{ 0x3E44, 0x1880}, 	// DYNAMIC_SEQRAM_44
{ 0x3E46, 0x4241}, 	// DYNAMIC_SEQRAM_46
{ 0x3E48, 0x0018}, 	// DYNAMIC_SEQRAM_48
{ 0x3E4A, 0xB54B}, 	// DYNAMIC_SEQRAM_4A
{ 0x3E4C, 0x1C00}, 	// DYNAMIC_SEQRAM_4C
{ 0x3E4E, 0x8000}, 	// DYNAMIC_SEQRAM_4E
{ 0x3E50, 0x1C10}, 	// DYNAMIC_SEQRAM_50
{ 0x3E52, 0x6081}, 	// DYNAMIC_SEQRAM_52
{ 0x3E54, 0x1580}, 	// DYNAMIC_SEQRAM_54
{ 0x3E56, 0x7C09}, 	// DYNAMIC_SEQRAM_56
{ 0x3E58, 0x7000}, 	// DYNAMIC_SEQRAM_58
{ 0x3E5A, 0x8082}, 	// DYNAMIC_SEQRAM_5A
{ 0x3E5C, 0x7281}, 	// DYNAMIC_SEQRAM_5C
{ 0x3E5E, 0x4C40}, 	// DYNAMIC_SEQRAM_5E
{ 0x3E60, 0x8E4D}, 	// DYNAMIC_SEQRAM_60
{ 0x3E62, 0x8110}, 	// DYNAMIC_SEQRAM_62
{ 0x3E64, 0x0CAF}, 	// DYNAMIC_SEQRAM_64
{ 0x3E66, 0x4D80}, 	// DYNAMIC_SEQRAM_66
{ 0x3E68, 0x100C}, 	// DYNAMIC_SEQRAM_68
{ 0x3E6A, 0x8440}, 	// DYNAMIC_SEQRAM_6A
{ 0x3E6C, 0x4C81}, 	// DYNAMIC_SEQRAM_6C
{ 0x3E6E, 0x7C5B}, 	// DYNAMIC_SEQRAM_6E
{ 0x3E70, 0x7000}, 	// DYNAMIC_SEQRAM_70
{ 0x3E72, 0x8054}, 	// DYNAMIC_SEQRAM_72
{ 0x3E74, 0x924C}, 	// DYNAMIC_SEQRAM_74
{ 0x3E76, 0x4078}, 	// DYNAMIC_SEQRAM_76
{ 0x3E78, 0x4D4F}, 	// DYNAMIC_SEQRAM_78
{ 0x3E7A, 0x4E98}, 	// DYNAMIC_SEQRAM_7A
{ 0x3E7C, 0x504E}, 	// DYNAMIC_SEQRAM_7C
{ 0x3E7E, 0x4F97}, 	// DYNAMIC_SEQRAM_7E
{ 0x3E80, 0x4F4E}, 	// DYNAMIC_SEQRAM_80
{ 0x3E82, 0x507C}, 	// DYNAMIC_SEQRAM_82
{ 0x3E84, 0x7B8D}, 	// DYNAMIC_SEQRAM_84
{ 0x3E86, 0x4D88}, 	// DYNAMIC_SEQRAM_86
{ 0x3E88, 0x4E10}, 	// DYNAMIC_SEQRAM_88
{ 0x3E8A, 0x0940}, 	// DYNAMIC_SEQRAM_8A
{ 0x3E8C, 0x8879}, 	// DYNAMIC_SEQRAM_8C
{ 0x3E8E, 0x5481}, 	// DYNAMIC_SEQRAM_8E
{ 0x3E90, 0x7000}, 	// DYNAMIC_SEQRAM_90
{ 0x3E92, 0x8082}, 	// DYNAMIC_SEQRAM_92
{ 0x3E94, 0x7281}, 	// DYNAMIC_SEQRAM_94
{ 0x3E96, 0x4C40}, 	// DYNAMIC_SEQRAM_96
{ 0x3E98, 0x8E4D}, 	// DYNAMIC_SEQRAM_98
{ 0x3E9A, 0x8110}, 	// DYNAMIC_SEQRAM_9A
{ 0x3E9C, 0x0CAF}, 	// DYNAMIC_SEQRAM_9C
{ 0x3E9E, 0x4D80}, 	// DYNAMIC_SEQRAM_9E
{ 0x3EA0, 0x100C}, 	// DYNAMIC_SEQRAM_A0
{ 0x3EA2, 0x8440}, 	// DYNAMIC_SEQRAM_A2
{ 0x3EA4, 0x4C81}, 	// DYNAMIC_SEQRAM_A4
{ 0x3EA6, 0x7C93}, 	// DYNAMIC_SEQRAM_A6
{ 0x3EA8, 0x7000}, 	// DYNAMIC_SEQRAM_A8
{ 0x3EAA, 0x0000}, 	// DYNAMIC_SEQRAM_AA
{ 0x3EAC, 0x0000}, 	// DYNAMIC_SEQRAM_AC
{ 0x3EAE, 0x0000}, 	// DYNAMIC_SEQRAM_AE
{ 0x3EB0, 0x0000}, 	// DYNAMIC_SEQRAM_B0
{ 0x3EB2, 0x0000}, 	// DYNAMIC_SEQRAM_B2
{ 0x3EB4, 0x0000}, 	// DYNAMIC_SEQRAM_B4
{ 0x3EB6, 0x0000}, 	// DYNAMIC_SEQRAM_B6
{ 0x3EB8, 0x0000}, 	// DYNAMIC_SEQRAM_B8
{ 0x3EBA, 0x0000}, 	// DYNAMIC_SEQRAM_BA
{ 0x3EBC, 0x0000}, 	// DYNAMIC_SEQRAM_BC
{ 0x3EBE, 0x0000}, 	// DYNAMIC_SEQRAM_BE
{ 0x3EC0, 0x0000}, 	// DYNAMIC_SEQRAM_C0
{ 0x3EC2, 0x0000}, 	// DYNAMIC_SEQRAM_C2
{ 0x3EC4, 0x0000}, 	// DYNAMIC_SEQRAM_C4
{ 0x3EC6, 0x0000}, 	// DYNAMIC_SEQRAM_C6
{ 0x3EC8, 0x0000}, 	// DYNAMIC_SEQRAM_C8
{ 0x3ECA, 0x0000}, 	// DYNAMIC_SEQRAM_CA

// Char settings -- 110211
{ 0x30B2, 0xC000}, 	// CALIB_TIED_OFFSET
{ 0x30D4, 0x9400}, 	// COLUMN_CORRECTION
{ 0x31C0, 0x0000}, 	// FUSE_REF_ADDR
{ 0x316A, 0x8200}, 	// DAC_FBIAS
{ 0x316C, 0x8200}, 	// DAC_TXLO
{ 0x3EFE, 0x2808}, 	// DAC_LD_TXLO
{ 0x3EFC, 0x2868}, 	// DAC_LD_FBIAS
{ 0x3ED2, 0xD165}, 	// DAC_LD_6_7
{ 0x3EF2, 0xD165}, 	// DAC_LP_6_7
{ 0x3ED8, 0x7F1A}, 	// DAC_LD_12_13
{ 0x3EDA, 0x2828}, 	// DAC_LD_14_15
{ 0x3EE2, 0x0058}, 	// DAC_LD_22_23

{ 0x002C, 0x000F}, 	// PLL_P7_DIVIDER

// CPIPE_Calibration
// N FW DC
{ 0x3382, 0x012C}, 	// DM_CDC_HI_THR_COMB
{ 0x3384, 0x0158}, 	// DM_CDC_HI_THR_SATURATION
{ 0x3386, 0x015C}, 	// DM_CDC_LO_THR_COMB
{ 0x3388, 0x00E6}, 	// DM_CDC_LO_THR_SATURATION

//
{ 0x338A, 0x000F}, 	// DM_DC_SINGLE_THRESHOLD

// CPIPE_Preference
// power save
{ 0x3276, 0x03FF}, 	// BLACK_LEVEL_TO_CCM
{ 0x32B2, 0x0000}, 	// DKDELTA_CCM_CTL
{ 0x3210, 0x0000}, 	// COLOR_PIPELINE_CONTROL
{ 0x3226, 0x0FFE}, 	// ZOOM_WINDOW_Y0
{ 0x3228, 0x0FFF}, 	// ZOOM_WINDOW_Y1

{ 0x305E, 0x1a20}, 	// GLOBAL_GAIN // 0x1F24
{ 0x32D4, 0x0080}, 	// RED_DIGITAL_GAIN
{ 0x32D6, 0x0080}, 	// GREEN1_DIGITAL_GAIN
{ 0x32D8, 0x0080}, 	// GREEN2_DIGITAL_GAIN
{ 0x32DA, 0x0080}, 	// BLUE_DIGITAL_GAIN
{ 0x32DC, 0x0080}, 	// SECOND_DIGITAL_GAIN

// Features
{ 0x3C00, 0x4000}, 	// TX_CONTROL
{ 0x3C00, 0x4001}, 	// TX_CONTROL
{ 0x3C40, 0x783C}, 	// MIPI_CONTROL

{ 0x0032, 0x01D8}, 	// PAD_CONTROL


// stream off
{ 0x301A, 0x10F0}, 	// RESET_REGISTER
//Delay=10	// delay for 10ms
//POLL_{ 0x4334, 0xFFFF, !=0, DELAY=1, TIMEOUT=100
{0x4334, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFFFF},

	//{0xffff,50},

// Clear_all_HW_error_bits
{ 0x3C42, 0x1800}, 	// MIPI_STATUS
{ 0x3C42, 0x0000}, 	// MIPI_STATUS
{ 0x4322, 0xF0D0}, 	// FM_ERROR
{ 0x3C06, 0x0001}, 	// TX_STATUS

// PLL setup
{ 0x0014, 0x2045}, 	// PLL_CONTROL
{ 0x3C46, 0x096A}, 	// MIPI_LINE_BYTE_CNT
{ 0x3C40, 0xAC3C}, 	// MIPI_CONTROL -- enable standby_eof
{ 0x4312, 0x009A}, 	// FM_TRIGGER_MARK
{ 0x0012, 0x0090}, 	// PLL_P_DIVIDERS
{ 0x3C4E, 0x0F00}, 	// MIPI_TIMING_T_HS_ZERO
{ 0x3C50, 0x0B06}, 	// MIPI_TIMING_T_HS_EXIT_HS_TRAIL
{ 0x3C52, 0x0D01}, 	// MIPI_TIMING_T_CLK_POST_CLK_PRE
{ 0x3C54, 0x071E}, 	// MIPI_TIMING_T_CLK_TRAIL_CLK_ZERO
{ 0x3C56, 0x0006}, 	// MIPI_TIMING_T_LPX
{ 0x3C58, 0x0A0C}, 	// MIPI_INIT_TIMING
{ 0x001A, 0x0014}, 	// RESET_AND_MISC_CONTROL
{ 0x0010, 0x0341}, 	// PLL_DIVIDERS
{ 0x002A, 0x7F7D}, 	// PLL_P4_P5_P6_DIVIDERS
{ 0x0014, 0x2047}, 	// PLL_CONTROL

// polling PLL status
//POLL_{ 0x0014, 0x8000, ==0, DELAY=10, TIMEOUT=100
{0x0014, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x8000},

//{0xffff,50},

{ 0x0014, 0xA046}, 	// PLL_CONTROL

// define pixel area and timing
{ 0x3002, 0x001C}, 	// Y_ADDR_START
{ 0x3004, 0x0020}, 	// X_ADDR_START
{ 0x3006, 0x045F}, 	// Y_ADDR_END
{ 0x3008, 0x07A7}, 	// X_ADDR_END
{ 0x300A, 0x049D}, 	// FRAME_LENGTH_LINES
{ 0x300C, 0x0C48}, 	// LINE_LENGTH_PCK
{ 0x3010, 0x00D4}, 	// FINE_CORRECTION
{ 0x3040, 0x8041}, 	// READ_MODE
{ 0x3ECE, 0x000A}, 	// DAC_LD_2_3
{ 0x3256, 0x043F}, 	// LAST_ROW
{ 0x3254, 0x0002}, 	// FIRST_COLOR
{ 0x4304, 0x0788}, 	// FM_INWID
{ 0x4306, 0x0440}, 	// FM_INHGT
{ 0x4300, 0x0001}, 	// FM_MODE
{ 0x4302, 0x0210}, 	// FM_CTRL
{ 0x3C38, 0x0006}, 	// OUT_IMAGE_PORCHES
{ 0x3012, 0x049C}, 	// COARSE_INTEGRATION_TIME
{ 0x3014, 0x04C4}, 	// FINE_INTEGRATION_TIME
//
// [combo-1]
// for NR -- revised on 0806
//john0926 disabe denoise { 0x3382, 0x07B6}, 	// DM_CDC_HI_THR_COMB
//john0926 disabe denoise { 0x3384, 0x035E}, 	// DM_CDC_HI_THR_SATURATION
//john0926 disabe denoise { 0x3386, 0x062E}, 	// DM_CDC_LO_THR_COMB
//john0926 disabe denoise { 0x3388, 0x026F}, 	// DM_CDC_LO_THR_SATURATION
//john0926 disabe denoise { 0x338A, 0x0005}, 	// DM_DC_SINGLE_THRESHOLD
//john0926 disabe denoise { 0x338C, 0x0005}, 	// DM_CDC_AGGR_WINDOW
//john0926 disabe denoise { 0x338E, 0x0000}, 	// DM_CDC_HI_THR_OFFSET
//john0926 disabe denoise { 0x3390, 0x0000}, 	// DM_CDC_LO_THR_OFFSET
//john0926 disabe denoise { 0x3392, 0xFF55}, 	// DM_CDC_CC_NOISE_PARAMETERS
//john0926 disabe denoise { 0x3398, 0x0030}, 	// ADACD_LOWLIGHT_CONTROL
//john0926 disabe denoise { 0x3398, 0x0030},   	// ADACD_LOWLIGHT_CONTROL 
//john0926 disabe denoise { 0x339A, 0x104F},   	// ADACD_NOISE_FLOOR_RG1  
//john0926 disabe denoise { 0x339C, 0x104F},   	// ADACD_NOISE_FLOOR_BG2    
//john0926 disabe denoise { 0x339E, 0x008F},   	// ADACD_KG_R               
//john0926 disabe denoise { 0x33A0, 0x0024},   	// ADACD_KG_G1            
//john0926 disabe denoise { 0x33A2, 0x010C},   	// ADACD_KG_B               
//john0926 disabe denoise { 0x33A4, 0x0024},   	// ADACD_KG_G2  
//john0926 disabe denoise { 0x33F4, 0x0D5D}, 	// KERNEL_CONFIG
//

// turn on frame_count in SP
{ 0x3C40, 0x783E },	// MIPI_CONTROL
{ 0x3C44, 0x0080}, 	// MIPI_CUSTOM_SHORT_PKT
{ 0x3C44, 0x00C0}, 	// MIPI_CUSTOM_SHORT_PKT
{ 0x3C44, 0x0080}, 	// MIPI_CUSTOM_SHORT_PKT

// enable standby_eof and enter MIPI standby

{ 0x3C40, 0xAC34},	// MIPI_CONTROL
{0x301C, 0x0102},
{0xffff,50},
// enable standby_eof and enter MIPI standby
#ifndef CONFIG_RAWCHIPII
{ 0x3C40, 0xAC36},	// MIPI_CONTROL
{0x301C, 0x0},
#else
{0xffff,200},
{0x3C40, 0xAC36},
{0x301C, 0x0002},

// Restart the sensor
{0xffff,300},
{0x3C40, 0xAC34},
{0x301C, 0x0102},

#endif

};

static struct msm_camera_i2c_reg_conf ar0260_recommend_settings_yushanii[] = {

// checking whether sensor is in FW standby mode
//
//POLL_{ 0x0018, 0x4000, ==0, DELAY=10, TIMEOUT=100; pg while (0x18==0)
{0x0018, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x4000},

// load FW patch
{ 0x001A, 0x0005}, 	// RESET_AND_MISC_CONTROL
{ 0x001C, 0x000C}, 	// MCU_BOOT_MODE
{ 0x001A, 0x0014}, 	// RESET_AND_MISC_CONTROL
// polling MCU status
//POLL_{ 0x001C, 0xFF00, !=6, DELAY=10, TIMEOUT=100
{0x001C, 6<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFF00},
//{0xffff,50},


// Load FW Patch 11ff
{ 0x0982, 0x0001}, 	// ACCESS_CTL_STAT
{ 0x098A, 0x6000}, 	// PHYSICAL_ADDRESS_ACCESS
{ 0xE000, 0xC0F1},
{ 0xE002, 0x0C72},
{ 0xE004, 0x0760},
{ 0xE006, 0xD81D},
{ 0xE008, 0x0ABA},
{ 0xE00A, 0x0780},
{ 0xE00C, 0xE080},
{ 0xE00E, 0x0064},
{ 0xE010, 0x0001},
{ 0xE012, 0x0916},
{ 0xE014, 0x0860},
{ 0xE016, 0xD802},
{ 0xE018, 0xD900},
{ 0xE01A, 0x70CF},
{ 0xE01C, 0xFF00},
{ 0xE01E, 0x31B0},
{ 0xE020, 0xB038},
{ 0xE022, 0x1CFC},
{ 0xE024, 0xB388},
{ 0xE026, 0x76CF},
{ 0xE028, 0xFF00},
{ 0xE02A, 0x33CC},
{ 0xE02C, 0x200A},
{ 0xE02E, 0x0F80},
{ 0xE030, 0xFFFF},
{ 0xE032, 0xE048},
{ 0xE034, 0x1CFC},
{ 0xE036, 0xB008},
{ 0xE038, 0x2022},
{ 0xE03A, 0x0F80},
{ 0xE03C, 0x0000},
{ 0xE03E, 0xFC3C},
{ 0xE040, 0x2020},
{ 0xE042, 0x0F80},
{ 0xE044, 0x0000},
{ 0xE046, 0xEA34},
{ 0xE048, 0x1404},
{ 0xE04A, 0x340E},
{ 0xE04C, 0xD801},
{ 0xE04E, 0x71CF},
{ 0xE050, 0xFF00},
{ 0xE052, 0x33CC},
{ 0xE054, 0x190A},
{ 0xE056, 0x801C},
{ 0xE058, 0x208A},
{ 0xE05A, 0x0004},
{ 0xE05C, 0x1964},
{ 0xE05E, 0x8004},
{ 0xE060, 0x0C12},
{ 0xE062, 0x0760},
{ 0xE064, 0xD83C},
{ 0xE066, 0x0E5E},
{ 0xE068, 0x0880},
{ 0xE06A, 0x216F},
{ 0xE06C, 0x003F},
{ 0xE06E, 0xF1FD},
{ 0xE070, 0x0C02},
{ 0xE072, 0x0760},
{ 0xE074, 0xD81E},
{ 0xE076, 0xC0D1},
{ 0xE078, 0x7EE0},
{ 0xE07A, 0x78E0},
{ 0xE07C, 0xC0F1},
{ 0xE07E, 0xE889},
{ 0xE080, 0x70CF},
{ 0xE082, 0xFF00},
{ 0xE084, 0x0000},
{ 0xE086, 0x900E},
{ 0xE088, 0xB8E7},
{ 0xE08A, 0x0F78},
{ 0xE08C, 0xFFC1},
{ 0xE08E, 0xD800},
{ 0xE090, 0xF1F3},

// Overwrite Fixtable 10ff
{ 0x098E, 0x0000}, 	// LOGICAL_ADDRESS_ACCESS
{ 0x098A, 0x5F38}, 	// PHYSICAL_ADDRESS_ACCESS
{ 0x0990, 0xFFFF},
{ 0x0992, 0xE07C},

// Release Processor from Pause
{ 0x001C, 0x0600}, 	// MCU_BOOT_MODE

// check MCU code
//POLL_{ 0x001C, 0xFF00, <0x3C, DELAY=10, TIMEOUT=100
	{0x001C, 0x3C<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_LESS,0xFF00},

	//{0xffff,50},

// Sensor recommended -- 110211
{ 0x3E00, 0x042D}, 	// DYNAMIC_SEQRAM_00
{ 0x3E02, 0x39FF}, 	// DYNAMIC_SEQRAM_02
{ 0x3E04, 0x49FF}, 	// DYNAMIC_SEQRAM_04
{ 0x3E06, 0xFFFF}, 	// DYNAMIC_SEQRAM_06
{ 0x3E08, 0x8071}, 	// DYNAMIC_SEQRAM_08
{ 0x3E0A, 0x7211}, 	// DYNAMIC_SEQRAM_0A
{ 0x3E0C, 0xE040}, 	// DYNAMIC_SEQRAM_0C
{ 0x3E0E, 0xA840}, 	// DYNAMIC_SEQRAM_0E
{ 0x3E10, 0x4100}, 	// DYNAMIC_SEQRAM_10
{ 0x3E12, 0x1846}, 	// DYNAMIC_SEQRAM_12
{ 0x3E14, 0xA547}, 	// DYNAMIC_SEQRAM_14
{ 0x3E16, 0xAD57}, 	// DYNAMIC_SEQRAM_16
{ 0x3E18, 0x8149}, 	// DYNAMIC_SEQRAM_18
{ 0x3E1A, 0x9D49}, 	// DYNAMIC_SEQRAM_1A
{ 0x3E1C, 0x9F46}, 	// DYNAMIC_SEQRAM_1C
{ 0x3E1E, 0x8000}, 	// DYNAMIC_SEQRAM_1E
{ 0x3E20, 0x1842}, 	// DYNAMIC_SEQRAM_20
{ 0x3E22, 0x4180}, 	// DYNAMIC_SEQRAM_22
{ 0x3E24, 0x0018}, 	// DYNAMIC_SEQRAM_24
{ 0x3E26, 0x8149}, 	// DYNAMIC_SEQRAM_26
{ 0x3E28, 0x9C49}, 	// DYNAMIC_SEQRAM_28
{ 0x3E2A, 0x9347}, 	// DYNAMIC_SEQRAM_2A
{ 0x3E2C, 0x804D}, 	// DYNAMIC_SEQRAM_2C
{ 0x3E2E, 0x804A}, 	// DYNAMIC_SEQRAM_2E
{ 0x3E30, 0x100C}, 	// DYNAMIC_SEQRAM_30
{ 0x3E32, 0x8000}, 	// DYNAMIC_SEQRAM_32
{ 0x3E34, 0x1841}, 	// DYNAMIC_SEQRAM_34
{ 0x3E36, 0x4280}, 	// DYNAMIC_SEQRAM_36
{ 0x3E38, 0x0018}, 	// DYNAMIC_SEQRAM_38
{ 0x3E3A, 0x9710}, 	// DYNAMIC_SEQRAM_3A
{ 0x3E3C, 0x0C80}, 	// DYNAMIC_SEQRAM_3C
{ 0x3E3E, 0x4DA2}, 	// DYNAMIC_SEQRAM_3E
{ 0x3E40, 0x4BA0}, 	// DYNAMIC_SEQRAM_40
{ 0x3E42, 0x4A00}, 	// DYNAMIC_SEQRAM_42
{ 0x3E44, 0x1880}, 	// DYNAMIC_SEQRAM_44
{ 0x3E46, 0x4241}, 	// DYNAMIC_SEQRAM_46
{ 0x3E48, 0x0018}, 	// DYNAMIC_SEQRAM_48
{ 0x3E4A, 0xB54B}, 	// DYNAMIC_SEQRAM_4A
{ 0x3E4C, 0x1C00}, 	// DYNAMIC_SEQRAM_4C
{ 0x3E4E, 0x8000}, 	// DYNAMIC_SEQRAM_4E
{ 0x3E50, 0x1C10}, 	// DYNAMIC_SEQRAM_50
{ 0x3E52, 0x6081}, 	// DYNAMIC_SEQRAM_52
{ 0x3E54, 0x1580}, 	// DYNAMIC_SEQRAM_54
{ 0x3E56, 0x7C09}, 	// DYNAMIC_SEQRAM_56
{ 0x3E58, 0x7000}, 	// DYNAMIC_SEQRAM_58
{ 0x3E5A, 0x8082}, 	// DYNAMIC_SEQRAM_5A
{ 0x3E5C, 0x7281}, 	// DYNAMIC_SEQRAM_5C
{ 0x3E5E, 0x4C40}, 	// DYNAMIC_SEQRAM_5E
{ 0x3E60, 0x8E4D}, 	// DYNAMIC_SEQRAM_60
{ 0x3E62, 0x8110}, 	// DYNAMIC_SEQRAM_62
{ 0x3E64, 0x0CAF}, 	// DYNAMIC_SEQRAM_64
{ 0x3E66, 0x4D80}, 	// DYNAMIC_SEQRAM_66
{ 0x3E68, 0x100C}, 	// DYNAMIC_SEQRAM_68
{ 0x3E6A, 0x8440}, 	// DYNAMIC_SEQRAM_6A
{ 0x3E6C, 0x4C81}, 	// DYNAMIC_SEQRAM_6C
{ 0x3E6E, 0x7C5B}, 	// DYNAMIC_SEQRAM_6E
{ 0x3E70, 0x7000}, 	// DYNAMIC_SEQRAM_70
{ 0x3E72, 0x8054}, 	// DYNAMIC_SEQRAM_72
{ 0x3E74, 0x924C}, 	// DYNAMIC_SEQRAM_74
{ 0x3E76, 0x4078}, 	// DYNAMIC_SEQRAM_76
{ 0x3E78, 0x4D4F}, 	// DYNAMIC_SEQRAM_78
{ 0x3E7A, 0x4E98}, 	// DYNAMIC_SEQRAM_7A
{ 0x3E7C, 0x504E}, 	// DYNAMIC_SEQRAM_7C
{ 0x3E7E, 0x4F97}, 	// DYNAMIC_SEQRAM_7E
{ 0x3E80, 0x4F4E}, 	// DYNAMIC_SEQRAM_80
{ 0x3E82, 0x507C}, 	// DYNAMIC_SEQRAM_82
{ 0x3E84, 0x7B8D}, 	// DYNAMIC_SEQRAM_84
{ 0x3E86, 0x4D88}, 	// DYNAMIC_SEQRAM_86
{ 0x3E88, 0x4E10}, 	// DYNAMIC_SEQRAM_88
{ 0x3E8A, 0x0940}, 	// DYNAMIC_SEQRAM_8A
{ 0x3E8C, 0x8879}, 	// DYNAMIC_SEQRAM_8C
{ 0x3E8E, 0x5481}, 	// DYNAMIC_SEQRAM_8E
{ 0x3E90, 0x7000}, 	// DYNAMIC_SEQRAM_90
{ 0x3E92, 0x8082}, 	// DYNAMIC_SEQRAM_92
{ 0x3E94, 0x7281}, 	// DYNAMIC_SEQRAM_94
{ 0x3E96, 0x4C40}, 	// DYNAMIC_SEQRAM_96
{ 0x3E98, 0x8E4D}, 	// DYNAMIC_SEQRAM_98
{ 0x3E9A, 0x8110}, 	// DYNAMIC_SEQRAM_9A
{ 0x3E9C, 0x0CAF}, 	// DYNAMIC_SEQRAM_9C
{ 0x3E9E, 0x4D80}, 	// DYNAMIC_SEQRAM_9E
{ 0x3EA0, 0x100C}, 	// DYNAMIC_SEQRAM_A0
{ 0x3EA2, 0x8440}, 	// DYNAMIC_SEQRAM_A2
{ 0x3EA4, 0x4C81}, 	// DYNAMIC_SEQRAM_A4
{ 0x3EA6, 0x7C93}, 	// DYNAMIC_SEQRAM_A6
{ 0x3EA8, 0x7000}, 	// DYNAMIC_SEQRAM_A8
{ 0x3EAA, 0x0000}, 	// DYNAMIC_SEQRAM_AA
{ 0x3EAC, 0x0000}, 	// DYNAMIC_SEQRAM_AC
{ 0x3EAE, 0x0000}, 	// DYNAMIC_SEQRAM_AE
{ 0x3EB0, 0x0000}, 	// DYNAMIC_SEQRAM_B0
{ 0x3EB2, 0x0000}, 	// DYNAMIC_SEQRAM_B2
{ 0x3EB4, 0x0000}, 	// DYNAMIC_SEQRAM_B4
{ 0x3EB6, 0x0000}, 	// DYNAMIC_SEQRAM_B6
{ 0x3EB8, 0x0000}, 	// DYNAMIC_SEQRAM_B8
{ 0x3EBA, 0x0000}, 	// DYNAMIC_SEQRAM_BA
{ 0x3EBC, 0x0000}, 	// DYNAMIC_SEQRAM_BC
{ 0x3EBE, 0x0000}, 	// DYNAMIC_SEQRAM_BE
{ 0x3EC0, 0x0000}, 	// DYNAMIC_SEQRAM_C0
{ 0x3EC2, 0x0000}, 	// DYNAMIC_SEQRAM_C2
{ 0x3EC4, 0x0000}, 	// DYNAMIC_SEQRAM_C4
{ 0x3EC6, 0x0000}, 	// DYNAMIC_SEQRAM_C6
{ 0x3EC8, 0x0000}, 	// DYNAMIC_SEQRAM_C8
{ 0x3ECA, 0x0000}, 	// DYNAMIC_SEQRAM_CA

// Char settings -- 110211
{ 0x30B2, 0xC000}, 	// CALIB_TIED_OFFSET
{ 0x30D4, 0x9400}, 	// COLUMN_CORRECTION
{ 0x31C0, 0x0000}, 	// FUSE_REF_ADDR
{ 0x316A, 0x8200}, 	// DAC_FBIAS
{ 0x316C, 0x8200}, 	// DAC_TXLO
{ 0x3EFE, 0x2808}, 	// DAC_LD_TXLO
{ 0x3EFC, 0x2868}, 	// DAC_LD_FBIAS
{ 0x3ED2, 0xD165}, 	// DAC_LD_6_7
{ 0x3EF2, 0xD165}, 	// DAC_LP_6_7
{ 0x3ED8, 0x7F1A}, 	// DAC_LD_12_13
{ 0x3EDA, 0x2828}, 	// DAC_LD_14_15
{ 0x3EE2, 0x0058}, 	// DAC_LD_22_23

{ 0x002C, 0x000F}, 	// PLL_P7_DIVIDER

// CPIPE_Calibration
// N FW DC
{ 0x3382, 0x012C}, 	// DM_CDC_HI_THR_COMB
{ 0x3384, 0x0158}, 	// DM_CDC_HI_THR_SATURATION
{ 0x3386, 0x015C}, 	// DM_CDC_LO_THR_COMB
{ 0x3388, 0x00E6}, 	// DM_CDC_LO_THR_SATURATION

//
{ 0x338A, 0x000F}, 	// DM_DC_SINGLE_THRESHOLD

// CPIPE_Preference
// power save
{ 0x3276, 0x03FF}, 	// BLACK_LEVEL_TO_CCM
{ 0x32B2, 0x0000}, 	// DKDELTA_CCM_CTL
{ 0x3210, 0x0000}, 	// COLOR_PIPELINE_CONTROL
{ 0x3226, 0x0FFE}, 	// ZOOM_WINDOW_Y0
{ 0x3228, 0x0FFF}, 	// ZOOM_WINDOW_Y1

{ 0x305E, 0x1a20}, 	// GLOBAL_GAIN // 0x1F24
{ 0x32D4, 0x0080}, 	// RED_DIGITAL_GAIN
{ 0x32D6, 0x0080}, 	// GREEN1_DIGITAL_GAIN
{ 0x32D8, 0x0080}, 	// GREEN2_DIGITAL_GAIN
{ 0x32DA, 0x0080}, 	// BLUE_DIGITAL_GAIN
{ 0x32DC, 0x0080}, 	// SECOND_DIGITAL_GAIN

// Features
{ 0x3C00, 0x4000}, 	// TX_CONTROL
{ 0x3C00, 0x4001}, 	// TX_CONTROL
{ 0x3C40, 0x783C}, 	// MIPI_CONTROL

{ 0x0032, 0x01D8}, 	// PAD_CONTROL


// stream off
{ 0x301A, 0x10F0}, 	// RESET_REGISTER
//Delay=10	// delay for 10ms
//POLL_{ 0x4334, 0xFFFF, !=0, DELAY=1, TIMEOUT=100
{0x4334, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFFFF},

	//{0xffff,50},

// Clear_all_HW_error_bits
{ 0x3C42, 0x1800}, 	// MIPI_STATUS
{ 0x3C42, 0x0000}, 	// MIPI_STATUS
{ 0x4322, 0xF0D0}, 	// FM_ERROR
{ 0x3C06, 0x0001}, 	// TX_STATUS

// PLL setup
{ 0x0014, 0x2045}, 	// PLL_CONTROL
{ 0x3C46, 0x096A}, 	// MIPI_LINE_BYTE_CNT
{ 0x3C40, 0xAC3C}, 	// MIPI_CONTROL -- enable standby_eof
{ 0x4312, 0x009A}, 	// FM_TRIGGER_MARK
{ 0x0012, 0x0090}, 	// PLL_P_DIVIDERS
{ 0x3C4E, 0x0F00}, 	// MIPI_TIMING_T_HS_ZERO
{ 0x3C50, 0x0B06}, 	// MIPI_TIMING_T_HS_EXIT_HS_TRAIL
{ 0x3C52, 0x0D01}, 	// MIPI_TIMING_T_CLK_POST_CLK_PRE
{ 0x3C54, 0x071E}, 	// MIPI_TIMING_T_CLK_TRAIL_CLK_ZERO
{ 0x3C56, 0x0006}, 	// MIPI_TIMING_T_LPX
{ 0x3C58, 0x0A0C}, 	// MIPI_INIT_TIMING
{ 0x001A, 0x0014}, 	// RESET_AND_MISC_CONTROL
{ 0x0010, 0x0341}, 	// PLL_DIVIDERS
{ 0x002A, 0x7F7D}, 	// PLL_P4_P5_P6_DIVIDERS
{ 0x0014, 0x2047}, 	// PLL_CONTROL

// polling PLL status
//POLL_{ 0x0014, 0x8000, ==0, DELAY=10, TIMEOUT=100
{0x0014, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x8000},

//{0xffff,50},

{ 0x0014, 0xA046}, 	// PLL_CONTROL

// define pixel area and timing
{ 0x3002, 0x001C}, 	// Y_ADDR_START
{ 0x3004, 0x0020}, 	// X_ADDR_START
{ 0x3006, 0x045F}, 	// Y_ADDR_END
{ 0x3008, 0x07A7}, 	// X_ADDR_END
{ 0x300A, 0x049D}, 	// FRAME_LENGTH_LINES
{ 0x300C, 0x0C48}, 	// LINE_LENGTH_PCK
{ 0x3010, 0x00D4}, 	// FINE_CORRECTION
{ 0x3040, 0x8041}, 	// READ_MODE
{ 0x3ECE, 0x000A}, 	// DAC_LD_2_3
{ 0x3256, 0x043F}, 	// LAST_ROW
{ 0x3254, 0x0002}, 	// FIRST_COLOR
{ 0x4304, 0x0788}, 	// FM_INWID
{ 0x4306, 0x0440}, 	// FM_INHGT /*TODO:Fix me: temp solution: send one line to Y2 as status line */ //HTC_START steven wu 20121120 fix preview segmentation(CAMIF error)
{ 0x4300, 0x0001}, 	// FM_MODE
{ 0x4302, 0x0210}, 	// FM_CTRL
{ 0x3C38, 0x0006}, 	// OUT_IMAGE_PORCHES
{ 0x3012, 0x0172}, 	// COARSE_INTEGRATION_TIME		0x049C
{ 0x3014, 0x04C4}, 	// FINE_INTEGRATION_TIME
//
// [combo-1]
// for NR -- revised on 0806
//john0926 disabe denoise { 0x3382, 0x07B6}, 	// DM_CDC_HI_THR_COMB
//john0926 disabe denoise { 0x3384, 0x035E}, 	// DM_CDC_HI_THR_SATURATION
//john0926 disabe denoise { 0x3386, 0x062E}, 	// DM_CDC_LO_THR_COMB
//john0926 disabe denoise { 0x3388, 0x026F}, 	// DM_CDC_LO_THR_SATURATION
//john0926 disabe denoise { 0x338A, 0x0005}, 	// DM_DC_SINGLE_THRESHOLD
//john0926 disabe denoise { 0x338C, 0x0005}, 	// DM_CDC_AGGR_WINDOW
//john0926 disabe denoise { 0x338E, 0x0000}, 	// DM_CDC_HI_THR_OFFSET
//john0926 disabe denoise { 0x3390, 0x0000}, 	// DM_CDC_LO_THR_OFFSET
//john0926 disabe denoise { 0x3392, 0xFF55}, 	// DM_CDC_CC_NOISE_PARAMETERS
//john0926 disabe denoise { 0x3398, 0x0030}, 	// ADACD_LOWLIGHT_CONTROL
//john0926 disabe denoise { 0x3398, 0x0030},   	// ADACD_LOWLIGHT_CONTROL
//john0926 disabe denoise { 0x339A, 0x104F},   	// ADACD_NOISE_FLOOR_RG1
//john0926 disabe denoise { 0x339C, 0x104F},   	// ADACD_NOISE_FLOOR_BG2
//john0926 disabe denoise { 0x339E, 0x008F},   	// ADACD_KG_R
//john0926 disabe denoise { 0x33A0, 0x0024},   	// ADACD_KG_G1
//john0926 disabe denoise { 0x33A2, 0x010C},   	// ADACD_KG_B
//john0926 disabe denoise { 0x33A4, 0x0024},   	// ADACD_KG_G2
//john0926 disabe denoise { 0x33F4, 0x0D5D}, 	// KERNEL_CONFIG
//

// turn on frame_count in SP
//{ 0x3C40, 0x783E },	// MIPI_CONTROL
{ 0x3C44, 0x0080}, 	// MIPI_CUSTOM_SHORT_PKT
{ 0x3C44, 0x00C0}, 	// MIPI_CUSTOM_SHORT_PKT
{ 0x3C44, 0x0080}, 	// MIPI_CUSTOM_SHORT_PKT

// enable standby_eof and enter MIPI standby
#if 1
{0x301C, 0x0102},
{0x3C40, 0xAC34},	// MIPI_CONTROL
{0xffff,50},
#endif

// enable standby_eof and enter MIPI standby
#if 0
{ 0x3C40, 0xAC36},	// MIPI_CONTROL
{0x301C, 0x0},
#else
//{0xffff,200},
{0x301C, 0x0002},
{0x3C40, 0xAC36},

#if 0
// Restart the sensor
{0xffff,50},
{0x301C, 0x0102},
{0x3C40, 0xAC34},
#endif
#endif

};

static struct msm_sensor_output_info_t ar0260_dimensions[] = {
	{
		.x_output = 0x0788, //1928
		.y_output = 0x0440, //1089
		.line_length_pclk = 0x0C48, //3144
		.frame_length_lines = 0x049D, //1181
		.vt_pixel_clk = 111428570,//109714286,
		.op_pixel_clk = 78000000,//78000000,//76800000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
		
	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C48,
		.frame_length_lines = 0x049D,
		.vt_pixel_clk = 111428570,//109714286,
		.op_pixel_clk = 78000000,//76800000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
};

static struct msm_sensor_output_info_t ar0260_dimensions_yushanii[] = {
	{
		.x_output = 0x0788, //1928
		.y_output = 0x0440, //1089
		.line_length_pclk = 0x0C48, //3144
		.frame_length_lines = 0x049D, //1181
		.vt_pixel_clk = 111428570,//109714286,
		.op_pixel_clk = 78000000,//78000000,//76800000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C48,
		.frame_length_lines = 0x049D,
		.vt_pixel_clk = 111428570,//109714286,
		.op_pixel_clk = 78000000,//76800000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
};

static struct msm_sensor_output_reg_addr_t ar0260_reg_addr = {
	.x_output = 0x4304,//0xC86C,
	.y_output = 0x4306,//0xC86E,
	.line_length_pclk = 0x300C,//0xC814,
	.frame_length_lines = 0x300A,//0xC812,
};
static struct msm_sensor_exp_gain_info_t ar0260_exp_gain_info = {
	.coarse_int_time_addr = 0x3012,
	.global_gain_addr = 0x305E,
	.vert_offset = 4,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
};


#else


static struct msm_camera_i2c_reg_conf ar0260_recommend_settings[] = {
	
	{ 0x098E, 0xCA12},	
	
	{ 0xCA12, 0x01,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	// CAM_SYSCTL_PLL_ENABLE
	{ 0xCA13, 0x00,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	// CAM_SYSCTL_SENSOR_CLK_DIV2_EN
	
	{ 0xCA14, 0x0120},	// CAM_SYSCTL_PLL_DIVIDER_M_N
	{ 0xCA16, 0x1090},	// CAM_SYSCTL_PLL_DIVIDER_P
	{ 0xCA18, 0x7F7D},	// CAM_SYSCTL_PLL_DIVIDER_P4_P5_P6
	{ 0xCA1A, 0x000F},	// CAM_SYSCTL_PLL_DIVIDER_P7
	//{ 0xC808, 0x0345},
	//{ 0xC80a, 0x0DB7},	// CAM_SENSOR_CFG_PIXCLK
	{ 0xC800, 0x001C},	// CAM_SENSOR_CFG_Y_ADDR_START
	{ 0xC802, 0x0020},	// CAM_SENSOR_CFG_X_ADDR_START
	{ 0xC804, 0x045F},	// CAM_SENSOR_CFG_Y_ADDR_END
	{ 0xC806, 0x07A7},	// CAM_SENSOR_CFG_X_ADDR_END
	{ 0xC80E, 0x0336},	// CAM_SENSOR_CFG_FINE_INTEG_TIME_MIN
	{ 0xC810, 0x0A6E},	// CAM_SENSOR_CFG_FINE_INTEG_TIME_MAX
	{ 0xC81E, 0x300A},	// CAM_SENSOR_CFG_REG_0_ADDRESS
	{ 0xC820, 0x0495},	// CAM_SENSOR_CFG_REG_0_DATA
	{ 0xC812, 0x0495},	// CAM_SENSOR_CFG_FRAME_LENGTH_LINES
	{ 0xC814, 0x0C2E},	// CAM_SENSOR_CFG_LINE_LENGTH_PCK
	{ 0xC816, 0x00D4},	// CAM_SENSOR_CFG_FINE_CORRECTION
	{ 0xC818, 0x043F},	// CAM_SENSOR_CFG_CPIPE_LAST_ROW
	{ 0xC830, 0x0000},	// CAM_SENSOR_CONTROL_READ_MODE
	{ 0xC86C, 0x0788},	// CAM_OUTPUT_WIDTH
	{ 0xC86E, 0x0440},	// CAM_OUTPUT_HEIGHT
	{ 0xC830, 0x0002},	// CAM_SENSOR_CONTROL_READ_MODE
	{ 0x3E00, 0x042D},	// DYNAMIC_SEQRAM_00
	{ 0x3E02, 0x39FF},	// DYNAMIC_SEQRAM_02
	{ 0x3E04, 0x49FF},	// DYNAMIC_SEQRAM_04
	{ 0x3E06, 0xFFFF},	// DYNAMIC_SEQRAM_06
	{ 0x3E08, 0x8071},	// DYNAMIC_SEQRAM_08
	{ 0x3E0A, 0x7211},	// DYNAMIC_SEQRAM_0A
	{ 0x3E0C, 0xE040},	// DYNAMIC_SEQRAM_0C
	{ 0x3E0E, 0xA840},	// DYNAMIC_SEQRAM_0E
	{ 0x3E10, 0x4100},	// DYNAMIC_SEQRAM_10
	{ 0x3E12, 0x1846},	// DYNAMIC_SEQRAM_12
	{ 0x3E14, 0xA547},	// DYNAMIC_SEQRAM_14
	{ 0x3E16, 0xAD57},	// DYNAMIC_SEQRAM_16
	{ 0x3E18, 0x8149},	// DYNAMIC_SEQRAM_18
	{ 0x3E1A, 0x9D49},	// DYNAMIC_SEQRAM_1A
	{ 0x3E1C, 0x9F46},	// DYNAMIC_SEQRAM_1C
	{ 0x3E1E, 0x8000},	// DYNAMIC_SEQRAM_1E
	{ 0x3E20, 0x1842},	// DYNAMIC_SEQRAM_20
	{ 0x3E22, 0x4180},	// DYNAMIC_SEQRAM_22
	{ 0x3E24, 0x0018},	// DYNAMIC_SEQRAM_24
	{ 0x3E26, 0x8149},	// DYNAMIC_SEQRAM_26
	{ 0x3E28, 0x9C49},	// DYNAMIC_SEQRAM_28
	{ 0x3E2A, 0x9347},	// DYNAMIC_SEQRAM_2A
	{ 0x3E2C, 0x804D},	// DYNAMIC_SEQRAM_2C
	{ 0x3E2E, 0x804A},	// DYNAMIC_SEQRAM_2E
	{ 0x3E30, 0x100C},	// DYNAMIC_SEQRAM_30
	{ 0x3E32, 0x8000},	// DYNAMIC_SEQRAM_32
	{ 0x3E34, 0x1841},	// DYNAMIC_SEQRAM_34
	{ 0x3E36, 0x4280},	// DYNAMIC_SEQRAM_36
	{ 0x3E38, 0x0018},	// DYNAMIC_SEQRAM_38
	{ 0x3E3A, 0x9710},	// DYNAMIC_SEQRAM_3A
	{ 0x3E3C, 0x0C80},	// DYNAMIC_SEQRAM_3C
	{ 0x3E3E, 0x4DA2},	// DYNAMIC_SEQRAM_3E
	{ 0x3E40, 0x4BA0},	// DYNAMIC_SEQRAM_40
	{ 0x3E42, 0x4A00},	// DYNAMIC_SEQRAM_42
	{ 0x3E44, 0x1880},	// DYNAMIC_SEQRAM_44
	{ 0x3E46, 0x4241},	// DYNAMIC_SEQRAM_46
	{ 0x3E48, 0x0018},	// DYNAMIC_SEQRAM_48
	{ 0x3E4A, 0xB54B},	// DYNAMIC_SEQRAM_4A
	{ 0x3E4C, 0x1C00},	// DYNAMIC_SEQRAM_4C
	{ 0x3E4E, 0x8000},	// DYNAMIC_SEQRAM_4E
	{ 0x3E50, 0x1C10},	// DYNAMIC_SEQRAM_50
	{ 0x3E52, 0x6081},	// DYNAMIC_SEQRAM_52
	{ 0x3E54, 0x1580},	// DYNAMIC_SEQRAM_54
	{ 0x3E56, 0x7C09},	// DYNAMIC_SEQRAM_56
	{ 0x3E58, 0x7000},	// DYNAMIC_SEQRAM_58
	{ 0x3E5A, 0x8082},	// DYNAMIC_SEQRAM_5A
	{ 0x3E5C, 0x7281},	// DYNAMIC_SEQRAM_5C
	{ 0x3E5E, 0x4C40},	// DYNAMIC_SEQRAM_5E
	{ 0x3E60, 0x8E4D},	// DYNAMIC_SEQRAM_60
	{ 0x3E62, 0x8110},	// DYNAMIC_SEQRAM_62
	{ 0x3E64, 0x0CAF},	// DYNAMIC_SEQRAM_64
	{ 0x3E66, 0x4D80},	// DYNAMIC_SEQRAM_66
	{ 0x3E68, 0x100C},	// DYNAMIC_SEQRAM_68
	{ 0x3E6A, 0x8440},	// DYNAMIC_SEQRAM_6A
	{ 0x3E6C, 0x4C81},	// DYNAMIC_SEQRAM_6C
	{ 0x3E6E, 0x7C5B},	// DYNAMIC_SEQRAM_6E
	{ 0x3E70, 0x7000},	// DYNAMIC_SEQRAM_70
	{ 0x3E72, 0x8054},	// DYNAMIC_SEQRAM_72
	{ 0x3E74, 0x924C},	// DYNAMIC_SEQRAM_74
	{ 0x3E76, 0x4078},	// DYNAMIC_SEQRAM_76
	{ 0x3E78, 0x4D4F},	// DYNAMIC_SEQRAM_78
	{ 0x3E7A, 0x4E98},	// DYNAMIC_SEQRAM_7A
	{ 0x3E7C, 0x504E},	// DYNAMIC_SEQRAM_7C
	{ 0x3E7E, 0x4F97},	// DYNAMIC_SEQRAM_7E
	{ 0x3E80, 0x4F4E},	// DYNAMIC_SEQRAM_80
	{ 0x3E82, 0x507C},	// DYNAMIC_SEQRAM_82
	{ 0x3E84, 0x7B8D},	// DYNAMIC_SEQRAM_84
	{ 0x3E86, 0x4D88},	// DYNAMIC_SEQRAM_86
	{ 0x3E88, 0x4E10},	// DYNAMIC_SEQRAM_88
	{ 0x3E8A, 0x0940},	// DYNAMIC_SEQRAM_8A
	{ 0x3E8C, 0x8879},	// DYNAMIC_SEQRAM_8C
	{ 0x3E8E, 0x5481},	// DYNAMIC_SEQRAM_8E
	{ 0x3E90, 0x7000},	// DYNAMIC_SEQRAM_90
	{ 0x3E92, 0x8082},	// DYNAMIC_SEQRAM_92
	{ 0x3E94, 0x7281},	// DYNAMIC_SEQRAM_94
	{ 0x3E96, 0x4C40},	// DYNAMIC_SEQRAM_96
	{ 0x3E98, 0x8E4D},	// DYNAMIC_SEQRAM_98
	{ 0x3E9A, 0x8110},	// DYNAMIC_SEQRAM_9A
	{ 0x3E9C, 0x0CAF},	// DYNAMIC_SEQRAM_9C
	{ 0x3E9E, 0x4D80},	// DYNAMIC_SEQRAM_9E
	{ 0x3EA0, 0x100C},	// DYNAMIC_SEQRAM_A0
	{ 0x3EA2, 0x8440},	// DYNAMIC_SEQRAM_A2
	{ 0x3EA4, 0x4C81},	// DYNAMIC_SEQRAM_A4
	{ 0x3EA6, 0x7C93},	// DYNAMIC_SEQRAM_A6
	{ 0x3EA8, 0x7000},	// DYNAMIC_SEQRAM_A8
	{ 0x3EAA, 0x0000},	// DYNAMIC_SEQRAM_AA
	{ 0x3EAC, 0x0000},	// DYNAMIC_SEQRAM_AC
	{ 0x3EAE, 0x0000},	// DYNAMIC_SEQRAM_AE
	{ 0x3EB0, 0x0000},	// DYNAMIC_SEQRAM_B0
	{ 0x3EB2, 0x0000},	// DYNAMIC_SEQRAM_B2
	{ 0x3EB4, 0x0000},	// DYNAMIC_SEQRAM_B4
	{ 0x3EB6, 0x0000},	// DYNAMIC_SEQRAM_B6
	{ 0x3EB8, 0x0000},	// DYNAMIC_SEQRAM_B8
	{ 0x3EBA, 0x0000},	// DYNAMIC_SEQRAM_BA
	{ 0x3EBC, 0x0000},	// DYNAMIC_SEQRAM_BC
	{ 0x3EBE, 0x0000},	// DYNAMIC_SEQRAM_BE
	{ 0x3EC0, 0x0000},	// DYNAMIC_SEQRAM_C0
	{ 0x3EC2, 0x0000},	// DYNAMIC_SEQRAM_C2
	{ 0x3EC4, 0x0000},	// DYNAMIC_SEQRAM_C4
	{ 0x3EC6, 0x0000},	// DYNAMIC_SEQRAM_C6
	{ 0x3EC8, 0x0000},	// DYNAMIC_SEQRAM_C8
	{ 0x3ECA, 0x0000},	// DYNAMIC_SEQRAM_CA
	{ 0x30B2, 0xC000},	// CALIB_TIED_OFFSET
	{ 0x30D4, 0x9400},	// COLUMN_CORRECTION
	{ 0x31C0, 0x0000},	// FUSE_REF_ADDR
	{ 0x316A, 0x8200},	// DAC_FBIAS
	{ 0x316C, 0x8200},	// DAC_TXLO
	{ 0x3EFE, 0x2808},	// DAC_LD_TXLO
	{ 0x3EFC, 0x2868},	// DAC_LD_FBIAS
	{ 0x3ED2, 0xD165},	// DAC_LD_6_7
	{ 0x3EF2, 0xD165},	// DAC_LP_6_7
	{ 0x3ED8, 0x7F1A},	// DAC_LD_12_13
	{ 0x3EDA, 0x2828},	// DAC_LD_14_15
	{ 0x3EE2, 0x0058},	// DAC_LD_22_23
	{ 0xC984, 0x012C},	// CAM_LL_START_CDC_HI_THR_COMB
	{ 0xC988, 0x0158},	// CAM_LL_START_CDC_HI_THR_SATUR
	{ 0xC98C, 0x012C},	// CAM_LL_MID_CDC_HI_THR_COMB
	{ 0xC990, 0x0158},	// CAM_LL_MID_CDC_HI_THR_SATUR
	{ 0xC994, 0x012C},	// CAM_LL_STOP_CDC_HI_THR_COMB
	{ 0xC998, 0x0158},	// CAM_LL_STOP_CDC_HI_THR_SATUR
	{ 0xC986, 0x015C},	// CAM_LL_START_CDC_LO_THR_COMB
	{ 0xC98A, 0x00E6},	// CAM_LL_START_CDC_LO_THR_SATUR
	{ 0xC98E, 0x015C},	// CAM_LL_MID_CDC_LO_THR_COMB
	{ 0xC992, 0x00E6},	// CAM_LL_MID_CDC_LO_THR_SATUR
	{ 0xC996, 0x015C},	// CAM_LL_STOP_CDC_LO_THR_COMB
	{ 0xC99A, 0x00E6},	// CAM_LL_STOP_CDC_LO_THR_SATUR
	{ 0xC97E, 0x000F},	// CAM_LL_START_DC_SINGLE_PIXEL_THR
	{ 0xC980, 0x000F},	// CAM_LL_STOP_DC_SINGLE_PIXEL_THR
	{ 0x4300, 0x0000},	// FM_MODE
	{ 0xCA1C, 0x8040},	// CAM_PORT_OUTPUT_CONTROL
	{ 0x001E, 0x0777},	// PAD_SLEW
	{ 0xC870, 0x4090},	// CAM_OUTPUT_FORMAT
	{ 0xC870, 0x4090},	// CAM_OUTPUT_FORMAT
	{ 0xCA1C, 0x8041},	// CAM_PORT_OUTPUT_CONTROL
	{ 0xDC00, 0x28,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	// SYSMGR_NEXT_STATE  CHANGE-CONFIG COMMAND
	{ 0x0080, 0x8002},	// COMMAND_REGISTER
	
	//POLL_FIELD= COMMAND_REGISTER, HOST_COMMAND_1, !=0, DELAY=1, TIMEOUT=100
	//delay=100
	{ 0x0080, 0x2,MSM_CAMERA_I2C_UNSET_WORD_MASK,MSM_CAMERA_I2C_CMD_POLL},	// COMMAND_REGISTER
	
	{ 0x001A, 0x0000},	// RESET_AND_MISC_CONTROL
	{ 0x3276, 0x03FF},	// BLACK_LEVEL_TO_CCM
	{ 0x32B2, 0x0000},	// DKDELTA_CCM_CTL
	{ 0x3210, 0x0000},	// COLOR_PIPELINE_CONTROL
	{ 0x3226, 0x0FFE},	// ZOOM_WINDOW_Y0
	{ 0x3228, 0x0FFF},	// ZOOM_WINDOW_Y1
	{ 0x32D4, 0x0080},	// RED_DIGITAL_GAIN
	{ 0x32D6, 0x0080},	// GREEN1_DIGITAL_GAIN
	{ 0x32D8, 0x0080},	// GREEN2_DIGITAL_GAIN
	{ 0x32DA, 0x0080},	// BLUE_DIGITAL_GAIN
	{ 0x32DC, 0x0080},	// SECOND_DIGITAL_GAIN
	{ 0x3330, 0x0100},	// OUTPUT_FORMAT_TEST
	{ 0x4302, 0x0210},	// FM_CTRL
	{ 0x4300, 0x0001},	// FM_MODE
	{ 0x3012, 0x0DAC},//john0927 let initila AE smooth { 0x3012, 0x0488},	// COARSE_INTEGRATION_TIME
	{ 0x305E, 0x1820},//john0927 let initila AE smooth { 0x305E, 0x1F24},	// GLOBAL_GAIN
	{ 0x3C40, 0xAC34},
	{ 0x3C40, 0xAC36},


};

static struct msm_sensor_output_info_t ar0260_dimensions[] = {
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,//76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,//76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
	},
};

static struct msm_sensor_output_reg_addr_t ar0260_reg_addr = {
	.x_output = 0xC86C,
	.y_output = 0xC86E,
	.line_length_pclk = 0xC814,
	.frame_length_lines = 0xC812,
};
static struct msm_sensor_exp_gain_info_t ar0260_exp_gain_info = {
	.coarse_int_time_addr = 0x3012,
	.global_gain_addr = 0x305E,
	.vert_offset = 4,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
};
#endif


static struct v4l2_subdev_info ar0260_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ar0260_init_conf[] = {
	{&ar0260_recommend_settings[0],
	ARRAY_SIZE(ar0260_recommend_settings), 0, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array ar0260_init_conf_yushanii[] = {
	{&ar0260_recommend_settings_yushanii[0],
	ARRAY_SIZE(ar0260_recommend_settings_yushanii), 0, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array ar0260_confs[] = {
	{&ar0260_snap_settings[0],
	ARRAY_SIZE(ar0260_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&ar0260_prev_settings[0],
	ARRAY_SIZE(ar0260_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};




static struct msm_camera_csid_vc_cfg ar0260_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ar0260_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ar0260_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *ar0260_csi_params_array[] = {
	&ar0260_csi_params,
	&ar0260_csi_params,
};




static struct msm_sensor_id_info_t ar0260_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x4581,
};



#if 0	/* HTC_START for i2c operation */

static struct i2c_client *ar0260_client;

#define MAX_I2C_RETRIES 20
#define CHECK_STATE_TIME 100

enum ar0260_width {
	WORD_LEN,
	BYTE_LEN
};


static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; /* number sent */

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}

static int ar0260_i2c_txdata(unsigned short saddr,
				  unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer_retry(ar0260_client->adapter, msg, 1) < 0) {
		pr_err("ar0260_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int ar0260_i2c_write(unsigned short saddr,
				 unsigned short waddr, unsigned short wdata,
				 enum ar0260_width width)
{
	int rc = -EIO;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN:{
			/*pr_info("i2c_write, WORD_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = (waddr & 0xFF00) >> 8;
			buf[1] = (waddr & 0x00FF);
			buf[2] = (wdata & 0xFF00) >> 8;
			buf[3] = (wdata & 0x00FF);

			rc = ar0260_i2c_txdata(saddr, buf, 4);
		}
		break;

	case BYTE_LEN:{
			/*pr_info("i2c_write, BYTE_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = waddr;
			buf[1] = wdata;
			rc = ar0260_i2c_txdata(saddr, buf, 2);
		}
		break;

	default:
		break;
	}

	if (rc < 0)
		pr_info("i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		     waddr, wdata);

	return rc;
}

#if 0
static int ar0260_i2c_write_table(struct ar0260_i2c_reg_conf
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ar0260_i2c_write(ar0260_client->addr,
				       reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				       reg_conf_tbl->width);
		if (rc < 0) {
		pr_err("%s: num_of_items_in_table=%d\n", __func__,
			num_of_items_in_table);
			break;
		}
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}
#endif

static int ar0260_i2c_rxdata(unsigned short saddr,
			      unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 2,  /* .len = 1, */
		 .buf = rxdata,
		 },
		{
		 .addr = saddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer_retry(ar0260_client->adapter, msgs, 2) < 0) {
		pr_err("ar0260_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

/*read 2 bytes data from sensor via I2C */
static int32_t ar0260_i2c_read_w(unsigned short saddr, unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = ar0260_i2c_rxdata(saddr, buf, 2);
	if (rc < 0)
		return rc;

	*rdata = buf[0] << 8 | buf[1];

	if (rc < 0)
		CDBG("ar0260_i2c_read_w failed!\n");

	return rc;
}

#if 0
static int ar0260_i2c_read(unsigned short saddr,
				unsigned short raddr, unsigned char *rdata)
{
	int rc = 0;
	unsigned char buf[1];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	rc = ar0260_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;
	*rdata = buf[0];
	if (rc < 0)
		pr_info("ar0260_i2c_read failed!\n");

	return rc;
}
#endif
static int ar0260_i2c_write_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, unsigned short state)
{
	int rc;
	unsigned short check_value;
	unsigned short check_bit;

	if (state)
		check_bit = 0x0001 << bit;
	else
		check_bit = 0xFFFF & (~(0x0001 << bit));
	pr_info("ar0260_i2c_write_bit check_bit:0x%4x", check_bit);
	rc = ar0260_i2c_read_w(saddr, raddr, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: ar0260: 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);
	if (state)
		check_value = (check_value | check_bit);
	else
		check_value = (check_value & check_bit);

	pr_info("%s: ar0260: Set to 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);

	rc = ar0260_i2c_write(saddr, raddr, check_value,
		WORD_LEN);
	return rc;
}

static int ar0260_i2c_check_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, int check_state)
{
	int k;
	unsigned short check_value;
	unsigned short check_bit;
	check_bit = 0x0001 << bit;
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		ar0260_i2c_read_w(ar0260_client->addr,
			      raddr, &check_value);
		if (check_state) {
			if ((check_value & check_bit))
			break;
		} else {
			if (!(check_value & check_bit))
			break;
		}
		msleep(1);
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s failed addr:0x%2x data check_bit:0x%2x",
			__func__, raddr, check_bit);
		return -1;
	}
	return 1;
}


#endif	/* HTC_END */

static int ar0260_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	if (data->sensor_platform_info)
		ar0260_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	return 0;

#if 0	/* HTC_START for i2c operation */
	if (ar0260_s_ctrl.sensor_i2c_client && ar0260_s_ctrl.sensor_i2c_client->client)
		ar0260_client = ar0260_s_ctrl.sensor_i2c_client->client;
	#if 1	//FIXME:	test only and never run here
	ar0260_i2c_check_bit(0, 0, 0, 0);
	ar0260_i2c_write_bit(0, 0, 0, 0);
	#endif
#endif	/* HTC_END */
}

static const char *ar0260Vendor = "aptina";
static const char *ar0260NAME = "ar0260";
static const char *ar0260Size = "2.1M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", ar0260Vendor, ar0260NAME, ar0260Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ar0260;

static int ar0260_sysfs_init(void)
{
	int ret ;
	pr_info("ar0260:kobject creat and add\n");
	android_ar0260 = kobject_create_and_add("android_camera2", NULL);
	if (android_ar0260 == NULL) {
		pr_info("ar0260_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ar0260:sysfs_create_file\n");
	ret = sysfs_create_file(android_ar0260, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ar0260_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ar0260);
	}

	return 0 ;
}

static struct msm_camera_i2c_client ar0260_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#define CAM2_RSTz       (2)//GPIO(2)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz

int32_t ar0260_power_up(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	struct sensor_cfg_data cdata;  //CC140211
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);


#if 1	/* reset pin */	// FIXME: to make sure the reset pin pull low before power up
		rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
		if (rc < 0)
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		else {
			gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
			gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
		}
		mdelay(10); //temp timing setting
#endif

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		return rc;
	}

	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			return rc;
		}
	}

#ifdef CONFIG_RAWCHIPII
    if (sdata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
    	Ilp0100_enableIlp0100SensorClock(SENSOR_1);
        mdelay(35);	//temp timing setting
	}
#endif

#if 0	/* reset pin */
	mdelay(10);	//temp timing setting
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}

	mdelay(50);	//temp timing setting
#else

	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		
	}
	mdelay(1); // 50 ~ 200 ms
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	mdelay(1); // 50 ~ 200 ms
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);


	
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(25); // for readid delay
#endif

	ar0260_sensor_open_init(sdata);
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
	pr_info("%s end\n", __func__);

	return 0;  /*msm_sensor_power_up(sdata)*/
}

int32_t ar0260_power_down(struct msm_sensor_ctrl_t *s_ctrl)//(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

#if 0	/* reset pin */
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	mdelay(10); //temp timing setting
#endif

	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
	}
	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err("%s failed to disable power\n", __func__);
		return rc;
	}
	return 0;  /*msm_sensor_power_down(sdata);*/
}

int32_t ar0260_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	/* Retry mechanism for sensor probe fail. Disable by default in CC stage */
	/* int max_probe_count = 5; */
	int max_probe_count = 1;
	int probe_count = 0;

	pr_info("%s\n", __func__);

sensor_probe_retry:
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ar0260_sysfs_init();
	else {
		/* Retry mechanism for sensor probe fail. Disable by default in CC stage */
		probe_count++;
		if(probe_count < max_probe_count) {
			pr_info("%s  apply sensor probe retry mechanism , probe_count=%d\n", __func__, probe_count);
			goto sensor_probe_retry;
		}
	}

	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ar0260_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ar0260_s_ctrl},
	{ }
};

static struct i2c_driver ar0260_i2c_driver = {
	.id_table = ar0260_i2c_id,
	.probe  = ar0260_i2c_probe,//msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int ar0260_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int i;
	uint16_t read_data = 0;
	uint16_t id_addr[2] = {0x31F6,0x31F4};
	static uint16_t id_data[2] = {0,0};
	static int first=true;
	
	struct msm_camera_i2c_reg_conf cmd[]= {
		{0x301A, 0x10D8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x3134, 0xCD95,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304C, 0x3000,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304A, 0x0200,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304A, 0x0210,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

		//{0x304A, 0x60,MSM_CAMERA_I2C_SET_WORD_MASK,MSM_CAMERA_I2C_CMD_POLL},
	};
	
	pr_info("%s called\n", __func__);


	if (!first) {
		cdata->cfg.fuse.fuse_id_word1 = 0;
		cdata->cfg.fuse.fuse_id_word2 = 0;
		cdata->cfg.fuse.fuse_id_word3 = id_data[0];
		cdata->cfg.fuse.fuse_id_word4 = id_data[1];

		pr_info("ar0260: catched fuse->fuse_id : 0x%x 0x%x 0x%x 0x%x\n", 
			cdata->cfg.fuse.fuse_id_word1,
			cdata->cfg.fuse.fuse_id_word2,
			cdata->cfg.fuse.fuse_id_word3,
			cdata->cfg.fuse.fuse_id_word4);		
		return 0;	
	}
	first=false;

	rc = msm_camera_i2c_write_tbl(s_ctrl->sensor_i2c_client, cmd, ARRAY_SIZE (cmd), 0);
	if (rc < 0) {
		pr_err("%s: msm_camera_i2c_write failed\n", __func__);
		return 0;
	}
	msleep(50);
	
	for (i = 0; i < ARRAY_SIZE(id_addr); i++) {
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, id_addr[i], &read_data, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, id_addr[i]);
			break;
		}
		id_data[i] = read_data;
	}
	
	if (i==ARRAY_SIZE(id_data)) {
		cdata->cfg.fuse.fuse_id_word1 = 0;
		cdata->cfg.fuse.fuse_id_word2 = 0;
		cdata->cfg.fuse.fuse_id_word3 = id_data[0];
		cdata->cfg.fuse.fuse_id_word4 = id_data[1];
	
		pr_info("ar0260: fuse->fuse_id : 0x%x 0x%x 0x%x 0x%x\n", 
			cdata->cfg.fuse.fuse_id_word1,
			cdata->cfg.fuse.fuse_id_word2,
			cdata->cfg.fuse.fuse_id_word3,
			cdata->cfg.fuse.fuse_id_word4);
	}
	return 0;

}
/* HTC_END*/

// HTC_START pg 20121002 aptina don't need frame lenfth lines
int32_t aptina_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) /* HTC Angie 20111019 - Fix FPS */
{
	CDBG("%s: called\n", __func__);
/* HTC_START ben 20120229 */
	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;
/* HTC_END */

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}
// HTC_END pg 20121002 aptina don't need frame lenfth lines

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&ar0260_i2c_driver);
}

static struct v4l2_subdev_core_ops ar0260_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ar0260_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ar0260_subdev_ops = {
	.core = &ar0260_subdev_core_ops,
	.video  = &ar0260_subdev_video_ops,
};

static struct msm_sensor_fn_t ar0260_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = aptina_write_exp_gain, // HTC pg 20121002 aptina don't need frame lenfth lines
	.sensor_write_snapshot_exp_gain_ex = aptina_write_exp_gain, // HTC pg 20121002 aptina don't need frame lenfth lines
	.sensor_setting = msm_sensor_setting_parallel,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,//ar0260_sensor_config,
	.sensor_power_up = ar0260_power_up,
	.sensor_power_down = ar0260_power_down,
	.sensor_i2c_read_fuseid = ar0260_read_fuseid,
};

static struct msm_sensor_reg_t ar0260_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ar0260_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ar0260_start_settings),
	.stop_stream_conf = ar0260_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ar0260_stop_settings),


	.start_stream_conf_yushanii = ar0260_start_settings_yushanii,
	.start_stream_conf_size_yushanii = ARRAY_SIZE(ar0260_start_settings_yushanii),
	.stop_stream_conf_yushanii = ar0260_stop_settings_yushanii,
	.stop_stream_conf_size_yushanii = ARRAY_SIZE(ar0260_stop_settings_yushanii),

	.group_hold_on_conf = ar0260_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ar0260_groupon_settings),
	.group_hold_off_conf = ar0260_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ar0260_groupoff_settings),
	.init_settings = &ar0260_init_conf[0],
	.init_size = ARRAY_SIZE(ar0260_init_conf),

	.init_settings_yushanii = &ar0260_init_conf_yushanii[0],
	.init_size_yushanii = ARRAY_SIZE(ar0260_init_conf_yushanii),

	.mode_settings = &ar0260_confs[0],
	.output_settings = &ar0260_dimensions[0],

	.output_settings_yushanii = &ar0260_dimensions_yushanii[0],


	.num_conf = ARRAY_SIZE(ar0260_confs),
};

static struct msm_sensor_ctrl_t ar0260_s_ctrl = {
	.msm_sensor_reg = &ar0260_regs,
	.sensor_i2c_client = &ar0260_sensor_i2c_client,
	.sensor_i2c_addr = 0x90,
	.sensor_output_reg_addr = &ar0260_reg_addr,
	.sensor_id_info = &ar0260_id_info,
	.sensor_exp_gain_info = &ar0260_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ar0260_csi_params_array[0],
	.msm_sensor_mutex = &ar0260_mut,
	.sensor_i2c_driver = &ar0260_i2c_driver,
	.sensor_v4l2_subdev_info = ar0260_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ar0260_subdev_info),
	.sensor_v4l2_subdev_ops = &ar0260_subdev_ops,
	.func_tbl = &ar0260_func_tbl,
	.sensor_first_mutex = &ar0260_sensor_init_mut,  //CC120826,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 2.1 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
