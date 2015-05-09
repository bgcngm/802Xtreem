/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2011, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * The MSM peripherals are spread all over across 768MB of physical
 * space, which makes just having a simple IO_ADDRESS macro to slide
 * them into the right virtual location rough.  Instead, we will
 * provide a master phys->virt mapping for peripherals here.
 *
 */

#ifndef __ASM_ARCH_MSM_IOMAP_8X60_H
#define __ASM_ARCH_MSM_IOMAP_8X60_H

/* Physical base address and size of peripherals.
 * Ordered by the virtual base addresses they will be mapped at.
 *
 * MSM_VIC_BASE must be an value that can be loaded via a "mov"
 * instruction, otherwise entry-macro.S will not compile.
 *
 * If you add or remove entries here, you'll want to edit the
 * msm_io_desc array in arch/arm/mach-msm/io.c to reflect your
 * changes.
 *
 */

#define MSM_QGIC_DIST_BASE	IOMEM(0xFE000000)
#define MSM_QGIC_DIST_PHYS	0x02080000
#define MSM_QGIC_DIST_SIZE	SZ_4K

#define MSM_QGIC_CPU_BASE	IOMEM(0xFE001000)
#define MSM_QGIC_CPU_PHYS	0x02081000
#define MSM_QGIC_CPU_SIZE	SZ_4K

#define MSM_ACC_BASE		IOMEM(0xFE002000)
#define MSM_ACC_PHYS		0x02001000
#define MSM_ACC_SIZE		SZ_4K

#define MSM_GCC_BASE		IOMEM(0xFE003000)
#define MSM_GCC_PHYS		0x02082000
#define MSM_GCC_SIZE		SZ_4K

#define MSM_TLMM_BASE		IOMEM(0xFE004000)
#define MSM_TLMM_PHYS		0x00800000
#define MSM_TLMM_SIZE		SZ_16K

#define MSM_RPM_BASE		IOMEM(0xFE008000)
#define MSM_RPM_PHYS		0x00104000
#define MSM_RPM_SIZE		SZ_16K

#define MSM_CLK_CTL_BASE	IOMEM(0xFE010000)
#define MSM_CLK_CTL_PHYS	0x00900000
#define MSM_CLK_CTL_SIZE	SZ_16K

#define MSM_MMSS_CLK_CTL_BASE	IOMEM(0xFE014000)
#define MSM_MMSS_CLK_CTL_PHYS	0x04000000
#define MSM_MMSS_CLK_CTL_SIZE	SZ_4K

#define MSM_LPASS_CLK_CTL_BASE	IOMEM(0xFE015000)
#define MSM_LPASS_CLK_CTL_PHYS	0x28000000
#define MSM_LPASS_CLK_CTL_SIZE	SZ_4K

#define MSM_TMR_BASE		IOMEM(0xFE016000)
#define MSM_TMR_PHYS		0x02000000
#define MSM_TMR_SIZE		SZ_4K

#define MSM_TMR0_BASE		IOMEM(0xFE017000)
#define MSM_TMR0_PHYS		0x02040000
#define MSM_TMR0_SIZE		SZ_4K

#define MSM_SCPLL_BASE		IOMEM(0xFE018000)
#define MSM_SCPLL_PHYS		0x00903000
#define MSM_SCPLL_SIZE		SZ_1K

#define MSM_SHARED_RAM_BASE	IOMEM(0xFE200000)
#define MSM_SHARED_RAM_PHYS	0x40000000
#define MSM_SHARED_RAM_SIZE	SZ_1M

#define MSM_ACC0_BASE           IOMEM(0xFE300000)
#define MSM_ACC0_PHYS           0x02041000
#define MSM_ACC0_SIZE           SZ_4K

#define MSM_ACC1_BASE           IOMEM(0xFE301000)
#define MSM_ACC1_PHYS           0x02051000
#define MSM_ACC1_SIZE           SZ_4K

#define MSM_RPM_MPM_BASE        IOMEM(0xFE302000)
#define MSM_RPM_MPM_PHYS        0x00200000
#define MSM_RPM_MPM_SIZE        SZ_4K

#define MSM_SAW0_BASE		IOMEM(0xFE303000)
#define MSM_SAW0_PHYS		0x02042000
#define MSM_SAW0_SIZE		SZ_4K

#define MSM_SAW1_BASE		IOMEM(0xFE304000)
#define MSM_SAW1_PHYS		0x02052000
#define MSM_SAW1_SIZE		SZ_4K

