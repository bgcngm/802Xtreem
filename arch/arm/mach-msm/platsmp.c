/*
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *  Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <asm/hardware/gic.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/mach-types.h>
#include <asm/smp_plat.h>

#include <mach/socinfo.h>
#include <mach/hardware.h>
#include <mach/msm_iomap.h>

#include "pm.h"
#include "scm-boot.h"
#include "spm.h"

#define VDD_SC1_ARRAY_CLAMP_GFS_CTL 0x15A0
#define SCSS_CPU1CORE_RESET 0xD80
#define SCSS_DBG_STATUS_CORE_PWRDUP 0xE64

extern void msm_secondary_startup(void);

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
cpu2 reset vector address       	: phy 0x8F1F1040 : virt 0xFB600040
cpu3 reset vector address	        : phy 0x8F1F1044 : virt 0xFB600044
cpu0 reset vector address value 	: phy 0x8F1F1048 : virt 0xFB600048
cpu1 reset vector address value		: phy 0x8F1F104C : virt 0xFB60004C
cpu2 reset vector address value	   	: phy 0x8F1F1050 : virt 0xFB600050
cpu3 reset vector address value	        : phy 0x8F1F1054 : virt 0xFB600054
cpu0 frequency          		: phy 0x8F1F1058 : virt 0xFB600058
cpu1 frequency          		: phy 0x8F1F105C : virt 0xFB60005C
cpu2 frequency                          : phy 0x8F1F1060 : virt 0xFB600060
cpu3 frequency                          : phy 0x8F1F1064 : virt 0xFB600064
L2 frequency   		           	: phy 0x8F1F1068 : virt 0xFB600068
acpuclk_set_rate footprint cpu0         : phy 0x8F1F106C : virt 0xFB60006C
acpuclk_set_rate footprint cpu1         : phy 0x8F1F1070 : virt 0xFB600070
acpuclk_set_rate footprint cpu2         : phy 0x8F1F1074 : virt 0xFB600074
acpuclk_set_rate footprint cpu3         : phy 0x8F1F1078 : virt 0xFB600078
io footprint cpu 0			: phy 0x8F1F107C-0x8F1F1084 : virt 0xFB60007C-0xFB600084
io footprint cpu 1			: phy 0x8F1F1088-0x8F1F1090 : virt 0xFB600088-0xFB600090
io footprint cpu 2			: phy 0x8F1F1094-0x8F1F109C : virt 0xFB600094-0xFB60009C
io footprint cpu 3			: phy 0x8F1F10A0-0x8F1F10A8 : virt 0xFB6000A0-0xFB6000A8
*/
#define CPU0_EXIT_KERNEL_COUNTER_BASE			(CPU_FOOT_PRINT_BASE + 0x10)
#define CPU1_EXIT_KERNEL_COUNTER_BASE			(CPU_FOOT_PRINT_BASE + 0x14)
#define CPU2_EXIT_KERNEL_COUNTER_BASE			(CPU_FOOT_PRINT_BASE + 0x18)
#define CPU3_EXIT_KERNEL_COUNTER_BASE			(CPU_FOOT_PRINT_BASE + 0x1C)
static void init_cpu_debug_counter_for_cold_boot(void)
{
	static volatile bool is_cpu_debug_cnt_init = false;
	if (!is_cpu_debug_cnt_init)
	{
		*(unsigned *)CPU0_EXIT_KERNEL_COUNTER_BASE = 0x0;
		*(unsigned *)CPU1_EXIT_KERNEL_COUNTER_BASE = 0x0;
		*(unsigned *)CPU2_EXIT_KERNEL_COUNTER_BASE = 0x0;
		*(unsigned *)CPU3_EXIT_KERNEL_COUNTER_BASE = 0x0;
		is_cpu_debug_cnt_init = true;
		mb();
	}
}

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen".
 */
volatile int pen_release = -1;

static DEFINE_SPINLOCK(boot_lock);

