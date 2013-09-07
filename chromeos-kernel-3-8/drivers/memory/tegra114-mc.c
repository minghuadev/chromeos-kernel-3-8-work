/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_data/tegra_mc.h>

#define DRV_NAME "tegra114-mc"

static void __iomem *tegra_mc_base;
static void __iomem *tegra_mc0_base;
static void __iomem *tegra_mc1_base;

u32 tegra114_mc_readl(u32 offs)
{
	return readl(tegra_mc_base + offs);
}
EXPORT_SYMBOL(tegra114_mc_readl);

void tegra114_mc_writel(u32 val, u32 offs)
{
	writel(val, tegra_mc_base + offs);
}
EXPORT_SYMBOL(tegra114_mc_writel);

static int tegra114_mc_probe(struct platform_device *pdev)
{
	tegra_mc0_base = of_iomap(pdev->dev.of_node, 0);
	tegra_mc_base = of_iomap(pdev->dev.of_node, 1);
	tegra_mc1_base = of_iomap(pdev->dev.of_node, 2);

	return 0;
}

static const struct of_device_id tegra114_mc_of_match[] = {
	{ .compatible = "nvidia,tegra114-mc", },
	{},
};

static struct platform_driver tegra114_mc_driver = {
	.probe = tegra114_mc_probe,
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tegra114_mc_of_match,
	},
};

module_platform_driver(tegra114_mc_driver);

MODULE_DESCRIPTION("Tegra114 MC driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
