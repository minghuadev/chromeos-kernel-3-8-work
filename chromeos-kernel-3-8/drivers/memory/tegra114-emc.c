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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/tegra-soc.h>
#include <linux/platform_data/tegra_emc.h>
#include <linux/platform_data/tegra_mc.h>

#include <asm/cputime.h>

#include "tegra114-emc-reg.h"

#define EMC_CLK_DIV_SHIFT			0
#define EMC_CLK_DIV_MASK			(0xFF << EMC_CLK_DIV_SHIFT)
#define EMC_CLK_SOURCE_SHIFT			29
#define EMC_CLK_SOURCE_MASK			(0x7 << EMC_CLK_SOURCE_SHIFT)
#define EMC_CLK_LOW_JITTER_ENABLE		(0x1 << 31)
#define	EMC_CLK_MC_SAME_FREQ			(0x1 << 16)

#define TEGRA_EMC_TABLE_MAX_SIZE		16
#define EMC_STATUS_UPDATE_TIMEOUT		100

static bool emc_enable = true;
module_param(emc_enable, bool, 0644);

#define EMC_BURST_REG_LIST	\
{	\
	DEFINE_REG(EMC_RC),				\
	DEFINE_REG(EMC_RFC),				\
	DEFINE_REG(EMC_RFC_SLR),			\
	DEFINE_REG(EMC_RAS),				\
	DEFINE_REG(EMC_RP),				\
	DEFINE_REG(EMC_R2W),				\
	DEFINE_REG(EMC_W2R),				\
	DEFINE_REG(EMC_R2P),				\
	DEFINE_REG(EMC_W2P),				\
	DEFINE_REG(EMC_RD_RCD),				\
	DEFINE_REG(EMC_WR_RCD),				\
	DEFINE_REG(EMC_RRD),				\
	DEFINE_REG(EMC_REXT),				\
	DEFINE_REG(EMC_WEXT),				\
	DEFINE_REG(EMC_WDV),				\
	DEFINE_REG(EMC_WDV_MASK),			\
	DEFINE_REG(EMC_IBDLY),				\
	DEFINE_REG(EMC_PUTERM_EXTRA),			\
	DEFINE_REG(EMC_CDB_CNTL_2),			\
	DEFINE_REG(EMC_QRST),				\
	DEFINE_REG(EMC_RDV_MASK),			\
	DEFINE_REG(EMC_REFRESH),			\
	DEFINE_REG(EMC_BURST_REFRESH_NUM),		\
	DEFINE_REG(EMC_PRE_REFRESH_REQ_CNT),		\
	DEFINE_REG(EMC_PDEX2WR),			\
	DEFINE_REG(EMC_PDEX2RD),			\
	DEFINE_REG(EMC_PCHG2PDEN),			\
	DEFINE_REG(EMC_ACT2PDEN),			\
	DEFINE_REG(EMC_AR2PDEN),			\
	DEFINE_REG(EMC_RW2PDEN),			\
	DEFINE_REG(EMC_TXSR),				\
	DEFINE_REG(EMC_TXSRDLL),			\
	DEFINE_REG(EMC_TCKE),				\
	DEFINE_REG(EMC_TCKESR),				\
	DEFINE_REG(EMC_TPD),				\
	DEFINE_REG(EMC_TFAW),				\
	DEFINE_REG(EMC_TRPAB),				\
	DEFINE_REG(EMC_TCLKSTABLE),			\
	DEFINE_REG(EMC_TCLKSTOP),			\
	DEFINE_REG(EMC_TREFBW),				\
	DEFINE_REG(EMC_QUSE_EXTRA),			\
	DEFINE_REG(EMC_ODT_WRITE),			\
	DEFINE_REG(EMC_ODT_READ),			\
	DEFINE_REG(EMC_FBIO_CFG5),			\
	DEFINE_REG(EMC_CFG_DIG_DLL),			\
	DEFINE_REG(EMC_CFG_DIG_DLL_PERIOD),		\
	DEFINE_REG(EMC_DLL_XFORM_DQS4),			\
	DEFINE_REG(EMC_DLL_XFORM_DQS5),			\
	DEFINE_REG(EMC_DLL_XFORM_DQS6),			\
	DEFINE_REG(EMC_DLL_XFORM_DQS7),			\
	DEFINE_REG(EMC_DLL_XFORM_QUSE4),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE5),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE6),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE7),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS4),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS5),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS6),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS7),		\
	DEFINE_REG(EMC_XM2CMDPADCTRL),			\
	DEFINE_REG(EMC_XM2CMDPADCTRL4),			\
	DEFINE_REG(EMC_XM2DQSPADCTRL2),			\
	DEFINE_REG(EMC_XM2DQPADCTRL2),			\
	DEFINE_REG(EMC_XM2CLKPADCTRL),			\
	DEFINE_REG(EMC_XM2COMPPADCTRL),			\
	DEFINE_REG(EMC_XM2VTTGENPADCTRL),		\
	DEFINE_REG(EMC_XM2VTTGENPADCTRL2),		\
	DEFINE_REG(EMC_DSR_VTTGEN_DRV),			\
	DEFINE_REG(EMC_TXDSRVTTGEN),			\
	DEFINE_REG(EMC_FBIO_SPARE),			\
	DEFINE_REG(EMC_CTT_TERM_CTRL),			\
	DEFINE_REG(EMC_ZCAL_INTERVAL),			\
	DEFINE_REG(EMC_ZCAL_WAIT_CNT),			\
	DEFINE_REG(EMC_MRS_WAIT_CNT),			\
	DEFINE_REG(EMC_MRS_WAIT_CNT2),			\
	DEFINE_REG(EMC_AUTO_CAL_CONFIG2),		\
	DEFINE_REG(EMC_AUTO_CAL_CONFIG3),		\
	DEFINE_REG(EMC_CTT),				\
	DEFINE_REG(EMC_CTT_DURATION),			\
	DEFINE_REG(EMC_DYN_SELF_REF_CONTROL),		\
	DEFINE_REG(EMC_CA_TRAINING_TIMING_CNTL1),	\
	DEFINE_REG(EMC_CA_TRAINING_TIMING_CNTL2),	\
}

