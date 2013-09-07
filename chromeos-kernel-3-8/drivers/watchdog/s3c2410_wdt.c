/* linux/drivers/char/watchdog/s3c2410_wdt.c
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 Watchdog Timer Support
 *
 * Based on, softdog.c by Alan Cox,
 *     (c) Copyright 1996 Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h> /* for MODULE_ALIAS_MISCDEV */
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>

#include <mach/map.h>

#undef S3C_VA_WATCHDOG
#define S3C_VA_WATCHDOG (0)

#include <plat/regs-watchdog.h>

#define CONFIG_S3C2410_WATCHDOG_ATBOOT		(0)
#define CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME	(15)

static bool nowayout	= WATCHDOG_NOWAYOUT;
static int tmr_margin	= CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME;
static int tmr_atboot	= CONFIG_S3C2410_WATCHDOG_ATBOOT;
static int soft_noboot;
static int debug;

module_param(tmr_margin,  int, 0);
module_param(tmr_atboot,  int, 0);
module_param(nowayout,   bool, 0);
module_param(soft_noboot, int, 0);
module_param(debug,	  int, 0);

MODULE_PARM_DESC(tmr_margin, "Watchdog tmr_margin in seconds. (default="
		__MODULE_STRING(CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME) ")");
MODULE_PARM_DESC(tmr_atboot,
		"Watchdog is started at boot time if set to 1, default="
			__MODULE_STRING(CONFIG_S3C2410_WATCHDOG_ATBOOT));
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");
MODULE_PARM_DESC(soft_noboot, "Watchdog action, set to 1 to ignore reboots, "
			"0 to reboot (default 0)");
MODULE_PARM_DESC(debug, "Watchdog debug, set to >1 for debug (default 0)");

static struct device    *wdt_dev;	/* platform device attached to */
static struct resource	*wdt_mem;
static struct resource	*wdt_irq;
static struct clk	*wdt_clock;
static void __iomem	*wdt_base;
static unsigned int	 wdt_count;
static DEFINE_SPINLOCK(wdt_lock);

static void __iomem	*pmu_wdt_disable_reg;
static void __iomem	*pmu_wdt_mask_reset_reg;
static int		pmu_wdt_mask_bit;

/* watchdog control routines */

