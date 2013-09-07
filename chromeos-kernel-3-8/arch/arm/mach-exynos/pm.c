/*
 * Copyright (c) 2011-2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - Power Management support
 *
 * Based on arch/arm/mach-s3c2410/pm.c
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>

#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/smp_scu.h>

#include <plat/cpu.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-srom.h>

#include <mach/regs-irq.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-pmu.h>
#include <mach/pm-core.h>
#include <mach/pmu.h>

static struct sleep_save exynos4_set_clksrc[] = {
	{ .reg = EXYNOS4_CLKSRC_MASK_TOP		, .val = 0x00000001, },
	{ .reg = EXYNOS4_CLKSRC_MASK_CAM		, .val = 0x11111111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_TV			, .val = 0x00000111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_LCD0		, .val = 0x00001111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_MAUDIO		, .val = 0x00000001, },
	{ .reg = EXYNOS4_CLKSRC_MASK_FSYS		, .val = 0x01011111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_PERIL0		, .val = 0x01111111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_PERIL1		, .val = 0x01110111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_DMC		, .val = 0x00010000, },
};

static struct sleep_save exynos5420_set_clksrc[] = {
	{ .reg = EXYNOS5420_CLKSRC_MASK_CPERI,		.val = 0xffffffff, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_TOP0,		.val = 0x11111111, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_TOP1,		.val = 0x11101111, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_TOP2,		.val = 0x11111110, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_TOP7,		.val = 0x00111100, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_DISP10,		.val = 0x11111110, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_MAU,		.val = 0x10000000, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_FSYS,		.val = 0x11111110, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_PERIC0,		.val = 0x11111110, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_PERIC1,		.val = 0x11111100, },
	{ .reg = EXYNOS5420_CLKSRC_MASK_ISP,		.val = 0x11111000, },
	{ .reg = EXYNOS5420_CLKGATE_BUS_DISP1,		.val = 0xffffffff, },
	{ .reg = EXYNOS5420_CLKGATE_IP_PERIC,		.val = 0xffffffff, },
};

static struct sleep_save exynos4210_set_clksrc[] = {
	{ .reg = EXYNOS4210_CLKSRC_MASK_LCD1		, .val = 0x00001111, },
};

static struct sleep_save exynos4_epll_save[] = {
	SAVE_ITEM(EXYNOS4_EPLL_CON0),
	SAVE_ITEM(EXYNOS4_EPLL_CON1),
};

static struct sleep_save exynos4_vpll_save[] = {
	SAVE_ITEM(EXYNOS4_VPLL_CON0),
	SAVE_ITEM(EXYNOS4_VPLL_CON1),
};

static struct sleep_save exynos5_sys_save[] = {
	SAVE_ITEM(EXYNOS5_SYS_I2C_CFG),
};

static struct sleep_save exynos5420_sys_save[] = {
	SAVE_ITEM(EXYNOS5_SYS_DISP1_BLK_CFG),
};

static struct sleep_save exynos5420_cpustate_save[] = {
	SAVE_ITEM(EXYNOS5420_VA_CPU_STATE),
};

static struct sleep_save exynos_core_save[] = {
	/* SROM side */
	SAVE_ITEM(S5P_SROM_BW),
	SAVE_ITEM(S5P_SROM_BC0),
	SAVE_ITEM(S5P_SROM_BC1),
	SAVE_ITEM(S5P_SROM_BC2),
	SAVE_ITEM(S5P_SROM_BC3),
};


/* For Cortex-A9 Diagnostic and Power control register */
static unsigned int save_arm_register[2];

static int exynos_cpu_suspend(unsigned long arg)
{
	unsigned int i;

#ifdef CONFIG_CACHE_L2X0
	outer_flush_all();
#endif
	/*
	 * Clear IRAM register for cpu state so that primary CPU does
	 * not enter low power start in U-Boot.
	 * This is specific to exynos5420 SoC only.
	 */
	if (soc_is_exynos5420())
		__raw_writel(0x0, EXYNOS5420_VA_CPU_STATE);

	if (soc_is_exynos5250() || soc_is_exynos5420())
		flush_cache_all();

	/*
	 * Disable all interrupts.  eints will still be active during
	 * suspend so its ok to mask everything here
	 */
	if (soc_is_exynos5250() || soc_is_exynos5420())
		__raw_writel(0x0, S5P_VA_GIC_DIST + GIC_DIST_CTRL);

	/* issue the standby signal into the pm unit. */
	for (i = 0; i < 100; i++)
		cpu_do_idle();

	printk(KERN_ERR "Failed to suspend the system\n");
	return -EBUSY;
}