#define MC_BURST_REG_LIST	\
{	\
	DEFINE_REG(MC_EMEM_ARB_CFG),			\
	DEFINE_REG(MC_EMEM_ARB_OUTSTANDING_REQ),	\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RCD),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RP),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RC),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RAS),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_FAW),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RRD),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_RAP2PRE),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_WAP2PRE),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_R2R),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_W2W),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_R2W),		\
	DEFINE_REG(MC_EMEM_ARB_TIMING_W2R),		\
	DEFINE_REG(MC_EMEM_ARB_DA_TURNS),		\
	DEFINE_REG(MC_EMEM_ARB_DA_COVERS),		\
	DEFINE_REG(MC_EMEM_ARB_MISC0),			\
	DEFINE_REG(MC_EMEM_ARB_RING1_THROTTLE),	\
}

#define BURST_UP_DOWN_REG_LIST	\
{	\
	DEFINE_REG(MC_PTSA_GRANT_DECREMENT),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_G2_0),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_G2_1),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV_0),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV2_0),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV_2),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV_1),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV2_1),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_NV_3),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_EPP_0),		\
	DEFINE_REG(MC_LATENCY_ALLOWANCE_EPP_1),\
	}

#define EMC_TRIMMERS_REG_LIST	\
{	\
	DEFINE_REG(EMC_CDB_CNTL_1),			\
	DEFINE_REG(EMC_FBIO_CFG6),			\
	DEFINE_REG(EMC_QUSE),				\
	DEFINE_REG(EMC_EINPUT),				\
	DEFINE_REG(EMC_EINPUT_DURATION),		\
	DEFINE_REG(EMC_DLL_XFORM_DQS0),			\
	DEFINE_REG(EMC_QSAFE),				\
	DEFINE_REG(EMC_DLL_XFORM_QUSE0),		\
	DEFINE_REG(EMC_RDV),				\
	DEFINE_REG(EMC_XM2DQSPADCTRL4),			\
	DEFINE_REG(EMC_XM2DQSPADCTRL3),			\
	DEFINE_REG(EMC_DLL_XFORM_DQ0),			\
	DEFINE_REG(EMC_AUTO_CAL_CONFIG),		\
	DEFINE_REG(EMC_DLL_XFORM_ADDR0),		\
	DEFINE_REG(EMC_XM2CLKPADCTRL2),			\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS0),		\
	DEFINE_REG(EMC_DLL_XFORM_ADDR1),		\
	DEFINE_REG(EMC_DLL_XFORM_ADDR2),		\
	DEFINE_REG(EMC_DLL_XFORM_DQS1),			\
	DEFINE_REG(EMC_DLL_XFORM_DQS2),			\
	DEFINE_REG(EMC_DLL_XFORM_DQS3),			\
	DEFINE_REG(EMC_DLL_XFORM_DQ1),			\
	DEFINE_REG(EMC_DLL_XFORM_DQ2),			\
	DEFINE_REG(EMC_DLL_XFORM_DQ3),			\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS1),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS2),		\
	DEFINE_REG(EMC_DLI_TRIM_TXDQS3),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE1),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE2),		\
	DEFINE_REG(EMC_DLL_XFORM_QUSE3),\
}

#define DEFINE_REG(reg)		reg##_INDEX
enum EMC_BURST_REG_LIST;
enum MC_BURST_REG_LIST;
enum BURST_UP_DOWN_REG_LIST;
#undef DEFINE_REG

#define DEFINE_REG(reg)		reg##_TRIM_INDEX
enum EMC_TRIMMERS_REG_LIST;
#undef DEFINE_REG

#define DEFINE_REG(reg)		(reg)
static const u32 emc_burst_reg_addr[] = EMC_BURST_REG_LIST;
static const u32 mc_burst_reg_addr[] = MC_BURST_REG_LIST;
static const u32 burst_up_down_reg_addr[] = BURST_UP_DOWN_REG_LIST;
static const u32 emc_trimmer_offs[] = EMC_TRIMMERS_REG_LIST;
#undef DEFINE_REG

enum {
	DLL_CHANGE_NONE = 0,
	DLL_CHANGE_ON,
	DLL_CHANGE_OFF,
};

enum TEGRA_EMC_SOURCE {
	TEGRA_EMC_SRC_PLLM,
	TEGRA_EMC_SRC_PLLC,
	TEGRA_EMC_SRC_PLLP,
	TEGRA_EMC_SRC_CLKM,
	TEGRA_EMC_SRC_PLLM_UD,
	TEGRA_EMC_SRC_PLLC2,
	TEGRA_EMC_SRC_PLLC3,
	TEGRA_EMC_SRC_COUNT,
};

struct emc_table {
	u8 rev;
	unsigned long rate;
	int emc_min_mv;
	const char *src_name;
	u32 src_sel_reg;

	int emc_burst_regs_num;
	int mc_burst_regs_num;
	int emc_trimmers_num;
	int up_down_regs_num;

	/* unconditionally updated in one burst shot */
	u32 *emc_burst_regs;
	u32 *mc_burst_regs;

	/* unconditionally updated in one burst shot to particular channel */
	u32 *emc_trimmers_0;
	u32 *emc_trimmers_1;

	/* one burst shot, but update time depends on rate change direction */
	u32 *up_down_regs;

	/* updated separately under some conditions */
	u32 emc_zcal_cnt_long;
	u32 emc_acal_interval;
	u32 emc_cfg;
	u32 emc_mode_reset;
	u32 emc_mode_1;
	u32 emc_mode_2;
	u32 emc_mode_4;
	u32 clock_change_latency;
};

struct emc_stats {
	cputime64_t time_at_clock[TEGRA_EMC_TABLE_MAX_SIZE];
	int last_sel;
	u64 last_update;
	u64 clkchange_count;
	spinlock_t spinlock;
};