void __cpuinit platform_secondary_init(unsigned int cpu)
{
	WARN_ON(msm_platform_secondary_init(cpu));

	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int __cpuinit scorpion_release_secondary(void)
{
	void *base_ptr = ioremap_nocache(0x00902000, SZ_4K*2);
	if (!base_ptr)
		return -EINVAL;

	writel_relaxed(0, base_ptr + VDD_SC1_ARRAY_CLAMP_GFS_CTL);
	dmb();
	writel_relaxed(0, base_ptr + SCSS_CPU1CORE_RESET);
	writel_relaxed(3, base_ptr + SCSS_DBG_STATUS_CORE_PWRDUP);
	mb();
	iounmap(base_ptr);

	return 0;
}

static int __cpuinit krait_release_secondary_sim(unsigned long base, int cpu)
{
	void *base_ptr = ioremap_nocache(base + (cpu * 0x10000), SZ_4K);
	if (!base_ptr)
		return -ENODEV;

	if (machine_is_msm8960_sim() || machine_is_msm8960_rumi3()) {
		writel_relaxed(0x10, base_ptr+0x04);
		writel_relaxed(0x80, base_ptr+0x04);
	}

	if (machine_is_apq8064_sim())
		writel_relaxed(0xf0000, base_ptr+0x04);

	if (machine_is_msm8974_sim()) {
		writel_relaxed(0x800, base_ptr+0x04);
		writel_relaxed(0x3FFF, base_ptr+0x14);
	}

	mb();
	iounmap(base_ptr);
	return 0;
}

static int __cpuinit krait_release_secondary(unsigned long base, int cpu)
{
	void *base_ptr = ioremap_nocache(base + (cpu * 0x10000), SZ_4K);
	if (!base_ptr)
		return -ENODEV;

	msm_spm_turn_on_cpu_rail(cpu);

	writel_relaxed(0x109, base_ptr+0x04);
	writel_relaxed(0x101, base_ptr+0x04);
	mb();
	ndelay(300);

	writel_relaxed(0x121, base_ptr+0x04);
	mb();
	udelay(2);

#ifdef CONFIG_APQ8064_ONLY
	writel_relaxed(0x120, base_ptr+0x04);
#else
	writel_relaxed(0x020, base_ptr+0x04);
#endif
	mb();
	udelay(2);

#ifdef CONFIG_APQ8064_ONLY
	writel_relaxed(0x100, base_ptr+0x04);
#else
	writel_relaxed(0x000, base_ptr+0x04);
#endif
	mb();
	udelay(100);

#ifdef CONFIG_APQ8064_ONLY
	writel_relaxed(0x180, base_ptr+0x04);
#else
	writel_relaxed(0x080, base_ptr+0x04);
#endif
	mb();
	iounmap(base_ptr);
	return 0;
}

static int __cpuinit release_secondary(unsigned int cpu)
{
	BUG_ON(cpu >= get_core_count());

	if (cpu_is_msm8x60())
		return scorpion_release_secondary();

	if (machine_is_msm8960_sim() || machine_is_msm8960_rumi3() ||
	    machine_is_apq8064_sim())
		return krait_release_secondary_sim(0x02088000, cpu);

	if (machine_is_msm8974_sim())
		return krait_release_secondary_sim(0xf9088000, cpu);

	if (cpu_is_msm8960() || cpu_is_msm8930() || cpu_is_msm8930aa() ||
	    cpu_is_apq8064() || cpu_is_msm8627() || cpu_is_apq8064ab())
		return krait_release_secondary(0x02088000, cpu);

	WARN(1, "unknown CPU case in release_secondary\n");
	return -EINVAL;
}

DEFINE_PER_CPU(int, cold_boot_done);
static int cold_boot_flags[] = {
	0,
	SCM_FLAG_COLDBOOT_CPU1,
	SCM_FLAG_COLDBOOT_CPU2,
	SCM_FLAG_COLDBOOT_CPU3,
};

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	int ret;
	int flag = 0;
	unsigned long timeout;

	pr_debug("Starting secondary CPU %d\n", cpu);

	/* Set preset_lpj to avoid subsequent lpj recalculations */
	preset_lpj = loops_per_jiffy;

	if (cpu > 0 && cpu < ARRAY_SIZE(cold_boot_flags))
		flag = cold_boot_flags[cpu];
	else
		__WARN();

	if (per_cpu(cold_boot_done, cpu) == false) {
		init_cpu_debug_counter_for_cold_boot();
		ret = scm_set_boot_addr((void *)
					virt_to_phys(msm_secondary_startup),
					flag);
		if (ret == 0)
			release_secondary(cpu);
		else
			printk(KERN_DEBUG "Failed to set secondary core boot "
					  "address\n");
		per_cpu(cold_boot_done, cpu) = true;
	}

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	pen_release = cpu_logical_map(cpu);
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	gic_raise_softirq(cpumask_of(cpu), 1);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		dmac_inv_range((void *)&pen_release,
			       (void *)(&pen_release+sizeof(pen_release)));
		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}
/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	unsigned int i, ncores = get_core_count();

	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
}
