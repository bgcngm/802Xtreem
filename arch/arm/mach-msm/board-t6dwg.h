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
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_T6DWG_H
#define __ARCH_ARM_MACH_MSM_BOARD_T6DWG_H

#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8821.h>
#include <mach/msm_memtypes.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>

#define EVM	0x99
#define EVM1	99
#define XA	0
#define XB	1
#define XC	2
#define XD	3
#define PVT	0x80

#define GPIO(x) (x)
#define PMGPIO(x) (x)

int __init t6dwg_init_keypad(void);

/* Platform dependent definition */
#define LCD_TE			GPIO(0)
#define RAW_RST		GPIO(1)
#define CAM2_RSTz		GPIO(2)
#define AP2MDM_WAKEUP		GPIO(3)
#define CAM_MCLK1		GPIO(4)
#define CAM_MCLK0		GPIO(5)
#define CPU_CIR_TX		GPIO(6)
#define CPU_CIR_RX		GPIO(7)
#define TP_I2C_SDA		GPIO(8)
#define TP_I2C_SCL		GPIO(9)
#define QSC2AP_STATUS		GPIO(10)
#define VOL_UPz		GPIO(11)
#define CAM_I2C_SDA		GPIO(12)
#define CAM_I2C_SCL		GPIO(13)
#define WCSS_FM_SSBI		GPIO(14)
#define FM_DATA		GPIO(15)
#define BT_CTL  		GPIO(16)
#define BT_DATA		GPIO(17)
#define AP2QSC_UART_TX		GPIO(18)
#define AP2QSC_UART_RX		GPIO(19)
#define AP2QSC_UART_CTS	GPIO(20)
#define AP2QSC_UART_RFR	GPIO(21)
#define CPU_1WIRE_TX		GPIO(22)
#define CPU_1WIRE_RX		GPIO(23)
#define SR_I2C_SDA		GPIO(24)
#define SR_I2C_SCL		GPIO(25)
#define PWR_KEY_MSMz		GPIO(26)
#define AUD_CPU_RX_I2S_WS	GPIO(27)
#define AUD_CPU_RX_I2S_SCK	GPIO(28)
#define AUD_TFA_DO_A		GPIO(29)
#define AP2MDM_VDDMIN		GPIO(30)
#define FP_SPI_CS0			GPIO(31)
#define AUD_CPU_RX_I2S_SD1	GPIO(32)
#define RAW_INT1		GPIO(33)
#define TP_ATTz 		GPIO(34)
#define MDM2AP_HSIC_READY	GPIO(35)
#define MDM2AP_ERR_FATAL	GPIO(36)
#define AP2MDM_ERR_FATAL	GPIO(37)
#define MHL_INT		GPIO(38)
#define AUD_CPU_MCLK		GPIO(39)
#define AUD_CPU_SB_CLK		GPIO(40)
#define AUD_CPU_SB_DATA	GPIO(41)
#define AUD_WCD_INTR_OUT	GPIO(42)
#define AP2QSC_WAKEUP		GPIO(43)
#define AP2QSC_STATUS		GPIO(44)
#define QSC2APQ_VDDMIN		GPIO(45)
#define AP2QSC_VDDMIN		GPIO(46)
#define MDM2AP_WAKEUP		GPIO(47)
#define AP2MDM_STATUS		GPIO(48)
#define MDM2AP_STATUS		GPIO(49)
#define QSC2AP_ERR_FATAL	GPIO(50)
#define MCAM_FP_SPI_DO		GPIO(51)
#define MCAM_FP_SPI_DI		GPIO(52)
#define MCAM_SPI_CS0		GPIO(53)
#define MCAM_FP_SPI_CLK	GPIO(54)
#define FP_INT				GPIO(55)
#define NFC_IRQ		GPIO(56)
#define LCD_RST		GPIO(57)
#define WCN_PRIORITY		GPIO(58)
#define AP2MDM_PON_RESET_N	GPIO(59)
#define MDM_LTE_FRAME_SYNC	GPIO(60)
#define MDM_LTE_ACTIVE		GPIO(61)
#define PWR_MISTOUCH		GPIO(62)
#define WCSS_BT_SSBI		GPIO(63)
#define WCSS_WLAN_DATA_2	GPIO(64)
#define WCSS_WLAN_DATA_1	GPIO(65)
#define WCSS_WLAN_DATA_0	GPIO(66)
#define WCSS_WLAN_SET		GPIO(67)
#define WCSS_WLAN_CLK		GPIO(68)
#define APQ2MDM_IPC3		GPIO(69)
#define HDMI_DDC_CLK		GPIO(70)
#define HDMI_DDC_DATA		GPIO(71)
#define HDMI_HPLG_DET		GPIO(72)
#define PM8921_APC_SEC_IRQ_N	GPIO(73)
#define PM8921_APC_USR_IRQ_N	GPIO(74)
#define PM8921_MDM_IRQ_N	GPIO(75)
#define PM8821_APC_SEC_IRQ_N	GPIO(76)
#define VOL_DOWNz		GPIO(77)
#define PS_HOLD_APQ		GPIO(78)
#define SSBI_PM8821		GPIO(79)
#define MDM2AP_VDDMIN		GPIO(80)
#define APQ2MDM_IPC1		GPIO(81)
#define UART_TX		GPIO(82)
#define UART_RX		GPIO(83)
#define AUD_I2C_SDA		GPIO(84)
#define AUD_I2C_SCL		GPIO(85)
#define AP2QSC_ERR_FATAL	GPIO(86)
#define RAW_INT0		GPIO(87)
#define HSIC_STROBE		GPIO(88)
#define HSIC_DATA		GPIO(89)

