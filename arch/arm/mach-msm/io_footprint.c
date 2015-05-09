/* arch/arm/mach-msm/io_footprint.h
 * Copyright (C) 2012 HTC Corporation.
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
 */

#include <linux/atomic.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/memory_alloc.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm-generic/sizes.h>
#include <mach/memory.h>
#include <mach/system.h>
#include <mach/msm_iomap.h>

/*
PHY define in msm_iomap-8960.h, VIRT define in msm_iomap.h
The counters to check kernel exit for both cpu's
kernel foot print for cpu0  		: phy 0x8F1F1000 : virt 0xFB600000
kernel foot print for cpu1  		: phy 0x8F1F1004 : virt 0xFB600004
kernel foot print for cpu2 		: phy 0x8F1F1008 : virt 0xFB600008
kernel foot print for cpu3 		: phy 0x8F1F100C : virt 0xFB60000C
kernel exit counter from cpu0		: phy 0x8F1F1010 : virt 0xFB600010
kernel exit counter from cpu1		: phy 0x8F1F1014 : virt 0xFB600014
kernel exit counter from cpu2		: phy 0x8F1F1018 : virt 0xFB600018
kernel exit counter from cpu3		: phy 0x8F1F101C : virt 0xFB60001C
msm_pm_boot_entry       		: phy 0x8F1F1020 : virt 0xFB600020
msm_pm_boot_vector      		: phy 0x8F1F1024 : virt 0xFB600024
reset vector for cpu0(init)		: phy 0x8F1F1028 : virt 0xFB600028
reset vector for cpu1(init)     	: phy 0x8F1F102C : virt 0xFB60002C
reset vector for cpu2(init)		: phy 0x8F1F1030 : virt 0xFB600030
reset vector for cpu3(init)	        : phy 0x8F1F1034 : virt 0xFB600034
cpu0 reset vector address		: phy 0x8F1F1038 : virt 0xFB600038
cpu1 reset vector address		: phy 0x8F1F103C : virt 0xFB60003C
cpu2 reset vector address		: phy 0x8F1F1040 : virt 0xFB600040
cpu3 reset vector address		: phy 0x8F1F1044 : virt 0xFB600044
cpu0 reset vector address value		: phy 0x8F1F1048 : virt 0xFB600048
cpu1 reset vector address value		: phy 0x8F1F104C : virt 0xFB60004C
cpu2 reset vector address value		: phy 0x8F1F1050 : virt 0xFB600050
cpu3 reset vector address value		: phy 0x8F1F1054 : virt 0xFB600054
cpu0 frequency          		: phy 0x8F1F1058 : virt 0xFB600058
cpu1 frequency          		: phy 0x8F1F105C : virt 0xFB60005C
cpu2 frequency                      	: phy 0x8F1F1060 : virt 0xFB600060
cpu3 frequency                      	: phy 0x8F1F1064 : virt 0xFB600064
L2 frequency				: phy 0x8F1F1068 : virt 0xFB600068
acpuclk_set_rate footprint cpu0 	: phy 0x8F1F106C : virt 0xFB60006C
acpuclk_set_rate footprint cpu1		: phy 0x8F1F1070 : virt 0xFB600070
acpuclk_set_rate footprint cpu2		: phy 0x8F1F1074 : virt 0xFB600074
acpuclk_set_rate footprint cpu3		: phy 0x8F1F1078 : virt 0xFB600078
io footprint cpu 0			: phy 0x8F1F107C-0x8F1F1084 : virt 0xFB60007C-0xFB600084
io footprint cpu 1			: phy 0x8F1F1088-0x8F1F1090 : virt 0xFB600088-0xFB600090
io footprint cpu 2			: phy 0x8F1F1094-0x8F1F109C : virt 0xFB600094-0xFB60009C
io footprint cpu 3			: phy 0x8F1F10A0-0x8F1F10A8 : virt 0xFB6000A0-0xFB6000A8
*/

struct io_footprint {
	void *caller;
	void *data;
	unsigned done;
};
struct io_footprint *io_footprints = (struct io_footprint *)(CPU_FOOT_PRINT_BASE + 0x7C);

noinline void io_footprint_start(void *data)
{
	int cpu = raw_smp_processor_id();

	io_footprints[cpu].caller = __builtin_return_address(0);
	io_footprints[cpu].data = data;
	io_footprints[cpu].done = 0;

	mb();
}
EXPORT_SYMBOL(io_footprint_start);

inline void io_footprint_end(void)
{
	io_footprints[raw_smp_processor_id()].done = 1;

	mb();
}
EXPORT_SYMBOL(io_footprint_end);
