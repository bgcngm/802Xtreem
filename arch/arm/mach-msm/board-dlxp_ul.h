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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_M7_H
#define __ARCH_ARM_MACH_MSM_BOARD_M7_H

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

int __init dlxp_ul_init_keypad(void);

/* Platform dependent definition */
/* Platform dependent definition */
/* Platform dependent definition */

/* Not reviewed GPIO */
#define LCD_TE			GPIO(0)
#define RAW_RSTz		GPIO(1)
#define CAM2_RSTz		GPIO(2)
#define NC_GPIO3		GPIO(3)
#define NC_GPIO4		GPIO(4)
#define RAW_MCLK		GPIO(5)
#define CIR_UART_TX		GPIO(6)
#define CIR_UART_RX		GPIO(7)
#define I2C3_DATA_TS		GPIO(8)
#define I2C3_CLK_TS		GPIO(9)
#define NFC_IRQ			GPIO(10)
#define VOL_UPz			GPIO(11)
#define I2C4_DATA_CAM		GPIO(12)
#define I2C4_CLK_CAM		GPIO(13)
#define BT_UART_TX		GPIO(14)
#define BT_UART_RX		GPIO(15)
#define BT_UART_CTSz		GPIO(16)
#define BT_UART_RTSz		GPIO(17)
#define AP2MDM_ERR_FATAL	GPIO(18)
#define MDM2AP_ERR_FATAL	GPIO(19)
#define I2C1_DATA_APPS		GPIO(20)
#define I2C1_CLK_APPS		GPIO(21)
#define CPU_1WIRE_TX		GPIO(22)
#define CPU_1WIRE_RX		GPIO(23)
#define I2C2_DATA_SENS		GPIO(24)
#define I2C2_CLK_SENS		GPIO(25)
#define PWR_KEY_MSMz		GPIO(26)
#define WS			GPIO(27)
#define SCLK			GPIO(28)
#define DOUT			GPIO(29)
#define AP2MDM_VDDMIN		GPIO(30)
#define LCD_RSTz		GPIO(31)
#define DIIN			GPIO(32)
#define HAPTIC_3V3_EN		GPIO(33)
#define TP_ATTz			GPIO(34)
#define AUD_FM_I2S_BCLK		GPIO(35)
#define AUD_FM_I2S_SYNC		GPIO(36)
#define AUD_FM_I2S_DIN		GPIO(37)
#define MHL_INT			GPIO(38)
#define AUD_CPU_MCLK		GPIO(39)
#define SLIMBUS1_CLK		GPIO(40)
#define SLIMBUS1_DATA		GPIO(41)
#define AUD_WCD_INTR_OUT	GPIO(42)
#define AUD_BTPCM_DIN		GPIO(43)
#define AUD_BTPCM_DOUT		GPIO(44)
#define AUD_BTPCM_SYNC		GPIO(45)
#define AUD_BTPCM_CLK		GPIO(46)
#define AP2MDM_SOFT_RESET	GPIO(47)
#define AP2MDM_STATUS		GPIO(48)
#define MDM2AP_STATUS		GPIO(49)
#define NC_GPIO50		GPIO(50)
#define MCAM_SPI_DO		GPIO(51)
#define MCAM_SPI_DI		GPIO(52)
#define MCAM_SPI_CS0		GPIO(53)
#define MCAM_SPI_CLK		GPIO(54)
#define RAW_INTR0		GPIO(55)
#define RAW_INTR1		GPIO(56)
#define AP2MDM_WAKEUP		GPIO(57)
#define WCN_PRIORITY		GPIO(58)
#define AP2MDM_PON_RESET_N	GPIO(59)
#define MDM_LTE_FRAME_SYNC	GPIO(60)
#define MDM_LTE_ACTIVE		GPIO(61)
#define GSENSOR_INT		GPIO(62)
#define MHL_RSTz_XC			GPIO(62)
#define SDC4_DATA3_CPU		GPIO(63)
#define SDC4_DATA2_CPU		GPIO(64)
#define SDC4_DATA1_CPU		GPIO(65)
#define SDC4_DATA0_CPU		GPIO(66)
#define SDC4_CMD_CPU		GPIO(67)
#define SDC4_CLK_CPU		GPIO(68)
#define NFC_VEN			GPIO(69)
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
#define UART_TX			GPIO(82)
#define UART_RX			GPIO(83)
#define MDM2AP_READY		GPIO(84)
#define TP_RSTz			GPIO(85)
#define BL_HW_EN		GPIO(86)
#define RESET_EN_CLRz		GPIO(87)
#define HSIC_STROBE		GPIO(88)
#define HSIC_DATA		GPIO(89)