struct emc_sel {
	struct clk	*input;
	u32		value;
	unsigned long	input_rate;
};

static DEFINE_SPINLOCK(emc_access_lock);
static ktime_t clkchange_time;
static int tegra_emc_table_size;
static int clkchange_delay = 100;
static int last_round_idx;
static u32 tegra_dram_dev_num;
static u32 tegra_dram_type = -1;
static bool tegra_emc_ready;
static void __iomem *tegra_emc_base;
static void __iomem *tegra_emc0_base;
static void __iomem *tegra_emc1_base;
static void __iomem *tegra_clk_base;
static unsigned long emc_backup_rate;
static struct emc_sel tegra_emc_clk_sel[TEGRA_EMC_TABLE_MAX_SIZE];
static struct emc_stats tegra_emc_stats;
static struct emc_table *tegra_emc_table;
static struct emc_table *emc_timing;
static struct emc_table start_timing;
static struct clk *emc_clk;
static struct clk *emc_backup_src;
static struct clk *tegra_emc_src[TEGRA_EMC_SRC_COUNT];
static const char *tegra_emc_src_names[TEGRA_EMC_SRC_COUNT] = {
	[TEGRA_EMC_SRC_PLLM] = "pll_m",
	[TEGRA_EMC_SRC_PLLC] = "pll_c",
	[TEGRA_EMC_SRC_PLLP] = "pll_p",
	[TEGRA_EMC_SRC_CLKM] = "clk_m",
	[TEGRA_EMC_SRC_PLLM_UD] = "pll_m_ud",
	[TEGRA_EMC_SRC_PLLC2] = "pll_c2",
	[TEGRA_EMC_SRC_PLLC3] = "pll_c3",
};

static inline void emc0_writel(u32 val, unsigned long addr)
{
	writel(val, tegra_emc0_base + addr);
}

static inline u32 emc0_readl(unsigned long addr)
{
	return readl(tegra_emc0_base + addr);
}

static inline void emc1_writel(u32 val, unsigned long addr)
{
	writel(val, tegra_emc1_base + addr);
}

static inline u32 emc1_readl(unsigned long addr)
{
	return readl(tegra_emc1_base + addr);
}

static inline void emc_writel(u32 val, unsigned long addr)
{
	writel(val, tegra_emc_base + addr);
}

static inline u32 emc_readl(unsigned long addr)
{
	return readl(tegra_emc_base + addr);
}

static inline int get_start_idx(unsigned long rate)
{
	if (tegra_emc_table[last_round_idx].rate == rate)
		return last_round_idx;
	return 0;
}

static inline void ccfifo_writel(u32 val, unsigned long addr)
{
	writel(val, tegra_emc_base + EMC_CCFIFO_DATA);
	writel(addr, tegra_emc_base + EMC_CCFIFO_ADDR);
}

static inline void clk_cfg_writel(u32 val)
{
	writel(val, tegra_clk_base + 0x19c);
}

static inline u32 clk_cfg_readl(void)
{
	return readl(tegra_clk_base + 0x19c);
}

static inline u32 emc_src_val(u32 val)
{
	return (val & EMC_CLK_SOURCE_MASK) >> EMC_CLK_SOURCE_SHIFT;
}

static inline u32 emc_div_val(u32 val)
{
	return (val & EMC_CLK_DIV_MASK) >> EMC_CLK_DIV_SHIFT;
}

void tegra114_emc_timing_invalidate(void)
{
	emc_timing = NULL;
}

bool tegra114_emc_is_ready(void)
{
	return tegra_emc_ready;
}
EXPORT_SYMBOL(tegra114_emc_is_ready);

unsigned long tegra114_emc_get_rate(void)
{
	u32 val;
	u32 div_value;
	u32 src_value;
	unsigned long rate;

	if (!emc_enable)
		return -ENODEV;

	if (!tegra_emc_table_size)
		return -EINVAL;

	val = clk_cfg_readl();

	div_value = emc_div_val(val);
	src_value = emc_src_val(val);

	rate = __clk_get_rate(tegra_emc_src[src_value]);

	do_div(rate, div_value + 2);

	return rate * 2;
}

long tegra114_emc_round_rate(unsigned long rate)
{
	int i;
	int max = 0;

	if (!emc_enable)
		return 0;

	if (!tegra_emc_table_size)
		return 0;

	i = get_start_idx(rate);
	for (; i < tegra_emc_table_size; i++) {
		if (tegra_emc_clk_sel[i].input == NULL)
			continue;

		max = i;
		if (tegra_emc_table[i].rate >= rate) {
			last_round_idx = i;
			return tegra_emc_table[i].rate;
		}
	}

	return tegra_emc_table[max].rate;
}

static inline void emc_get_timing(struct emc_table *timing)
{
	int i;

	for (i = 0; i < timing->emc_burst_regs_num; i++) {
		if (emc_burst_reg_addr[i])
			timing->emc_burst_regs[i] =
				emc_readl(emc_burst_reg_addr[i]);
		else
			timing->emc_burst_regs[i] = 0;
	}

	for (i = 0; i < timing->mc_burst_regs_num; i++) {
		if (mc_burst_reg_addr[i])
			timing->mc_burst_regs[i] =
				tegra114_mc_readl(mc_burst_reg_addr[i]);
		else
			timing->mc_burst_regs[i] = 0;
	}

	for (i = 0; i < timing->emc_trimmers_num; i++) {
		timing->emc_trimmers_0[i] = emc0_readl(emc_trimmer_offs[i]);
		timing->emc_trimmers_1[i] = emc1_readl(emc_trimmer_offs[i]);
	}

	timing->emc_acal_interval = 0;
	timing->emc_zcal_cnt_long = 0;
	timing->emc_mode_reset = 0;
	timing->emc_mode_1 = 0;
	timing->emc_mode_2 = 0;
	timing->emc_mode_4 = 0;
	timing->emc_cfg = emc_readl(EMC_CFG);
	timing->rate = __clk_get_rate(emc_clk);
}