static void exynos_pm_prepare(void)
{
	unsigned int tmp;

	s3c_pm_do_save(exynos_core_save, ARRAY_SIZE(exynos_core_save));

	if (!(soc_is_exynos5250() || soc_is_exynos5420())) {
		s3c_pm_do_save(exynos4_epll_save, ARRAY_SIZE(exynos4_epll_save));
		s3c_pm_do_save(exynos4_vpll_save, ARRAY_SIZE(exynos4_vpll_save));
	}

	if (soc_is_exynos5250()) {
		s3c_pm_do_save(exynos5_sys_save, ARRAY_SIZE(exynos5_sys_save));
		/* Disable USE_RETENTION of JPEG_MEM_OPTION */
		tmp = __raw_readl(EXYNOS5_JPEG_MEM_OPTION);
		tmp &= ~EXYNOS5_OPTION_USE_RETENTION;
		__raw_writel(tmp, EXYNOS5_JPEG_MEM_OPTION);
	} else if (soc_is_exynos5420()) {
		s3c_pm_do_save(exynos5420_sys_save,
					ARRAY_SIZE(exynos5420_sys_save));
		/*
		 * The cpu state needs to be saved and restored so that the
		 * secondary CPUs will enter low power start. Though the U-Boot
		 * is setting the cpu state with low power flag, the kernel
		 * needs to restore it back in case, the primary cpu fails to
		 * suspend for any reason
		 */
		s3c_pm_do_save(exynos5420_cpustate_save,
			ARRAY_SIZE(exynos5420_cpustate_save));
	}

	/* Set value of power down register for sleep mode */

	exynos_sys_powerdown_conf(SYS_SLEEP);
	__raw_writel(S5P_CHECK_SLEEP, S5P_INFORM1);

	/* ensure at least INFORM0 has the resume address */

	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);

	/* Before enter central sequence mode, clock src register have to set */

	if (!(soc_is_exynos5250() || soc_is_exynos5420()))
		s3c_pm_do_restore_core(exynos4_set_clksrc, ARRAY_SIZE(exynos4_set_clksrc));

	if (soc_is_exynos4210())
		s3c_pm_do_restore_core(exynos4210_set_clksrc, ARRAY_SIZE(exynos4210_set_clksrc));

	if (soc_is_exynos5420()) {
		s3c_pm_do_restore_core(exynos5420_set_clksrc,
				ARRAY_SIZE(exynos5420_set_clksrc));

		tmp = __raw_readl(EXYNOS5_ARM_L2_OPTION);
		tmp &= ~EXYNOS5_USE_RETENTION;
		__raw_writel(tmp, EXYNOS5_ARM_L2_OPTION);

		tmp = __raw_readl(EXYNOS5420_SFR_AXI_CGDIS1);
		tmp |= EXYNOS5420_UFS;
		__raw_writel(tmp, EXYNOS5420_SFR_AXI_CGDIS1);

		tmp = __raw_readl(EXYNOS5420_ARM_COMMON_OPTION);
		tmp &= ~EXYNOS5_L2RSTDISABLE_VALUE;
		__raw_writel(tmp, EXYNOS5420_ARM_COMMON_OPTION);
		tmp = __raw_readl(EXYNOS5420_FSYS2_OPTION);
		tmp |= EXYNOS5420_EMULATION;
		__raw_writel(tmp, EXYNOS5420_FSYS2_OPTION);
		tmp = __raw_readl(EXYNOS5420_PSGEN_OPTION);
		tmp |= EXYNOS5420_EMULATION;
		__raw_writel(tmp, EXYNOS5420_PSGEN_OPTION);
	}
}

static int exynos_pm_add(struct device *dev, struct subsys_interface *sif)
{
	pm_cpu_prep = exynos_pm_prepare;
	pm_cpu_sleep = exynos_cpu_suspend;

	return 0;
}

static unsigned long pll_base_rate;