#define DBG(fmt, ...)					\
do {							\
	if (debug)					\
		pr_info(fmt, ##__VA_ARGS__);		\
} while (0)

/* functions */

static void s3c2410wdt_mask_and_disable_reset(int mask, int mask_bit)
{
	unsigned int value;

	if (IS_ERR(pmu_wdt_disable_reg) || IS_ERR(pmu_wdt_mask_reset_reg)
					|| (mask_bit < 0))
		return;

	if (mask) {
		value = readl(pmu_wdt_disable_reg);
		value |= (1 << mask_bit);
		writel(value, pmu_wdt_disable_reg);

		value = readl(pmu_wdt_mask_reset_reg);
		value |= (1 << mask_bit);
		writel(value, pmu_wdt_mask_reset_reg);
	} else {
		value = readl(pmu_wdt_disable_reg);
		value &= ~(1 << mask_bit);
		writel(value, pmu_wdt_disable_reg);

		value = readl(pmu_wdt_mask_reset_reg);
		value &= ~(1 << mask_bit);
		writel(value, pmu_wdt_mask_reset_reg);
	}
}

static int s3c2410wdt_keepalive(struct watchdog_device *wdd)
{
	spin_lock(&wdt_lock);
	writel(wdt_count, wdt_base + S3C2410_WTCNT);
	spin_unlock(&wdt_lock);

	return 0;
}

static void __s3c2410wdt_stop(void)
{
	unsigned long wtcon;

	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon &= ~(S3C2410_WTCON_ENABLE | S3C2410_WTCON_RSTEN);
	writel(wtcon, wdt_base + S3C2410_WTCON);
}

static int s3c2410wdt_stop(struct watchdog_device *wdd)
{
	spin_lock(&wdt_lock);
	__s3c2410wdt_stop();
	spin_unlock(&wdt_lock);

	return 0;
}

static int s3c2410wdt_start(struct watchdog_device *wdd)
{
	unsigned long wtcon;

	spin_lock(&wdt_lock);

	__s3c2410wdt_stop();

	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon |= S3C2410_WTCON_ENABLE | S3C2410_WTCON_DIV128;

	if (soft_noboot) {
		wtcon |= S3C2410_WTCON_INTEN;
		wtcon &= ~S3C2410_WTCON_RSTEN;
	} else {
		wtcon &= ~S3C2410_WTCON_INTEN;
		wtcon |= S3C2410_WTCON_RSTEN;
	}

	DBG("%s: wdt_count=0x%08x, wtcon=%08lx\n",
	    __func__, wdt_count, wtcon);

	writel(wdt_count, wdt_base + S3C2410_WTDAT);
	writel(wdt_count, wdt_base + S3C2410_WTCNT);
	writel(wtcon, wdt_base + S3C2410_WTCON);
	spin_unlock(&wdt_lock);

	return 0;
}

static inline int s3c2410wdt_is_running(void)
{
	return readl(wdt_base + S3C2410_WTCON) & S3C2410_WTCON_ENABLE;
}

static int s3c2410wdt_set_heartbeat(struct watchdog_device *wdd, unsigned timeout)
{
	unsigned long freq = clk_get_rate(wdt_clock);
	unsigned int count;
	unsigned int divisor = 1;
	unsigned long wtcon;

	if (timeout < 1)
		return -EINVAL;

	freq /= 128;
	count = timeout * freq;

	DBG("%s: count=%d, timeout=%d, freq=%lu\n",
	    __func__, count, timeout, freq);

	/* if the count is bigger than the watchdog register,
	   then work out what we need to do (and if) we can
	   actually make this value
	*/

	if (count >= 0x10000) {
		for (divisor = 1; divisor <= 0x100; divisor++) {
			if ((count / divisor) < 0x10000)
				break;
		}

		if ((count / divisor) >= 0x10000) {
			dev_err(wdt_dev, "timeout %d too big\n", timeout);
			return -EINVAL;
		}
	}

	DBG("%s: timeout=%d, divisor=%d, count=%d (%08x)\n",
	    __func__, timeout, divisor, count, count/divisor);

	count /= divisor;
	wdt_count = count;

	/* update the pre-scaler */
	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon &= ~S3C2410_WTCON_PRESCALE_MASK;
	wtcon |= S3C2410_WTCON_PRESCALE(divisor-1);

	writel(count, wdt_base + S3C2410_WTDAT);
	writel(wtcon, wdt_base + S3C2410_WTCON);

	wdd->timeout = (count * divisor) / freq;

	return 0;
}

#define OPTIONS (WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE)

static const struct watchdog_info s3c2410_wdt_ident = {
	.options          =     OPTIONS,
	.firmware_version =	0,
	.identity         =	"S3C2410 Watchdog",
};

static struct watchdog_ops s3c2410wdt_ops = {
	.owner = THIS_MODULE,
	.start = s3c2410wdt_start,
	.stop = s3c2410wdt_stop,
	.ping = s3c2410wdt_keepalive,
	.set_timeout = s3c2410wdt_set_heartbeat,
};

static struct watchdog_device s3c2410_wdd = {
	.info = &s3c2410_wdt_ident,
	.ops = &s3c2410wdt_ops,
};

/* interrupt handler code */

static irqreturn_t s3c2410wdt_irq(int irqno, void *param)
{
	dev_info(wdt_dev, "watchdog timer expired (irq)\n");

	s3c2410wdt_keepalive(&s3c2410_wdd);
	return IRQ_HANDLED;
}

static int wdt_clk_change(bool is_pre_rate_change)
{
	int ret;

	if (!s3c2410wdt_is_running())
		goto done;

	if (is_pre_rate_change) {
		/* To ensure that over the change we don't cause the
		 * watchdog to trigger, we perform an keep-alive if
		 * the watchdog is running.
		 */

		s3c2410wdt_keepalive(&s3c2410_wdd);
	} else {
		s3c2410wdt_stop(&s3c2410_wdd);

		ret = s3c2410wdt_set_heartbeat(&s3c2410_wdd,
						s3c2410_wdd.timeout);

		if (ret >= 0)
			s3c2410wdt_start(&s3c2410_wdd);
		else
			goto err;
	}

done:
	return 0;

err:
	dev_err(wdt_dev, "cannot set new value for timeout %d\n",
				s3c2410_wdd.timeout);
	return ret;
}

#ifdef CONFIG_COMMON_CLK
static int wdt_clk_notify(struct notifier_block *nb,
				unsigned long action, void *wclk)
{
	if (action == PRE_RATE_CHANGE)
		return wdt_clk_change(true);
	else if (action == POST_RATE_CHANGE)
		return wdt_clk_change(false);
	return 0;
}

static struct notifier_block wdt_clk_nb = {
	.notifier_call = wdt_clk_notify,
};

static inline int s3c2410wdt_clk_notify_register(void)
{
	return clk_notifier_register(wdt_clock, &wdt_clk_nb);
}

static inline int s3c2410wdt_clk_notify_unregister(void)
{
	return clk_notifier_unregister(wdt_clock, &wdt_clk_nb);
}
#else
static inline int s3c2410wdt_clk_notify_register(void)
{
	return -EINVAL;
}

static inline int s3c2410wdt_clk_notify_unregister(void)
{
	return -EINVAL;
}
#endif

#if defined(CONFIG_CPU_FREQ) && !defined(CONFIG_COMMON_CLK)

static int s3c2410wdt_cpufreq_transition(struct notifier_block *nb,
					  unsigned long val, void *data)
{
	if (val == CPUFREQ_PRECHANGE)
		return wdt_clk_change(true);
	else if (val == CPUFREQ_POSTCHANGE)
		return wdt_clk_change(false);
	return 0;
}

static struct notifier_block s3c2410wdt_cpufreq_transition_nb = {
	.notifier_call	= s3c2410wdt_cpufreq_transition,
};

static inline int s3c2410wdt_cpufreq_register(void)
{
	return cpufreq_register_notifier(&s3c2410wdt_cpufreq_transition_nb,
					 CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void s3c2410wdt_cpufreq_deregister(void)
{
	cpufreq_unregister_notifier(&s3c2410wdt_cpufreq_transition_nb,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int s3c2410wdt_cpufreq_register(void)
{
	return 0;
}

static inline void s3c2410wdt_cpufreq_deregister(void)
{
}
#endif

static int s3c2410wdt_probe(struct platform_device *pdev)
{
	struct device *dev;
	unsigned int wtcon;
	int started = 0;
	int ret;
	int size;
	struct resource *res;
	unsigned int mask_bit;

	DBG("%s: probe=%p\n", __func__, pdev);

	dev = &pdev->dev;
	wdt_dev = &pdev->dev;

	wdt_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (wdt_mem == NULL) {
		dev_err(dev, "no memory resource specified\n");
		return -ENOENT;
	}

	wdt_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (wdt_irq == NULL) {
		dev_err(dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err;
	}

	/* get the memory region for the watchdog timer */

	size = resource_size(wdt_mem);
	if (!request_mem_region(wdt_mem->start, size, pdev->name)) {
		dev_err(dev, "failed to get memory region\n");
		ret = -EBUSY;
		goto err;
	}

	wdt_base = ioremap(wdt_mem->start, size);
	if (wdt_base == NULL) {
		dev_err(dev, "failed to ioremap() region\n");
		ret = -EINVAL;
		goto err_req;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pmu_wdt_disable_reg = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	pmu_wdt_mask_reset_reg = devm_ioremap_resource(&pdev->dev, res);

	if (!IS_ERR(pmu_wdt_disable_reg) && !IS_ERR(pmu_wdt_mask_reset_reg)) {
		if (pdev->dev.of_node) {
			if (of_property_read_u32(pdev->dev.of_node,
							"reset-mask-bit",
							&mask_bit)) {
				dev_warn(dev, "reset-mask-bit not specified\n");
				pmu_wdt_mask_bit = -EINVAL;
			} else {
				pmu_wdt_mask_bit = mask_bit;
			}
		}
	}

	DBG("probe: mapped wdt_base=%p\n", wdt_base);

	wdt_clock = clk_get(&pdev->dev, "watchdog");
	if (IS_ERR(wdt_clock)) {
		dev_err(dev, "failed to find watchdog clock source\n");
		ret = PTR_ERR(wdt_clock);
		goto err_map;
	}

	clk_prepare_enable(wdt_clock);

	/* WDT registers to CCF for clock change notifications first
	 * and registers to cpufreq in case of CCF registration fails */

	if (s3c2410wdt_clk_notify_register() != 0) {
		ret = s3c2410wdt_cpufreq_register();
		if (ret < 0) {
			pr_err("failed to register cpufreq\n");
			goto err_clk;
		}
	}

	/* see if we can actually set the requested timer margin, and if
	 * not, try the default value */

	if (s3c2410wdt_set_heartbeat(&s3c2410_wdd, tmr_margin)) {
		started = s3c2410wdt_set_heartbeat(&s3c2410_wdd,
					CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME);

		if (started == 0)
			dev_info(dev,
			   "tmr_margin value out of range, default %d used\n",
			       CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME);
		else
			dev_info(dev, "default timer value is out of range, "
							"cannot start\n");
	}

	ret = request_irq(wdt_irq->start, s3c2410wdt_irq, 0, pdev->name, pdev);
	if (ret != 0) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
		goto err_cpufreq;
	}

	watchdog_set_nowayout(&s3c2410_wdd, nowayout);

	ret = watchdog_register_device(&s3c2410_wdd);
	if (ret) {
		dev_err(dev, "cannot register watchdog (%d)\n", ret);
		goto err_irq;
	}

	if (tmr_atboot && started == 0) {
		dev_info(dev, "starting watchdog timer\n");
		s3c2410wdt_start(&s3c2410_wdd);
	} else if (!tmr_atboot) {
		/* if we're not enabling the watchdog, then ensure it is
		 * disabled if it has been left running from the bootloader
		 * or other source */

		s3c2410wdt_stop(&s3c2410_wdd);
	}

	/* print out a statement of readiness */

	wtcon = readl(wdt_base + S3C2410_WTCON);

	dev_info(dev, "watchdog %sactive, reset %sabled, irq %sabled\n",
		 (wtcon & S3C2410_WTCON_ENABLE) ?  "" : "in",
		 (wtcon & S3C2410_WTCON_RSTEN) ? "en" : "dis",
		 (wtcon & S3C2410_WTCON_INTEN) ? "en" : "dis");

	s3c2410wdt_mask_and_disable_reset(0, pmu_wdt_mask_bit);
	return 0;

 err_irq:
	free_irq(wdt_irq->start, pdev);

 err_cpufreq:
	if (s3c2410wdt_clk_notify_unregister())
		s3c2410wdt_cpufreq_deregister();

 err_clk:
	clk_disable_unprepare(wdt_clock);
	clk_put(wdt_clock);
	wdt_clock = NULL;

 err_map:
	iounmap(wdt_base);

 err_req:
	release_mem_region(wdt_mem->start, size);

 err:
	wdt_irq = NULL;
	wdt_mem = NULL;
	return ret;
}

static int s3c2410wdt_remove(struct platform_device *dev)
{
	s3c2410wdt_mask_and_disable_reset(1, pmu_wdt_mask_bit);
	watchdog_unregister_device(&s3c2410_wdd);

	free_irq(wdt_irq->start, dev);

	if (s3c2410wdt_clk_notify_unregister())
		s3c2410wdt_cpufreq_deregister();

	clk_disable_unprepare(wdt_clock);
	clk_put(wdt_clock);
	wdt_clock = NULL;

	iounmap(wdt_base);

	release_mem_region(wdt_mem->start, resource_size(wdt_mem));
	wdt_irq = NULL;
	wdt_mem = NULL;
	return 0;
}

static void s3c2410wdt_shutdown(struct platform_device *dev)
{
	s3c2410wdt_mask_and_disable_reset(1, pmu_wdt_mask_bit);
	s3c2410wdt_stop(&s3c2410_wdd);
}

#ifdef CONFIG_PM

static unsigned long wtcon_save;
static unsigned long wtdat_save;

static int s3c2410wdt_suspend(struct platform_device *dev, pm_message_t state)
{
	/* Save watchdog state, and turn it off. */
	wtcon_save = readl(wdt_base + S3C2410_WTCON);
	wtdat_save = readl(wdt_base + S3C2410_WTDAT);

	s3c2410wdt_mask_and_disable_reset(1, pmu_wdt_mask_bit);
	/* Note that WTCNT doesn't need to be saved. */
	s3c2410wdt_stop(&s3c2410_wdd);

	return 0;
}

static int s3c2410wdt_resume(struct platform_device *dev)
{
	/* Restore watchdog state. */

	writel(wtdat_save, wdt_base + S3C2410_WTDAT);
	writel(wtdat_save, wdt_base + S3C2410_WTCNT); /* Reset count */
	writel(wtcon_save, wdt_base + S3C2410_WTCON);

	s3c2410wdt_mask_and_disable_reset(0, pmu_wdt_mask_bit);
	pr_info("watchdog %sabled\n",
		(wtcon_save & S3C2410_WTCON_ENABLE) ? "en" : "dis");

	return 0;
}

#else
#define s3c2410wdt_suspend NULL
#define s3c2410wdt_resume  NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id s3c2410_wdt_match[] = {
	{ .compatible = "samsung,s3c2410-wdt" },
	{},
};
MODULE_DEVICE_TABLE(of, s3c2410_wdt_match);
#endif

static struct platform_driver s3c2410wdt_driver = {
	.probe		= s3c2410wdt_probe,
	.remove		= s3c2410wdt_remove,
	.shutdown	= s3c2410wdt_shutdown,
	.suspend	= s3c2410wdt_suspend,
	.resume		= s3c2410wdt_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c2410-wdt",
		.of_match_table	= of_match_ptr(s3c2410_wdt_match),
	},
};

module_platform_driver(s3c2410wdt_driver);

MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>, "
	      "Dimitry Andric <dimitry.andric@tomtom.com>");
MODULE_DESCRIPTION("S3C2410 Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:s3c2410-wdt");