static inline int get_dll_change(const struct emc_table *next_timing,
				const struct emc_table *last_timing)
{
	bool next_dll_enabled = !(next_timing->emc_mode_1 & 0x1);
	bool last_dll_enabled = !(last_timing->emc_mode_1 & 0x1);

	if (next_dll_enabled == last_dll_enabled)
		return DLL_CHANGE_NONE;
	else if (next_dll_enabled)
		return DLL_CHANGE_ON;
	else
		return DLL_CHANGE_OFF;
}

static inline bool dqs_preset(const struct emc_table *next_timing,
				const struct emc_table *last_timing)
{
	bool ret = false;

#define DQS_SET(reg, bit)						\
	do {								\
		if ((next_timing->emc_burst_regs[EMC_##reg##_INDEX] &	\
			EMC_##reg##_##bit##_ENABLE) &&			\
			(!(last_timing->emc_burst_regs[EMC_##reg##_INDEX] &\
			EMC_##reg##_##bit##_ENABLE))) {			\
			emc_writel(					\
				last_timing->emc_burst_regs[EMC_##reg##_INDEX]\
				| EMC_##reg##_##bit##_ENABLE, EMC_##reg);\
			ret = true;					\
		}							\
	} while (0)

	DQS_SET(XM2DQSPADCTRL2, VREF);

	return ret;
}

static int wait_for_update(u32 status_reg, u32 bit_mask, bool updated_state)
{
	int i;
	for (i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; i++) {
		if (!!(emc_readl(status_reg) & bit_mask) == updated_state)
			return 0;
		udelay(1);
	}
	return -ETIMEDOUT;
}

static inline void emc_timing_update(void)
{
	int err;

	emc_writel(0x1, EMC_TIMING_CONTROL);
	err = wait_for_update(EMC_STATUS, EMC_STATUS_TIMING_UPDATE_STALLED,
		false);
	if (err) {
		pr_err("%s: timing update error: %d", __func__, err);
		BUG();
	}
}

static inline void overwrite_mrs_wait_cnt(const struct emc_table *next_timing,
	bool zcal_long)
{
	u32 reg;
	u32 cnt = 512;

	/* For ddr3 when DLL is re-started: overwrite EMC DFS table settings
	   for MRS_WAIT_LONG with maximum of MRS_WAIT_SHORT settings and
	   expected operation length. Reduce the latter by the overlapping
	   zq-calibration, if any */
	if (zcal_long)
		cnt -= tegra_dram_dev_num * 256;

	reg = (next_timing->emc_burst_regs[EMC_MRS_WAIT_CNT_INDEX] &
		EMC_MRS_WAIT_CNT_SHORT_WAIT_MASK) >>
		EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT;
	if (cnt < reg)
		cnt = reg;

	reg = (next_timing->emc_burst_regs[EMC_MRS_WAIT_CNT_INDEX] &
		(~EMC_MRS_WAIT_CNT_LONG_WAIT_MASK));
	reg |= (cnt << EMC_MRS_WAIT_CNT_LONG_WAIT_SHIFT) &
		EMC_MRS_WAIT_CNT_LONG_WAIT_MASK;

	emc_writel(reg, EMC_MRS_WAIT_CNT);
}

static inline void set_dram_mode(const struct emc_table *next_timing,
				 const struct emc_table *last_timing,
				 int dll_change)
{
	if (tegra_dram_type == DRAM_TYPE_DDR3) {
		if (next_timing->emc_mode_1 != last_timing->emc_mode_1)
			ccfifo_writel(next_timing->emc_mode_1, EMC_EMRS);
		if (next_timing->emc_mode_2 != last_timing->emc_mode_2)
			ccfifo_writel(next_timing->emc_mode_2, EMC_EMRS2);

		if ((next_timing->emc_mode_reset != last_timing->emc_mode_reset)
			|| (dll_change == DLL_CHANGE_ON)) {
			u32 reg = next_timing->emc_mode_reset &
				(~EMC_MODE_SET_DLL_RESET);
			if (dll_change == DLL_CHANGE_ON) {
				reg |= EMC_MODE_SET_DLL_RESET;
				reg |= EMC_MODE_SET_LONG_CNT;
			}
			ccfifo_writel(reg, EMC_MRS);
		}
	} else {
		if (next_timing->emc_mode_2 != last_timing->emc_mode_2)
			ccfifo_writel(next_timing->emc_mode_2, EMC_MRW2);
		if (next_timing->emc_mode_1 != last_timing->emc_mode_1)
			ccfifo_writel(next_timing->emc_mode_1, EMC_MRW);
		if (next_timing->emc_mode_4 != last_timing->emc_mode_4)
			ccfifo_writel(next_timing->emc_mode_4, EMC_MRW4);
	}
}

static inline void do_clock_change(u32 clk_setting)
{
	int err;

	tegra114_mc_readl(MC_EMEM_ADR_CFG);
	clk_cfg_writel(clk_setting);
	clk_cfg_readl();

	err = wait_for_update(EMC_INTSTATUS, EMC_INTSTATUS_CLKCHANGE_COMPLETE,
		true);
	if (err) {
		pr_err("%s: clock change completion error: %d", __func__, err);
		BUG();
	}
}

static noinline void emc_set_clock(const struct emc_table *next_timing,
				   const struct emc_table *last_timing,
				   u32 clk_setting)
{
	int i, dll_change, pre_wait;
	bool dyn_sref_enabled, zcal_long;

	u32 emc_cfg_reg = emc_readl(EMC_CFG);

	dyn_sref_enabled = emc_cfg_reg & EMC_CFG_DYN_SREF_ENABLE;
	dll_change = get_dll_change(next_timing, last_timing);
	zcal_long = (next_timing->emc_burst_regs[EMC_ZCAL_INTERVAL_INDEX] != 0)
		&& (last_timing->emc_burst_regs[EMC_ZCAL_INTERVAL_INDEX] == 0);

	/* 1. clear clkchange_complete interrupts */
	emc_writel(EMC_INTSTATUS_CLKCHANGE_COMPLETE, EMC_INTSTATUS);

	/* 2. disable dynamic self-refresh and preset dqs vref, then wait for
	   possible self-refresh entry/exit and/or dqs vref settled - waiting
	   before the clock change decreases worst case change stall time */
	pre_wait = 0;
	if (dyn_sref_enabled) {
		emc_cfg_reg &= ~EMC_CFG_DYN_SREF_ENABLE;
		emc_writel(emc_cfg_reg, EMC_CFG);
		pre_wait = 5;		/* 5us+ for self-refresh entry/exit */
	}

	/* 2.5 check dq/dqs vref delay */
	if (dqs_preset(next_timing, last_timing)) {
		if (pre_wait < 3)
			pre_wait = 3;	/* 3us+ for dqs vref settled */
	}
	if (pre_wait) {
		emc_timing_update();
		udelay(pre_wait);
	}

	/* 4. program burst shadow registers */
	for (i = 0; i < next_timing->emc_burst_regs_num; i++) {
		if (!emc_burst_reg_addr[i])
			continue;
		emc_writel(next_timing->emc_burst_regs[i],
			emc_burst_reg_addr[i]);
	}

	for (i = 0; i < next_timing->mc_burst_regs_num; i++) {
		if (!mc_burst_reg_addr[i])
			continue;
		tegra114_mc_writel(next_timing->mc_burst_regs[i],
			mc_burst_reg_addr[i]);
	}

	for (i = 0; i < next_timing->emc_trimmers_num; i++) {
		emc0_writel(next_timing->emc_trimmers_0[i],
			emc_trimmer_offs[i]);
		emc1_writel(next_timing->emc_trimmers_1[i],
			emc_trimmer_offs[i]);
	}

	emc_cfg_reg &= ~EMC_CFG_UPDATE_MASK;
	emc_cfg_reg |= next_timing->emc_cfg & EMC_CFG_UPDATE_MASK;
	emc_writel(emc_cfg_reg, EMC_CFG);
	wmb();
	barrier();

	/* 4.1 On ddr3 when DLL is re-started predict MRS long wait count and
	   overwrite DFS table setting */
	if ((tegra_dram_type == DRAM_TYPE_DDR3)
		&& (dll_change == DLL_CHANGE_ON))
		overwrite_mrs_wait_cnt(next_timing, zcal_long);

	/* 5.2 disable auto-refresh to save time after clock change */
	emc_writel(EMC_REFCTRL_DISABLE_ALL(tegra_dram_dev_num), EMC_REFCTRL);

	/* 6. turn Off dll and enter self-refresh on DDR3 */
	if (tegra_dram_type == DRAM_TYPE_DDR3) {
		if (dll_change == DLL_CHANGE_OFF)
			ccfifo_writel(next_timing->emc_mode_1, EMC_EMRS);
		ccfifo_writel(DRAM_BROADCAST(tegra_dram_dev_num) |
			EMC_SELF_REF_CMD_ENABLED, EMC_SELF_REF);
	}

	/* 7. flow control marker 2 */
	ccfifo_writel(1, EMC_STALL_THEN_EXE_AFTER_CLKCHANGE);

	/* 8. exit self-refresh on DDR3 */
	if (tegra_dram_type == DRAM_TYPE_DDR3)
		ccfifo_writel(DRAM_BROADCAST(tegra_dram_dev_num), EMC_SELF_REF);

	/* 9. set dram mode registers */
	set_dram_mode(next_timing, last_timing, dll_change);

	/* 10. issue zcal command if turning zcal On */
	if (zcal_long) {
		ccfifo_writel(EMC_ZQ_CAL_LONG_CMD_DEV0, EMC_ZQ_CAL);
		if (tegra_dram_dev_num > 1)
			ccfifo_writel(EMC_ZQ_CAL_LONG_CMD_DEV1, EMC_ZQ_CAL);
	}

	/* 10.1 dummy write to RO register to remove stall after change */
	ccfifo_writel(0, EMC_CCFIFO_STATUS);

	/* 11.5 program burst_up_down registers if emc rate is going down */
	if (next_timing->rate < last_timing->rate) {
		for (i = 0; i < next_timing->up_down_regs_num; i++)
			tegra114_mc_writel(next_timing->up_down_regs[i],
				burst_up_down_reg_addr[i]);
		wmb();
	}

	/* 12-14. read any MC register to ensure the programming is done
	   change EMC clock source register wait for clk change completion */
	do_clock_change(clk_setting);

	/* 14.1 re-enable auto-refresh */
	emc_writel(EMC_REFCTRL_ENABLE_ALL(tegra_dram_dev_num), EMC_REFCTRL);

	/* 14.2 program burst_up_down registers if emc rate is going up */
	if (next_timing->rate > last_timing->rate) {
		for (i = 0; i < next_timing->up_down_regs_num; i++)
			tegra114_mc_writel(next_timing->up_down_regs[i],
				burst_up_down_reg_addr[i]);
		wmb();
	}

	/* 16. restore dynamic self-refresh */
	if (next_timing->emc_cfg & EMC_CFG_DYN_SREF_ENABLE) {
		emc_cfg_reg |= EMC_CFG_DYN_SREF_ENABLE;
		emc_writel(emc_cfg_reg, EMC_CFG);
	}

	/* 17. set zcal wait count */
	if (zcal_long)
		emc_writel(next_timing->emc_zcal_cnt_long, EMC_ZCAL_WAIT_CNT);

	/* 18. update restored timing */
	udelay(2);
	emc_timing_update();
}

static void emc_last_stats_update(int last_sel)
{
	unsigned long flags;
	u64 cur_jiffies = get_jiffies_64();

	spin_lock_irqsave(&tegra_emc_stats.spinlock, flags);

	if (tegra_emc_stats.last_sel < TEGRA_EMC_TABLE_MAX_SIZE)
		tegra_emc_stats.time_at_clock[tegra_emc_stats.last_sel] =
			tegra_emc_stats.time_at_clock[tegra_emc_stats.last_sel]
			+ (cur_jiffies - tegra_emc_stats.last_update);

	tegra_emc_stats.last_update = cur_jiffies;

	if (last_sel < TEGRA_EMC_TABLE_MAX_SIZE) {
		tegra_emc_stats.clkchange_count++;
		tegra_emc_stats.last_sel = last_sel;
	}
	spin_unlock_irqrestore(&tegra_emc_stats.spinlock, flags);
}

static int emc_table_lookup(unsigned long rate)
{
	int i;
	i = get_start_idx(rate);
	for (; i < tegra_emc_table_size; i++) {
		if (tegra_emc_clk_sel[i].input == NULL)
			continue;

		if (tegra_emc_table[i].rate == rate)
			break;
	}

	if (i >= tegra_emc_table_size)
		return -EINVAL;
	return i;
}

int tegra114_emc_set_rate(unsigned long rate)
{
	int i;
	u32 clk_setting;
	struct emc_table *last_timing;
	unsigned long flags;
	s64 last_change_delay;

	if (!emc_enable)
		return -ENODEV;

	if (!tegra_emc_table_size)
		return -EINVAL;

	if (rate == tegra114_emc_get_rate())
		return 0;

	i = emc_table_lookup(rate);

	if (IS_ERR(ERR_PTR(i)))
		return i;

	if (!emc_timing) {
		emc_get_timing(&start_timing);
		last_timing = &start_timing;
	} else
		last_timing = emc_timing;

	clk_setting = tegra_emc_clk_sel[i].value;

	last_change_delay = ktime_us_delta(ktime_get(), clkchange_time);
	if ((last_change_delay >= 0) && (last_change_delay < clkchange_delay))
		udelay(clkchange_delay - (int)last_change_delay);

	spin_lock_irqsave(&emc_access_lock, flags);
	emc_set_clock(&tegra_emc_table[i], last_timing, clk_setting);
	clkchange_time = ktime_get();
	emc_timing = &tegra_emc_table[i];
	spin_unlock_irqrestore(&emc_access_lock, flags);

	emc_last_stats_update(i);

	return 0;
}

struct clk *tegra114_emc_predict_parent(unsigned long rate)
{
	int val;
	u32 src_val;

	if (!tegra_emc_table)
		return ERR_PTR(-EINVAL);

	val = emc_table_lookup(rate);
	if (IS_ERR_VALUE(val))
		return ERR_PTR(val);

	src_val = emc_src_val(tegra_emc_table[val].src_sel_reg);
	return tegra_emc_src[src_val];
}

static int find_matching_input(const struct emc_table *table,
				struct emc_sel *emc_clk_sel)
{
	u32 div_value;
	u32 src_value;
	unsigned long input_rate = 0;
	struct clk *input_clk;

	div_value = emc_div_val(table->src_sel_reg);
	src_value = emc_src_val(table->src_sel_reg);

	if (div_value & 0x1) {
		pr_warn("Tegra114: invalid odd divider for EMC rate %lu\n",
			table->rate);
		return -EINVAL;
	}

	if (src_value >= __clk_get_num_parents(emc_clk)) {
		pr_warn("Tegra114: no matching input found for EMC rate %lu\n",
			table->rate);
		return -EINVAL;
	}

	if (div_value && (table->src_sel_reg & EMC_CLK_LOW_JITTER_ENABLE)) {
		pr_warn("Tegra114: invalid LJ path for EMC rate %lu\n",
			table->rate);
		return -EINVAL;
	}

	if (!(table->src_sel_reg & EMC_CLK_MC_SAME_FREQ) !=
		!(MC_EMEM_ARB_MISC0_EMC_SAME_FREQ &
		table->mc_burst_regs[MC_EMEM_ARB_MISC0_INDEX])) {
		pr_warn("Tegra114: ambiguous EMC to MC ratio for rate %lu\n",
			table->rate);
		return -EINVAL;
	}

	input_clk = tegra_emc_src[src_value];
	if (src_value == TEGRA_EMC_SRC_PLLM_UD) {
		input_rate = table->rate * (1 + div_value / 2);
	} else {
		input_rate = __clk_get_rate(input_clk);
		if (input_rate != (table->rate * (1 + div_value / 2))) {
			pr_warn("Tegra114: EMC rate %lu does not match input\n",
				table->rate);
			return -EINVAL;
		}
	}

	if (src_value == TEGRA_EMC_SRC_PLLC) {
		emc_backup_src = tegra_emc_src[TEGRA_EMC_SRC_PLLC];
		emc_backup_rate = table->rate;
	}

	emc_clk_sel->input = input_clk;
	emc_clk_sel->input_rate = input_rate;
	emc_clk_sel->value = table->src_sel_reg;

	return 0;
}

static int purge_emc_table(void)
{
	int i;
	int ret = 0;

	if (emc_backup_src)
		return 0;

	for (i = 0; i < tegra_emc_table_size; i++) {
		struct emc_sel *sel = &tegra_emc_clk_sel[i];
		if (sel->input) {
			if (__clk_get_rate(sel->input) != sel->input_rate) {
				sel->input = NULL;
				sel->input_rate = 0;
				sel->value = 0;
			}
		}
	}
	return ret;
}

static struct device_node *tegra114_emc_sku_devnode(struct device_node *np)
{
	struct device_node *iter;
	struct property *prop;
	const __be32 *p;
	u32 u;

	for_each_child_of_node(np, iter) {
		if (of_find_property(iter, "nvidia,chip-sku", NULL)) {
			of_property_for_each_u32(iter, "nvidia,chip-sku",
				prop, p, u) {
				if (u == tegra_sku_id)
					return iter;
			}
		} else
			return iter;
	}

	return ERR_PTR(-ENODATA);
}

void tegra114_parse_dt_data(struct platform_device *pdev)
{
	struct device_node *np;
	struct device_node *iter;
	struct device_node *tablenode = NULL;
	int i;
	u32 prop;
	int regs_count;
	int ret;

	np = pdev->dev.of_node;
	if (of_find_property(np, "nvidia,use-chip-sku", NULL))
		tablenode = tegra114_emc_sku_devnode(np);
	else
		tablenode = np;

	if (IS_ERR(tablenode))
		return;

	tegra_emc_table_size = 0;
	for_each_child_of_node(tablenode, iter)
		if (of_device_is_compatible(iter, "nvidia,tegra114-emc-table"))
			tegra_emc_table_size++;

	if (!tegra_emc_table_size)
		return;

	tegra_emc_table = devm_kzalloc(&pdev->dev,
		sizeof(struct emc_table) * tegra_emc_table_size, GFP_KERNEL);

	if (!tegra_emc_table)
		return;

	i = 0;
	for_each_child_of_node(tablenode, iter) {
		ret = of_property_read_u8(iter, "nvidia,revision",
			&tegra_emc_table[i].rev);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-src-sel-reg",
			&tegra_emc_table[i].src_sel_reg);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-min-mv",
			&tegra_emc_table[i].emc_min_mv);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-zcal-wait-cnt",
			&tegra_emc_table[i].emc_zcal_cnt_long);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-acal-interval",
			&tegra_emc_table[i].emc_acal_interval);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-cfg",
			&tegra_emc_table[i].emc_cfg);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-mode-reset",
			&tegra_emc_table[i].emc_mode_reset);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-mode-1",
			&tegra_emc_table[i].emc_mode_1);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-mode-2",
			&tegra_emc_table[i].emc_mode_2);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-mode-4",
			&tegra_emc_table[i].emc_mode_4);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-latency",
			&tegra_emc_table[i].clock_change_latency);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-burst-regs-num",
			&tegra_emc_table[i].emc_burst_regs_num);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,mc-burst-regs-num",
			&tegra_emc_table[i].mc_burst_regs_num);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,emc-trimmers-num",
			&tegra_emc_table[i].emc_trimmers_num);
		if (ret)
			continue;
		ret = of_property_read_u32(iter, "nvidia,up-down-regs-num",
			&tegra_emc_table[i].up_down_regs_num);
		if (ret)
			continue;
		regs_count = tegra_emc_table[i].emc_burst_regs_num +
			tegra_emc_table[i].mc_burst_regs_num +
			tegra_emc_table[i].emc_trimmers_num * 2 +
			tegra_emc_table[i].up_down_regs_num;
		tegra_emc_table[i].emc_burst_regs = devm_kzalloc(&pdev->dev,
			sizeof(u32) * regs_count, GFP_KERNEL);
		ret = of_property_read_u32_array(iter, "nvidia,emc-registers",
			tegra_emc_table[i].emc_burst_regs, regs_count);
		if (ret)
			continue;
		tegra_emc_table[i].mc_burst_regs =
			tegra_emc_table[i].emc_burst_regs +
			tegra_emc_table[i].emc_burst_regs_num;
		tegra_emc_table[i].emc_trimmers_0 =
			tegra_emc_table[i].mc_burst_regs +
			tegra_emc_table[i].mc_burst_regs_num;
		tegra_emc_table[i].emc_trimmers_1 =
			tegra_emc_table[i].emc_trimmers_0 +
			tegra_emc_table[i].emc_trimmers_num;
		tegra_emc_table[i].up_down_regs =
			tegra_emc_table[i].emc_trimmers_1 +
			tegra_emc_table[i].emc_trimmers_num;
		ret = of_property_read_u32(iter, "clock-frequency", &prop);
		if (ret)
			continue;
		tegra_emc_table[i].rate = prop;
		i++;
	}
	tegra_emc_table_size = i;
}