static void exynos4_restore_pll(void)
{
	unsigned long pll_con, locktime, lockcnt;
	unsigned long pll_in_rate;
	unsigned int p_div, epll_wait = 0, vpll_wait = 0;

	if (pll_base_rate == 0)
		return;

	pll_in_rate = pll_base_rate;

	/* EPLL */
	pll_con = exynos4_epll_save[0].val;

	if (pll_con & (1 << 31)) {
		pll_con &= (PLL46XX_PDIV_MASK << PLL46XX_PDIV_SHIFT);
		p_div = (pll_con >> PLL46XX_PDIV_SHIFT);

		pll_in_rate /= 1000000;

		locktime = (3000 / pll_in_rate) * p_div;
		lockcnt = locktime * 10000 / (10000 / pll_in_rate);

		__raw_writel(lockcnt, EXYNOS4_EPLL_LOCK);

		s3c_pm_do_restore_core(exynos4_epll_save,
					ARRAY_SIZE(exynos4_epll_save));
		epll_wait = 1;
	}

	pll_in_rate = pll_base_rate;

	/* VPLL */
	pll_con = exynos4_vpll_save[0].val;

	if (pll_con & (1 << 31)) {
		pll_in_rate /= 1000000;
		/* 750us */
		locktime = 750;
		lockcnt = locktime * 10000 / (10000 / pll_in_rate);

		__raw_writel(lockcnt, EXYNOS4_VPLL_LOCK);

		s3c_pm_do_restore_core(exynos4_vpll_save,
					ARRAY_SIZE(exynos4_vpll_save));
		vpll_wait = 1;
	}

	/* Wait PLL locking */

	do {
		if (epll_wait) {
			pll_con = __raw_readl(EXYNOS4_EPLL_CON0);
			if (pll_con & (1 << EXYNOS4_EPLLCON0_LOCKED_SHIFT))
				epll_wait = 0;
		}

		if (vpll_wait) {
			pll_con = __raw_readl(EXYNOS4_VPLL_CON0);
			if (pll_con & (1 << EXYNOS4_VPLLCON0_LOCKED_SHIFT))
				vpll_wait = 0;
		}
	} while (epll_wait || vpll_wait);
}

static struct subsys_interface exynos_pm_interface = {
	.name		= "exynos_pm",
	.subsys		= &exynos_subsys,
	.add_dev	= exynos_pm_add,
};

static __init int exynos_pm_drvinit(void)
{
	struct clk *pll_base;
	unsigned int tmp;

	s3c_pm_init();

	/* All wakeup disable */

	tmp = __raw_readl(S5P_WAKEUP_MASK);
	tmp |= ((0xFF << 8) | (0x1F << 1));
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	if (!(soc_is_exynos5250() || soc_is_exynos5420())) {
		pll_base = clk_get(NULL, "xtal");

		if (!IS_ERR(pll_base)) {
			pll_base_rate = clk_get_rate(pll_base);
			clk_put(pll_base);
		}
	}

	return subsys_interface_register(&exynos_pm_interface);
}
arch_initcall(exynos_pm_drvinit);

static int exynos_pm_suspend(void)
{
	unsigned long tmp;
	unsigned int cluster_id;

	/* Powering on ISP before suspend */
	if (soc_is_exynos5250())
		__raw_writel(S5P_INT_LOCAL_PWR_EN, EXYNOS5_ISP_CONFIGURATION);

	/* Setting Central Sequence Register for power down mode */

	tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
	tmp &= ~S5P_CENTRAL_LOWPWR_CFG;
	__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

	/* Setting SEQ_OPTION register */

	if (soc_is_exynos5250()) {
		tmp = (S5P_USE_STANDBY_WFI0 | S5P_USE_STANDBY_WFE0);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_OPTION);
	} else if (soc_is_exynos5420()) {
		cluster_id = (read_cpuid(CPUID_MPIDR) >> 8) & 0xf;
		if (!cluster_id)
			__raw_writel(EXYNOS5420_ARM_USE_STANDBY_WFI0,
				     S5P_CENTRAL_SEQ_OPTION);
		else
			__raw_writel(EXYNOS5420_KFC_USE_STANDBY_WFI0,
				     S5P_CENTRAL_SEQ_OPTION);
	}

	if (!(soc_is_exynos5250() || soc_is_exynos5420())) {
		/* Save Power control register */
		asm ("mrc p15, 0, %0, c15, c0, 0"
		     : "=r" (tmp) : : "cc");
		save_arm_register[0] = tmp;

		/* Save Diagnostic register */
		asm ("mrc p15, 0, %0, c15, c0, 1"
		     : "=r" (tmp) : : "cc");
		save_arm_register[1] = tmp;
	}

	return 0;
}

