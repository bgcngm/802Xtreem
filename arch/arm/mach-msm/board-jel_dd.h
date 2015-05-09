/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_JEL_DD_H
#define __ARCH_ARM_MACH_MSM_BOARD_JEL_DD_H

#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>

/* Macros assume PMIC GPIOs and MPPs start at 1 */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

#define JEL_DD_LAYOUTS			{\
		{ { 0,  1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}

extern struct platform_device msm8960_device_ext_5v_vreg __devinitdata;
extern struct platform_device msm8960_device_ext_l2_vreg __devinitdata;
extern struct platform_device msm8960_device_rpm_regulator __devinitdata;

extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V           2
#define GPIO_VREG_ID_EXT_OTG_SW         3

/* EVM */
#define jel_dd_LCD_TE                   (0)
#define jel_dd_NC_GPIO_1                (1)
#define JEL_DD_GPIO_SIM_HOTSWAP		(3)
#define JEL_DD_GPIO_RAFT_UP_EN_CPU		(10)
#define JEL_DD_GPIO_NFC_NC_12		(12)
#define JEL_DD_GPIO_NFC_NC_13		(13)
#define JEL_DD_GPIO_NC_GPIO_14		(14)
#define JEL_DD_GPIO_SIM_CD			(15)
#define JEL_DD_GPIO_TP_I2C_SDA		(16)
#define JEL_DD_GPIO_TP_I2C_SCL		(17)
#define JEL_DD_GPIO_CAM_I2C_SDA		(20)
#define JEL_DD_GPIO_CAM_I2C_SCL		(21)
#define JEL_DD_GPIO_AC_I2C_SDA		(36)
#define JEL_DD_GPIO_AC_I2C_SCL		(37)
#define JEL_DD_GPIO_SENSOR_I2C_SDA		(44)
#define JEL_DD_GPIO_SENSOR_I2C_SCL		(45)
#define JEL_DD_GPIO_MAIN_CAM_ID		(47)
#define jel_dd_LCD_RSTz			(48)
#define JEL_DD_GPIO_NC_55			(55)
#define JEL_DD_GPIO_NC_69			(69)
#define JEL_DD_GPIO_MCAM_CLK_ON		(75)
#define JEL_DD_GPIO_CAM2_STANDBY		(76)
#define JEL_DD_GPIO_NC_80			(80)
#define JEL_DD_GPIO_NC_82			(82)
#define JEL_DD_GPIO_USBz_AUDIO_SW		(89)
#define JEL_DD_GPIO_NC_90			(90)
#define JEL_DD_GPIO_NC_92			(92)
#define JEL_DD_GPIO_NC_93			(93)
#define JEL_DD_GPIO_V_CAM_D1V2_EN		(95)
#define JEL_DD_GPIO_NC_96			(96)
#define JEL_DD_GPIO_NC_100			(100)
#define JEL_DD_GPIO_NC_101			(101)
#define JEL_DD_GPIO_NC_102			(102)
#define JEL_DD_GPIO_PM_MDM_INTz		(105)
#define JEL_DD_GPIO_AUDIOz_UART_SW		(107)
#define JEL_DD_GPIO_PRX_LB_SW_SEL		(111)
#define JEL_DD_GPIO_ANT_SW_SEL4		(112)
#define JEL_DD_GPIO_BC1_SW_SEL0		(113)
#define JEL_DD_GPIO_ANT_SW_SEL3		(116)
#define JEL_DD_GPIO_ANT_SW_SEL2		(117)
#define JEL_DD_GPIO_ANT_SW_SEL1		(118)
#define JEL_DD_GPIO_ANT_SW_SEL0		(119)
#define JEL_DD_GPIO_RTR0_PA_ON7_U700	(129)
#define JEL_DD_GPIO_PA_ON4_MODE		(132)
#define JEL_DD_GPIO_PA_ON1_GSMHB		(135)
#define JEL_DD_GPIO_RTR0_PA_ON0_CELL	(136)
#define JEL_DD_GPIO_G850_700_SEL		(139)
#define JEL_DD_GPIO_RTR0_GP_CLK		(144)
#define JEL_DD_GPIO_RTR0_GPRSSYNC		(145)
#define JEL_DD_GPIO_RTR0_GPDATA2		(146)
#define JEL_DD_GPIO_RTR0_GPDATA1		(147)
#define JEL_DD_GPIO_RTR0_GPDATA0		(148)
#define JEL_DD_GPIO_NC_150			(150)
#define JEL_DD_GPIO_NC_151			(151)

#define PMGPIO(x) (x)

#if 0 /* Not in use, duplicate with the following PMGPIO  */
#define JEL_DD_PMGPIO_PM_SPI_CS0		PMGPIO(5)
#define JEL_DD_PMGPIO_RAFT_uP_SPI_CS0	PMGPIO(6)
#define JEL_DD_PMGPIO_NC_8			PMGPIO(8)
#define JEL_DD_PMGPIO_PM_SPI_CLK		PMGPIO(11)
#define JEL_DD_PMGPIO_RAFT_uP_SPI_CLK	PMGPIO(12)
#define JEL_DD_PMGPIO_PM_SPI_DO		PMGPIO(13)
#define JEL_DD_PMGPIO_RAFT_uP_SPI_DO	PMGPIO(14)
#define JEL_DD_PMGPIO_RAFT_UP_EN_CPU_PM	PMGPIO(15)
#define JEL_DD_PMGPIO_RAFT_UP_EN		PMGPIO(16)
#endif
#define JEL_DD_PMGPIO_AUD_AMP_EN		PMGPIO(18)
#if 0 /* Not in use, duplicate with the following PMGPIO  */
#define JEL_DD_PMGPIO_SDC3_CDz		PMGPIO(23)
#define JEL_DD_PMGPIO_UIM1_RST		PMGPIO(27)
#define JEL_DD_PMGPIO_UIM1_CLK		PMGPIO(30)
#define JEL_DD_PMGPIO_COVER_DETz		PMGPIO(41)
#define JEL_DD_PMGPIO_AD_EN_MSM		PMGPIO(42)
#endif

/* IMC */
#define JEL_DD_GPIO_GSM_XMM_RESET		(9)
#define JEL_DD_GPIO_GSM_XMM_PWR_ONOFF	(18)
#define JEL_DD_GPIO_GSM_XMM_PWR_EN		(53)
#define JEL_DD_GPIO_GSM_XMM_AP_WAKE		(75)
#define JEL_DD_GPIO_GSM_AP_XMM_WAKE		(91)
#define JEL_DD_GPIO_GSM_XMM_AP_STATUS	(12)
#define JEL_DD_GPIO_GSM_AP_XMM_STATUS	(97)
#define JEL_DD_GPIO_GSM_UART_TX			(22)
#define JEL_DD_GPIO_GSM_UART_RX			(23)
#define JEL_DD_GPIO_GSM_UART_CTS		(24)
#define JEL_DD_GPIO_GSM_UART_RTS		(25)
#define JEL_DD_GPIO_GSM_SIM_DETECT		(81)

#define JEL_DD_GPIO_GSM_XMM_AP_FATAL	PMGPIO(9)
#define JEL_DD_GPIO_GSM_AP_XMM_FATAL	PMGPIO(10)

/* XA */
#define JEL_DD_GPIO_LCD_TE			(0) /* MDP_VSYNC_GPIO */
#define JEL_DD_GPIO_NC_1			(1)
#define JEL_DD_GPIO_NC_2			(2)
#define JEL_DD_GPIO_HAPTIC_EN		(3)
#define JEL_DD_GPIO_CAM_MCLK1		(4)
#define JEL_DD_GPIO_CAM_MCLK0		(5)
#define JEL_DD_GPIO_NFC_DL_MODE		(6)
#define JEL_DD_GPIO_TP_ATTz		(7)
#define JEL_DD_GPIO_TP_RSTz		(8)
#define JEL_DD_GPIO_NC_GPIO_9		(9)
#define JEL_DD_GPIO_SD_CDETz		(10)
#define JEL_DD_GPIO_RESOUTz_CONTROL	(11)
#define JEL_DD_GPIO_NC_12			(12)
#define JEL_DD_GPIO_NC_13			(13)
#define JEL_DD_GPIO_AUD_1WIRE_DO		(14)
#define JEL_DD_GPIO_AUD_1WIRE_DI		(15)
#define JEL_DD_GPIO_TP_I2C_DAT		(16)
#define JEL_DD_GPIO_TP_I2C_CLK		(17)
#define JEL_DD_GPIO_NC_18			(18)
#define JEL_DD_GPIO_USB_ID1		(19)
#define JEL_DD_GPIO_CAM_I2C_DAT		(20)
#define JEL_DD_GPIO_CAM_I2C_CLK		(21)
#define JEL_DD_GPIO_NC_22			(22)
#define JEL_DD_GPIO_NC_23			(23)
#define JEL_DD_GPIO_NC_24			(24)
#define JEL_DD_GPIO_NC_25			(25)
#define JEL_DD_GPIO_FM_SSBI		(26)
#define JEL_DD_GPIO_FM_DATA		(27)
#define JEL_DD_GPIO_BT_STROBE		(28)
#define JEL_DD_GPIO_BT_DATA		(29)
#define JEL_DD_GPIO_UIM1_DATA_MSM		(30)
#define JEL_DD_GPIO_UIM1_CLK_MSM		(31)
#define JEL_DD_GPIO_TORCH_FLASHz		(32)
#define JEL_DD_GPIO_DRIVER_EN		(33)
#define JEL_DD_GPIO_DEBUG_UART_TX		(34)
#define JEL_DD_GPIO_DEBUG_UART_RX		(35)
#define JEL_DD_GPIO_MC_I2C_DAT		(36)
#define JEL_DD_GPIO_MC_I2C_CLK		(37)
#define JEL_DD_GPIO_MSM_SPI_DO		(38)
#define JEL_DD_GPIO_RAW_INTR1		(39)
#define JEL_DD_GPIO_MSM_SPI_CSO		(40)
#define JEL_DD_GPIO_MSM_SPI_CLK		(41)
#define JEL_DD_GPIO_VOL_UPz		(42)
#define JEL_DD_GPIO_VOL_DOWNz		(43)
#define JEL_DD_GPIO_SR_I2C_DAT		(44)
#define JEL_DD_GPIO_SR_I2C_CLK		(45)
#define JEL_DD_GPIO_PWR_KEYz 		(46)
#define JEL_DD_GPIO_CAM_ID			(47)
#define JEL_DD_GPIO_LCD_RSTz		(48)
#define JEL_DD_GPIO_CAM_VCM_PD		(49)
#define JEL_DD_GPIO_NFC_VEN		(50)
#define JEL_DD_GPIO_RAW_RSTN		(51)
#define JEL_DD_GPIO_RAW_INTR0		(52)
#define JEL_DD_GPIO_NC_53			(53)
#define JEL_DD_GPIO_REGION_ID		(54)
#define JEL_DD_GPIO_TAM_DET_EN		(55)
#define JEL_DD_GPIO_V_3G_PA1_EN		(56)
#define JEL_DD_GPIO_V_3G_PA0_EN		(57)
#define JEL_DD_GPIO_NC_58			(58)
#define JEL_DD_GPIO_AUD_WCD_MCLK		(59)
#define JEL_DD_GPIO_AUD_WCD_SB_CLK		(60)
#define JEL_DD_GPIO_AUD_WCD_SB_DATA	(61)
#define JEL_DD_GPIO_AUD_WCD_INTR_OUT	(62)
#define JEL_DD_IMC_PCM_OUT		(63)
#define JEL_DD_IMC_PCM_IN		(64)
#define JEL_DD_IMC_PCM_SYNC		(65)
#define JEL_DD_IMC_PCM_CLK		(66)
#define JEL_DD_GPIO_GSENSOR_INT		(67)
#define JEL_DD_GPIO_CAM2_RSTz		(68)
#define JEL_DD_GPIO_GYRO_INT		(69)
#define JEL_DD_GPIO_COMPASS_INT		(70)
#define JEL_DD_GPIO_MCAM_SPI_DO		(71)
#define JEL_DD_GPIO_MCAM_SPI_DI		(72)
#define JEL_DD_GPIO_MCAM_SPI_CS0		(73)
#define JEL_DD_GPIO_MCAM_SPI_CLK		(74)
#define JEL_DD_GPIO_NC_75			(75)
#define JEL_DD_GPIO_NC_76			(76)
#define JEL_DD_GPIO_V_HAPTIC_3V3_EN	(77)
#define JEL_DD_GPIO_MHL_INT		(78)
#define JEL_DD_GPIO_V_CAM2_D1V8_EN		(79)
#define JEL_DD_GPIO_MHL_RSTz		(80)
#define JEL_DD_GPIO_V_TP_3V3_EN		(81)
#define JEL_DD_GPIO_MHL_USBz_SEL		(82)
#define JEL_DD_GPIO_WCN_BT_SSBI		(83)
#define JEL_DD_GPIO_WCN_CMD_DATA2		(84)
#define JEL_DD_GPIO_WCN_CMD_DATA1		(85)
#define JEL_DD_GPIO_WCN_CMD_DATA0		(86)
#define JEL_DD_GPIO_WCN_CMD_SET		(87)
#define JEL_DD_GPIO_WCN_CMD_CLK		(88)
#define JEL_DD_GPIO_MHL_USB_ENz		(89)
#define JEL_DD_GPIO_CAM_STEP_1		(90)
#define JEL_DD_GPIO_NC_91			(91)
#define JEL_DD_GPIO_CAM_STEP_2		(92)
#define JEL_DD_GPIO_V_HSMIC_2v85_EN	(93)
#define JEL_DD_GPIO_MBAT_IN		(94)
#define JEL_DD_GPIO_V_CAM_D1V2_EN		(95)
#define JEL_DD_GPIO_V_BOOST_5V_EN		(96)
#define JEL_DD_GPIO_NC_97			(97)
#define JEL_DD_GPIO_RIVA_TX		(98)
#define JEL_DD_GPIO_NC_99			(99)
#define JEL_DD_GPIO_HDMI_DDC_CLK		(100)
#define JEL_DD_GPIO_HDMI_DDC_DATA		(101)
#define JEL_DD_GPIO_HDMI_HPD		(102)
#define JEL_DD_GPIO_PM_SEC_INTz		(103)
#define JEL_DD_GPIO_PM_USR_INTz		(104)
#define JEL_DD_GPIO_PM_MSM_INTz		(105)
#define JEL_DD_GPIO_NFC_IRQ		(106)
#define JEL_DD_GPIO_CAM_PWDN		(107)
#define JEL_DD_GPIO_PS_HOLD		(108)
#define JEL_DD_GPIO_APT1_VCON		(109)
#define JEL_DD_GPIO_BC1_SW_SEL1		(110)
#define JEL_DD_GPIO_DRX_1X_EV_SEL		(111)
#define JEL_DD_GPIO_BOOT_CONFIG_6		(112)
#define JEL_DD_GPIO_NC_113			(113)
#define JEL_DD_GPIO_BC0_SW_SEL1		(114)
#define JEL_DD_GPIO_BC0_SW_SEL0		(115)
#define JEL_DD_GPIO_BOOT_CONFIG2		(116)
#define JEL_DD_GPIO_BOOT_CONFIG1		(117)
#define JEL_DD_GPIO_BOOT_CONFIG0		(118)
#define JEL_DD_GPIO_NC_119			(119)
#define JEL_DD_GPIO_PA1_R0			(120)
#define JEL_DD_GPIO_PA1_R1			(121)
#define JEL_DD_GPIO_PA0_R0			(122)
#define JEL_DD_GPIO_PA0_R1			(123)
#define JEL_DD_GPIO_RTR1_RF_ON		(124)
#define JEL_DD_GPIO_RTR0_RF_ON		(125)
#define JEL_DD_GPIO_RTR_RX_ON		(126)
#define JEL_DD_GPIO_APT0_VCON		(127)
#define JEL_DD_GPIO_NC_128			(128)
#define JEL_DD_GPIO_NC_129			(129)
#define JEL_DD_GPIO_NC_130			(130)
#define JEL_DD_GPIO_RTR0_PA_ON5_PCS	(131)
#define JEL_DD_GPIO_NC_132			(132)
#define JEL_DD_GPIO_RTR1_PA_ON3_BC1_1X	(133)
#define JEL_DD_GPIO_RTR1_PA_ON2_BC0_1X	(134)
#define JEL_DD_GPIO_NC_135			(135)
#define JEL_DD_GPIO_PTR0_PA_ON0_CELL	(136)
#define JEL_DD_GPIO_EXT_GPS_LNA_EN		(137)
#define JEL_DD_GPIO_NC_138			(138)
#define JEL_DD_GPIO_NC_139			(139)
#define JEL_DD_GPIO_RTR1_SSBI2		(140)
#define JEL_DD_GPIO_RTR1_SSBI1		(141)
#define JEL_DD_GPIO_RTR0_SSBI2		(142)
#define JEL_DD_GPIO_RTR0_SSBI1		(143)
#define JEL_DD_GPIO_NC_144			(144)
#define JEL_DD_GPIO_NC_145			(145)
#define JEL_DD_GPIO_NC_146			(146)
#define JEL_DD_GPIO_NC_147			(147)
#define JEL_DD_GPIO_NC_148			(148)
#define JEL_DD_GPIO_NC_149			(149)
#define JEL_DD_GPIO_HSIC_STROBE		(150)
#define JEL_DD_GPIO_HSIC_DATA		(151)

#define JEL_DD_PMGPIO_IMC_USB_ENz	PMGPIO(1)
#define JEL_DD_PMGPIO_IMC_USB_SEL	PMGPIO(2)
#define JEL_DD_PMGPIO_IMC_USB_DET	PMGPIO(3)
#define JEL_DD_PMGPIO_NC_4			PMGPIO(4)
#define JEL_DD_PMGPIO_MSM_SPI_CS0		PMGPIO(5)
#define JEL_DD_PMGPIO_HVDAC_SPI_CS0	PMGPIO(6)
#define JEL_DD_PMGPIO_AUD_1WIRE_DO		PMGPIO(7)
#define JEL_DD_PMGPIO_AUD_1WIRE_O		PMGPIO(8)
#define JEL_DD_PMGPIO_NC_9			PMGPIO(9)
#define JEL_DD_PMGPIO_NC_10		PMGPIO(10)
#define JEL_DD_PMGPIO_MSM_SPI_CLK		PMGPIO(11)
#define JEL_DD_PMGPIO_HVDAC_SPI_CLK	PMGPIO(12)
#define JEL_DD_PMGPIO_MSM_SPI_DO		PMGPIO(13)
#define JEL_DD_PMGPIO_HVDAC_SPI_DO		PMGPIO(14)
#define JEL_DD_PMGPIO_AUD_REMO_PRESz	PMGPIO(15)
#define JEL_DD_PMGPIO_CPU_1WIRE_RX	PMGPIO(16)
#define JEL_DD_PMGPIO_PROXIMITY_INTz	PMGPIO(17)
#define JEL_DD_PMGPIO_AUD_SPK_SDz		PMGPIO(18)
#define JEL_DD_PMGPIO_NC_PMGPIO_19	PMGPIO(19)
#define JEL_DD_PMGPIO_EARPHONE_DETz	PMGPIO(20)
#define JEL_DD_PMGPIO_NC_PMGPIO_21	PMGPIO(21)
#define JEL_DD_PMGPIO_NC_PMGPIO_22	PMGPIO(22)
#define JEL_DD_PMGPIO_NC_PMGPIO_23	PMGPIO(23)
#define JEL_DD_PMGPIO_NC_PMGPIO_24	PMGPIO(24)
#define JEL_DD_PMGPIO_NC_PMGPIO_25	PMGPIO(25)
#define JEL_DD_PMGPIO_HAPTIC_PWM		PMGPIO(26)
#define JEL_DD_PMGPIO_UIM_RST			PMGPIO(27)
#define JEL_DD_PMGPIO_NC_PMGPIO_28	PMGPIO(28)
#define JEL_DD_PMGPIO_SIM_CLK_MSM		PMGPIO(29)
#define JEL_DD_PMGPIO_UIM_CLK			PMGPIO(30)
#define JEL_DD_PMGPIO_NC_PMGPIO_31	PMGPIO(31)
#define JEL_DD_PMGPIO_NC_PMGPIO_32	PMGPIO(32)
#define JEL_DD_PMGPIO_LCD_ID0			PMGPIO(33)
#define JEL_DD_PMGPIO_AUD_CODEC_RSTz	PMGPIO(34)
#define JEL_DD_PMGPIO_LCD_ID1			PMGPIO(35)
#define JEL_DD_PMGPIO_NC_PMGPIO_36	PMGPIO(36)
#define JEL_DD_PMGPIO_NC_PMGPIO_37	PMGPIO(37)
#define JEL_DD_PMGPIO_NC_PMGPIO_38	PMGPIO(38)
#define JEL_DD_PMGPIO_SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define JEL_DD_PMGPIO_NC_PMGPIO_40	PMGPIO(40)
#define JEL_DD_PMGPIO_LEVEL_SHIFTER_EN	PMGPIO(40)
#define JEL_DD_PMGPIO_NC_PMGPIO_41	PMGPIO(41)
#define JEL_DD_PMGPIO_NC_PMGPIO_42	PMGPIO(42)
#define JEL_DD_PMGPIO_RAW_1V2_EN		PMGPIO(43)
#define JEL_DD_PMGPIO_RAW_1V8_EN		PMGPIO(44)

/* XB GPIO */
#define JEL_DD_GPIO_V_CAM2_D1V2_EN	(79)

#define JEL_DD_PMGPIO_NC_1			PMGPIO(1)
#define JEL_DD_PMGPIO_NC_2			PMGPIO(2)
#define jel_dd_IMC_USB_ENz	PMGPIO(1)
#define jel_dd_IMC_USB_SEL	PMGPIO(2)
#define jel_dd_IMC_USB_DET	PMGPIO(3)

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data msm_rpm_regulator_pdata __devinitdata;

void jel_dd_lcd_id_power(int pull);

#ifdef CONFIG_SERIAL_MSM_HS_IMC
void imc_pmic_init(void);
#endif

int __init jel_dd_init_keypad(void);
int __init jel_dd_gpiomux_init(void);
extern struct msm_camera_board_info jel_dd_camera_board_info;
void msm8960_init_cam(void);
#endif