int tegra114_init_emc_data(struct platform_device *pdev)
{
	int i;
	bool max_entry = false;
	u32 val;
	unsigned long max_rate;
	unsigned long table_rate;
	unsigned long old_rate;
	int regs_count;

	emc_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(emc_clk)) {
		pr_err("Tegra114: Can not find EMC clock\n");
		return -EINVAL;
	}

	for (i = 0; i < TEGRA_EMC_SRC_COUNT; i++) {
		tegra_emc_src[i] = clk_get_sys(NULL, tegra_emc_src_names[i]);
		if (IS_ERR(tegra_emc_src[i])) {
			pr_err("Tegra114: Can not find EMC source clock\n");
			return -ENODATA;
		}
	}

	tegra_emc_stats.clkchange_count = 0;
	spin_lock_init(&tegra_emc_stats.spinlock);
	tegra_emc_stats.last_update = get_jiffies_64();
	tegra_emc_stats.last_sel = TEGRA_EMC_TABLE_MAX_SIZE;

	tegra_dram_type = (emc_readl(EMC_FBIO_CFG5) & EMC_CFG5_TYPE_MASK)
		>> EMC_CFG5_TYPE_SHIFT;

	tegra_dram_dev_num = (tegra114_mc_readl(MC_EMEM_ADR_CFG) & 0x1) + 1;

	if ((tegra_dram_type != DRAM_TYPE_DDR3)
		&& (tegra_dram_type != DRAM_TYPE_LPDDR2)) {
		pr_err("Tegra114: DRAM not supported\n");
		return -ENODATA;
	}

	tegra114_parse_dt_data(pdev);

	if (!tegra_emc_table_size ||
		tegra_emc_table_size > TEGRA_EMC_TABLE_MAX_SIZE)
		return -EINVAL;

	old_rate = __clk_get_rate(emc_clk);
	max_rate = __clk_get_rate(tegra_emc_src[TEGRA_EMC_SRC_PLLM_UD]);

	for (i = 0; i < tegra_emc_table_size; i++) {
		table_rate = tegra_emc_table[i].rate;
		if (!table_rate)
			continue;

		if (i && (table_rate <= tegra_emc_table[i-1].rate))
			continue;

		if (find_matching_input(&tegra_emc_table[i],
			&tegra_emc_clk_sel[i]))
			continue;

		if (table_rate == old_rate)
			tegra_emc_stats.last_sel = i;

		if (table_rate == max_rate)
			max_entry = true;
	}

	if (!max_entry) {
		pr_err("Tegra114: invalid EMC DFS table\n");
		tegra_emc_table_size = 0;
		return -ENODATA;
	}

	purge_emc_table();

	pr_info("Tegra114: validated EMC DFS table\n");

	val = emc_readl(EMC_CFG_2) & (~EMC_CFG_2_MODE_MASK);
	val |= ((tegra_dram_type == DRAM_TYPE_LPDDR2) ? EMC_CFG_2_PD_MODE :
		EMC_CFG_2_SREF_MODE) << EMC_CFG_2_MODE_SHIFT;
	emc_writel(val, EMC_CFG_2);

	start_timing.emc_burst_regs_num = ARRAY_SIZE(emc_burst_reg_addr);
	start_timing.mc_burst_regs_num = ARRAY_SIZE(mc_burst_reg_addr);
	start_timing.emc_trimmers_num = ARRAY_SIZE(emc_trimmer_offs);
	start_timing.up_down_regs_num = ARRAY_SIZE(burst_up_down_reg_addr);

	regs_count = start_timing.emc_burst_regs_num +
		start_timing.mc_burst_regs_num +
		start_timing.emc_trimmers_num * 2 +
		start_timing.up_down_regs_num;
	start_timing.emc_burst_regs = devm_kzalloc(&pdev->dev,
		sizeof(u32) * regs_count, GFP_KERNEL);

	start_timing.mc_burst_regs = start_timing.emc_burst_regs +
		start_timing.emc_burst_regs_num;
	start_timing.emc_trimmers_0 = start_timing.mc_burst_regs +
		start_timing.mc_burst_regs_num;
	start_timing.emc_trimmers_1 = start_timing.emc_trimmers_0 +
		start_timing.emc_trimmers_num;
	start_timing.up_down_regs = start_timing.emc_trimmers_1 +
		start_timing.emc_trimmers_num;

	return 0;
}