static void exynos_pm_resume(void)
{
	unsigned long tmp;

	if (soc_is_exynos5420()) {
		/* Restore the IRAM register cpu state */
		s3c_pm_do_restore(exynos5420_cpustate_save,
			ARRAY_SIZE(exynos5420_cpustate_save));

		__raw_writel(EXYNOS5420_USE_STANDBY_WFI_ALL,
			S5P_CENTRAL_SEQ_OPTION);
	}

	/*
	 * If PMU failed while entering sleep mode, WFI will be
	 * ignored by PMU and then exiting cpu_do_idle().
	 * S5P_CENTRAL_LOWPWR_CFG bit will not be set automatically
	 * in this situation.
	 */
	tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
	if (!(tmp & S5P_CENTRAL_LOWPWR_CFG)) {
		tmp |= S5P_CENTRAL_LOWPWR_CFG;
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);
		/* clear the wakeup state register */
		__raw_writel(0x0, S5P_WAKEUP_STAT);
		/* No need to perform below restore code */
		goto early_wakeup;
	}
	if (!(soc_is_exynos5250() || soc_is_exynos5420())) {
		/* Restore Power control register */
		tmp = save_arm_register[0];
		asm volatile ("mcr p15, 0, %0, c15, c0, 0"
			      : : "r" (tmp)
			      : "cc");

		/* Restore Diagnostic register */
		tmp = save_arm_register[1];
		asm volatile ("mcr p15, 0, %0, c15, c0, 1"
			      : : "r" (tmp)
			      : "cc");
	}

	/* For release retention */

	if (soc_is_exynos5250()) {
		__raw_writel((1 << 28), S5P_PAD_RET_MAUDIO_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_GPIO_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_UART_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_MMCA_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_MMCB_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_EBIA_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_EBIB_OPTION);
	} else if (soc_is_exynos5420()) {
		__raw_writel(1 << 28, EXYNOS_PAD_RET_DRAM_OPTION);
		__raw_writel(1 << 28, EXYNOS_PAD_RET_MAUDIO_OPTION);
		__raw_writel(1 << 28, EXYNOS_PAD_RET_JTAG_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_GPIO_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_UART_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_MMCA_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_MMCB_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_MMCC_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_HSI_OPTION);
		__raw_writel(1 << 28, EXYNOS_PAD_RET_EBIA_OPTION);
		__raw_writel(1 << 28, EXYNOS_PAD_RET_EBIB_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_SPI_OPTION);
		__raw_writel(1 << 28, EXYNOS5420_PAD_RET_DRAM_COREBLK_OPTION);
	}

	if (soc_is_exynos5250())
		s3c_pm_do_restore(exynos5_sys_save,
			ARRAY_SIZE(exynos5_sys_save));
	else if (soc_is_exynos5420())
		s3c_pm_do_restore(exynos5420_sys_save,
			ARRAY_SIZE(exynos5420_sys_save));

	s3c_pm_do_restore_core(exynos_core_save, ARRAY_SIZE(exynos_core_save));

	if (!(soc_is_exynos5250() || soc_is_exynos5420())) {
		exynos4_restore_pll();

#ifdef CONFIG_SMP
		scu_enable(S5P_VA_SCU);
#endif
	}

early_wakeup:

	/* Powering off ISP */
	if (soc_is_exynos5250())
		__raw_writel(0x0, EXYNOS5_ISP_CONFIGURATION);

	if (soc_is_exynos5420()) {
		tmp = __raw_readl(EXYNOS5420_SFR_AXI_CGDIS1);
		tmp &= ~EXYNOS5420_UFS;
		__raw_writel(tmp, EXYNOS5420_SFR_AXI_CGDIS1);
		tmp = __raw_readl(EXYNOS5420_FSYS2_OPTION);
		tmp &= ~EXYNOS5420_EMULATION;
		__raw_writel(tmp, EXYNOS5420_FSYS2_OPTION);
		tmp = __raw_readl(EXYNOS5420_PSGEN_OPTION);
		tmp &= ~EXYNOS5420_EMULATION;
		__raw_writel(tmp, EXYNOS5420_PSGEN_OPTION);
	}

	/* Clear SLEEP mode set in INFORM1 */
	__raw_writel(0x0, S5P_INFORM1);

	return;
}

static struct syscore_ops exynos_pm_syscore_ops = {
	.suspend	= exynos_pm_suspend,
	.resume		= exynos_pm_resume,
};

static __init int exynos_pm_syscore_init(void)
{
	register_syscore_ops(&exynos_pm_syscore_ops);
	return 0;
}
arch_initcall(exynos_pm_syscore_init);