#define CAM_VCM_PD		PMGPIO(1)
#define EXT_BUCK_EN		PMGPIO(2)
#define GYRO_INT		PMGPIO(3)
#define CIR_LS_EN		PMGPIO(4)
#define CAM2_ID		PMGPIO(5)
#define COMPASS_AKM_INT	PMGPIO(6)
#define USB1_HS_ID_GPIO	PMGPIO(7)
#define AP2QSC_PWR_EN		PMGPIO(8)
#define V_AUD_HSMIC_2V85_EN	PMGPIO(9)
#define EXT_BUCK_VSEL		PMGPIO(10)
#define UART_QSCUSB_SEL	PMGPIO(11)
#define AUD_DMIC1_SEL		PMGPIO(12)
#define AUD_DMIC2_SEL		PMGPIO(13)
#define UART_QSCUSB_EN		PMGPIO(14)
#define MHL_USB_SW		PMGPIO(15)
#define AP2QSC_KPDPWR		PMGPIO(16)
#define PROXIMITY_INT		PMGPIO(17)
#define TORCH_FLASHz		PMGPIO(18)
#define FLASH_EN		PMGPIO(19)
#define EARPHONE_DETz		PMGPIO(20)
#define MHL_RSTz			PMGPIO(21)
#define MAIN_CAM_ID		PMGPIO(22)
#define G_INT			PMGPIO(23)
#define AUD_RECEIVER_SEL	PMGPIO(24)
#define MCAM_D1V2_EN		PMGPIO(25)
#define AP2QSC_SOFT_RESET	PMGPIO(26)
#define POGO_ID				PMGPIO(27)
#define OVP_INT_DECT		PMGPIO(28)
#define NFC_DL_MODE		PMGPIO(29)
#define NFC_VEN		PMGPIO(30)
#define V_CIR_3V_EN		PMGPIO(31)
#define APQ2MDM_IPC2_XB		PMGPIO(31)
#define IO_EXT_RSTz			PMGPIO(32)
#define AUD_RECEIVER_EN	PMGPIO(33)
#define AUD_WCD_RESET_N	PMGPIO(34)
#define CIR_RST		PMGPIO(35)
#define V_RAW_1V2_EN		PMGPIO(36)
#define AUD_HP_EN		PM8921_GPIO_PM_TO_SYS(37)
#define SDC3_CD		PMGPIO(38)
#define SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define REGION_ID		PMGPIO(40)
#define AUD_UART_OEz		PMGPIO(41)
#define CAM_PWDN		PMGPIO(42)
#define TP_RSTz		PMGPIO(43)
#define LCD_ID1		PMGPIO(44)