static int tegra114_emc_probe(struct platform_device *pdev)
{
	struct device_node *mc_np;
	struct platform_device *mc_dev;
	struct device_node *car_np;
	struct platform_device *car_dev;
	int ret;

	mc_np = of_parse_phandle(pdev->dev.of_node, "nvidia,mc", 0);
	if (!mc_np) {
		ret = -EINVAL;
		goto out;
	}

	mc_dev = of_find_device_by_node(mc_np);
	if (!mc_dev) {
		ret = -EINVAL;
		goto out;
	}
	if (!mc_dev->dev.driver) {
		ret = -EPROBE_DEFER;
		goto out;
	}

	car_np = of_parse_phandle(pdev->dev.of_node, "clocks", 0);
	if (!car_np) {
		ret = -EINVAL;
		goto out;
	}

	car_dev = of_find_device_by_node(car_np);
	if (!car_dev) {
		ret = -EINVAL;
		goto out;
	}

	tegra_clk_base = of_iomap(car_dev->dev.of_node, 0);
	tegra_emc0_base = of_iomap(pdev->dev.of_node, 0);
	tegra_emc1_base = of_iomap(pdev->dev.of_node, 1);
	tegra_emc_base = of_iomap(pdev->dev.of_node, 2);

	ret = tegra114_init_emc_data(pdev);

out:
	if (mc_np)
		of_node_put(mc_np);
	if (car_np)
		of_node_put(car_np);

	tegra_emc_ready = true;
	return ret;
}