#define CAM_VCM_PD		PMGPIO(1)
#define MHL_RSTz		PMGPIO(2)
#define EXT_BUCK_EN_XC          	PMGPIO(2)
#define GYRO_INT		PMGPIO(3)
#define CIR_LS_EN		PMGPIO(4)
#define V_CIR_3V_EN		PMGPIO(5)
#define COMPASS_AKM_INT		PMGPIO(6)
#define USB1_HS_ID_GPIO		PMGPIO(7)
#define BT_REG_ON		PMGPIO(8)
#define V_AUD_HSMIC_2V85_EN	PMGPIO(9)
#define AUD_HP_EN		PMGPIO(10)
#define EXT_BUCK_VSEL_XC        	PMGPIO(10)
#define HAPTIC_EN		PMGPIO(11)
#define CAM2_ID			PMGPIO(12)
#define V_TP_3V3_EN		PMGPIO(13)
#define uSD_CD			PMGPIO(14)
#define GSENSOR_INT_XC          	PMGPIO(14)
#define USBz_AUDIO_SW		PMGPIO(15)
#define WL_REG_ON		PMGPIO(16)
#define PROXIMITY_INTz		PMGPIO(17)
#define TORCH_FLASHz		PMGPIO(18)
#define DRIVER_EN		PMGPIO(19)
#define EARPHONE_DETz		PMGPIO(20)
#define AUD_HP_EN_XC            	PMGPIO(20)
#define LCD_ID0			PMGPIO(21)
#define MAIN_CAM_ID		PMGPIO(22)
#define BT_WAKE			PMGPIO(23)
#define AUD_RECEIVER_SEL	PMGPIO(24)
#define AUD_RECEIVER_EN		PMGPIO(25)
#define HAPTIC_PWM		PMGPIO(26)
#define MCAM_D1V2_EN		PMGPIO(27)
#define EARPHONE_DETz_XC        	PMGPIO(27)
#define CABLE_IN		PMGPIO(28)
#define NC_PMGPIO_29		PMGPIO(29)
#define NFC_DL_MODE		PMGPIO(30)
#define V_CAM_D1V2_EN		PMGPIO(31)
#define NC_PMGPIO_32		PMGPIO(32)
#define uSD_CD_XC               	PMGPIO(32)
#define BT_HOST_WAKE		PMGPIO(33)
#define AUD_WCD_RESET_N		PMGPIO(34)
#define CIR_RSTz		PMGPIO(35)
#define V_LCM_N5V_EN		PMGPIO(36)
#define V_LCM_P5V_EN		PMGPIO(37)
#define WL_HOST_WAKE		PMGPIO(38)
#define SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define REGION_ID		PMGPIO(40)
#define AUD_UART_OEz		PMGPIO(41)
#define CAM1_PWDN		PMGPIO(42)
#define BCM4335_SLEEP_CLK	PMGPIO(43)
#define AUD_DMIC2_SEL		PMGPIO(44)

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

extern struct pm8xxx_regulator_platform_data
	dlxp_ul_pm8921_regulator_pdata[] __devinitdata;

extern int dlxp_ul_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_3P3V		1
#define GPIO_VREG_ID_EXT_TS_SW		2
#define GPIO_VREG_ID_EXT_MPP8		3

extern struct gpio_regulator_platform_data
	dlxp_ul_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	dlxp_ul_rpm_regulator_pdata __devinitdata;

extern struct regulator_init_data dlxp_ul_saw_regulator_pdata_8921_s5;
extern struct regulator_init_data dlxp_ul_saw_regulator_pdata_8921_s6;
extern struct regulator_init_data dlxp_ul_saw_regulator_pdata_8821_s0;
extern struct regulator_init_data dlxp_ul_saw_regulator_pdata_8821_s1;

struct mmc_platform_data;
int __init apq8064_add_sdcc(unsigned int controller,
		struct mmc_platform_data *plat);

void dlxp_ul_init_mmc(void);
int dlxp_ul_wifi_init(void);
void dlxp_ul_init_gpiomux(void);
void dlxp_ul_init_pmic(void);

#if 1	// for pre-evt no camera
extern struct platform_device dlxp_ul_msm_rawchip_device;
#endif
void dlxp_ul_init_cam(void);

#define APQ_8064_GSBI1_QUP_I2C_BUS_ID 0
#define APQ_8064_GSBI3_QUP_I2C_BUS_ID 3
#define APQ_8064_GSBI4_QUP_I2C_BUS_ID 4

void dlxp_ul_init_fb(void);
void dlxp_ul_allocate_fb_region(void);
void dlxp_ul_mdp_writeback(struct memtype_reserve *reserve_table);

void dlxp_ul_init_gpu(void);
void dlxp_ul_pm8xxx_gpio_mpp_init(void);
void dlxp_ul_usb_uart_switch(int nvbus);

#ifdef CONFIG_RESET_BY_CABLE_IN
void reset_dflipflop(void);
#endif

#endif