/* Macros assume PMIC GPIOs and MPPs start at 1 */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8821_MPP_BASE			(PM8921_MPP_BASE + PM8921_NR_MPPS)
#define PM8821_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8821_MPP_BASE)
#define PM8821_IRQ_BASE			(PM8921_IRQ_BASE + PM8921_NR_IRQS)

#ifdef CONFIG_RESET_BY_CABLE_IN
#define AC_WDT_EN		GPIO(3)
#define AC_WDT_RST		GPIO(87)
#endif

#define PM2QSC_SOFT_RESET	PM8921_GPIO_PM_TO_SYS(26)
#define PM2QSC_PWR_EN		PM8921_GPIO_PM_TO_SYS(8)
#define PM2QSC_KEYPADPWR	PM8921_GPIO_PM_TO_SYS(16)

/* extra gpio for QSC */
#define AP2QSC_IPC1_XB_7	PM8921_MPP_PM_TO_SYS(7)

/* extra gpio for 1080p panel */
#define BL_HW_EN_MPP_8		PM8921_MPP_PM_TO_SYS(8)
#define LCM_N5V_EN_MPP_9	PM8921_MPP_PM_TO_SYS(9)
#define LCM_P5V_EN_MPP_10	PM8921_MPP_PM_TO_SYS(10)

extern struct pm8xxx_regulator_platform_data
	t6china_pm8921_regulator_pdata[] __devinitdata;

extern int t6china_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_3P3V		1
#define GPIO_VREG_ID_EXT_TS_SW		2
#define GPIO_VREG_ID_EXT_MPP8		3

extern struct gpio_regulator_platform_data
	t6china_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	t6china_rpm_regulator_pdata __devinitdata;

extern struct regulator_init_data t6china_saw_regulator_pdata_8921_s5;
extern struct regulator_init_data t6china_saw_regulator_pdata_8921_s6;
extern struct regulator_init_data t6china_saw_regulator_pdata_8821_s0;
extern struct regulator_init_data t6china_saw_regulator_pdata_8821_s1;

struct mmc_platform_data;
int __init apq8064_add_sdcc(unsigned int controller,
		struct mmc_platform_data *plat);

void t6china_init_mmc(void);
int t6china_wifi_init(void);
void t6china_init_gpiomux(void);
void t6dwg_init_pmic(void);
/* HTC_START - for HW VCM work-around */
void t6dwg_init_pmic_register_cam_cb(void *cam_vcm_on_cb, void *cam_vcm_off_cb);
/* HTC_END */

#if 1	// for pre-evt no camera
extern struct platform_device t6china_msm_rawchip_device;
#endif
void t6china_init_cam(void);



#define APQ_8064_GSBI1_QUP_I2C_BUS_ID 0
#define APQ_8064_GSBI2_QUP_I2C_BUS_ID 2
#define APQ_8064_GSBI3_QUP_I2C_BUS_ID 3
#define APQ_8064_GSBI4_QUP_I2C_BUS_ID 4
#define APQ_8064_GSBI7_QUP_I2C_BUS_ID 7

void t6china_init_fb(void);
void t6china_allocate_fb_region(void);
void t6china_mdp_writeback(struct memtype_reserve *reserve_table);

void t6china_init_gpu(void);
void t6dwg_pm8xxx_gpio_mpp_init(void);
void t6china_usb_uart_switch(int nvbus);

#ifdef CONFIG_RESET_BY_CABLE_IN
void reset_dflipflop(void);
#endif

#endif
