/*
 * Copyright (C) 2011 Google, Inc.
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * Author:
 *	Colin Cross <ccross@android.com>
 *	Olof Johansson <olof@lixom.net>
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

#ifndef __TEGRA_EMC_H_
#define __TEGRA_EMC_H_

#define TEGRA_EMC_NUM_REGS 46

struct tegra_emc_table {
	unsigned long rate;
	u32 regs[TEGRA_EMC_NUM_REGS];
};

struct tegra_emc_pdata {
	int num_tables;
	struct tegra_emc_table *tables;
};


#ifdef CONFIG_TEGRA114_EMC
long tegra114_emc_round_rate(unsigned long rate);
int tegra114_emc_set_rate(unsigned long rate);
unsigned long tegra114_emc_get_rate(void);
struct clk *tegra114_emc_predict_parent(unsigned long rate);
void tegra114_emc_timing_invalidate(void);
bool tegra114_emc_is_ready(void);
#else
static inline long tegra114_emc_round_rate(unsigned long rate)
{ return 0; }
static inline int tegra114_emc_set_rate(unsigned long rate) { return -ENODEV; }
static inline unsigned long tegra114_emc_get_rate(void) { return -ENODEV; }
static inline struct clk *tegra114_emc_predict_parent(unsigned long rate)
{ return ERR_PTR(-ENODEV); }
static inline void tegra114_emc_timing_invalidate(void) { return; };
static inline bool tegra114_emc_is_ready(void) { return true; };
#endif

#endif