#define MSM_IMEM_BASE		IOMEM(0xFE305000)
#define MSM_IMEM_PHYS		0x2A05F000
#define MSM_IMEM_SIZE		SZ_4K

#define MSM_SIC_NON_SECURE_BASE	IOMEM(0xFE600000)
#define MSM_SIC_NON_SECURE_PHYS	0x12100000
#define MSM_SIC_NON_SECURE_SIZE	SZ_64K

#define MSM_QFPROM_BASE	IOMEM(0xFE700000)
#define MSM_QFPROM_PHYS	0x00700000
#define MSM_QFPROM_SIZE	SZ_4K

#define MSM_TCSR_BASE	IOMEM(0xFE701000)
#define MSM_TCSR_PHYS	0x16B00000
#define MSM_TCSR_SIZE	SZ_4K

#define MSM_HDMI_BASE		IOMEM(0xFE800000)
#define MSM_HDMI_PHYS		0x04A00000
#define MSM_HDMI_SIZE		SZ_4K

#define MSM_EBI1_CH0_BASE	IOMEM(0xFE900000)
#define MSM_EBI1_CH0_PHYS	0x00a00000
#define MSM_EBI1_CH0_SIZE	0x00100000

#ifdef CONFIG_DEBUG_MSM8660_UART
#define MSM_DEBUG_UART_BASE	0xFEC40000
#define MSM_DEBUG_UART_PHYS	0x19C40000
#endif

#ifndef __ASSEMBLY__
extern void msm_map_msm8x60_io(void);
#endif

#define MSM_HTC_RAM_CONSOLE_PHYS	0x40300000
#define MSM_HTC_RAM_CONSOLE_SIZE	(SZ_1M - SZ_128K) /* 128K for debug info */

#define MSM_HTC_DEBUG_INFO_PHYS		(MSM_HTC_RAM_CONSOLE_PHYS + MSM_HTC_RAM_CONSOLE_SIZE)	/* 0x403E0000 */
#define MSM_HTC_DEBUG_INFO_SIZE		SZ_128K

#define MSM_TZLOG_PHYS			MSM_HTC_DEBUG_INFO_PHYS	/* 0x403E0000 */
#define MSM_TZLOG_SIZE			SZ_64K
#define MSM_WDT_PROC_INFO_SIZE		SZ_4K
#define MSM_TZ_DOG_BARK_REG_SAVE_PHYS	(MSM_HTC_DEBUG_INFO_PHYS + MSM_TZLOG_SIZE + MSM_WDT_PROC_INFO_SIZE + SZ_4K)	/* 0x403F2000 */
#define MSM_TZ_DOG_BARK_REG_SAVE_SIZE	SZ_4K

#define MSM_KERNEL_FOOTPRINT_VIRT	(0xFEA00000)
#define MSM_KERNEL_FOOTPRINT_BASE	IOMEM(MSM_KERNEL_FOOTPRINT_VIRT)	/* 4K */
#define MSM_KERNEL_FOOTPRINT_PHYS	(MSM_TZ_DOG_BARK_REG_SAVE_PHYS + MSM_TZ_DOG_BARK_REG_SAVE_SIZE) /*0x403F3000*/
#define MSM_KERNEL_FOOTPRINT_SIZE	SZ_4K

/* MSM_KERNEL_FOOTPRINT_BASE usage start */
#define CPU_FOOT_PRINT_BASE		(MSM_KERNEL_FOOTPRINT_BASE + 0x0)	/* Size: 0xAC */
#define APPS_WDOG_FOOT_PRINT_BASE	(MSM_KERNEL_FOOTPRINT_BASE + 0x100)	/* Size: 0x8 */
#define RTB_FOOT_PRINT_BASE     	(MSM_KERNEL_FOOTPRINT_BASE + 0x200)	/* Size: 0x14 */
/* MSM_KERNEL_FOOTPRINT_BASE usage done */

#define MSM_KALLSYMS_SAVE_BASE		IOMEM(0xFEA01000)	/*48K*/
#define MSM_KALLSYMS_SAVE_PHYS		(MSM_KERNEL_FOOTPRINT_PHYS + MSM_KERNEL_FOOTPRINT_SIZE) /* 0x403F4000 */
#define MSM_KALLSYMS_SAVE_SIZE		(SZ_64K - SZ_16K)

#define MSM_RTB_PHYS   0x7FF00000
#define MSM_RTB_BUFFER_SIZE    (SZ_1M)

#endif