static struct of_device_id tegra114_emc_of_match[] = {
	{ .compatible = "nvidia,tegra114-emc", },
	{ },
};

static struct platform_driver tegra114_emc_driver = {
	.driver         = {
		.name   = "tegra114-emc",
		.owner  = THIS_MODULE,
		.of_match_table = tegra114_emc_of_match,
	},
	.probe          = tegra114_emc_probe,
};

static int __init tegra114_emc_init(void)
{
	return platform_driver_register(&tegra114_emc_driver);
}
device_initcall(tegra114_emc_init);

#ifdef CONFIG_DEBUG_FS

static int emc_stats_show(struct seq_file *s, void *data)
{
	int i;

	emc_last_stats_update(TEGRA_EMC_TABLE_MAX_SIZE);

	seq_printf(s, "%-10s %-10s\n", "rate kHz", "time");
	for (i = 0; i < tegra_emc_table_size; i++) {
		if (tegra_emc_clk_sel[i].input == NULL)
			continue;

		seq_printf(s, "%-10lu %-10llu\n", tegra_emc_table[i].rate,
			cputime64_to_clock_t(tegra_emc_stats.time_at_clock[i]));
	}
	seq_printf(s, "%-15s %llu\n", "transitions:",
		tegra_emc_stats.clkchange_count);
	seq_printf(s, "%-15s %llu\n", "time-stamp:",
		cputime64_to_clock_t(tegra_emc_stats.last_update));

	return 0;
}

static int emc_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, emc_stats_show, inode->i_private);
}

static const struct file_operations emc_stats_fops = {
	.open		= emc_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init tegra_emc_debug_init(void)
{
	struct dentry *emc_debugfs_root;

	emc_debugfs_root = debugfs_create_dir("tegra_emc", NULL);
	if (!emc_debugfs_root)
		return -ENOMEM;

	if (!debugfs_create_file(
		"stats", S_IRUGO, emc_debugfs_root, NULL, &emc_stats_fops))
		goto err_out;

	if (!debugfs_create_u32("clkchange_delay", S_IRUGO | S_IWUSR,
		emc_debugfs_root, (u32 *)&clkchange_delay))
		goto err_out;

	return 0;

err_out:
	debugfs_remove_recursive(emc_debugfs_root);
	return -ENOMEM;
}

late_initcall(tegra_emc_debug_init);
#endif
