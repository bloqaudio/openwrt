// SPDX-License-Identifier: GPL-2.0-only

#include <linux/bitfield.h>
#include <linux/of.h>
#include <linux/phy.h>
#include <net/dsa.h>
#include <asm/mach-rtl-otto/mach-rtl-otto.h>

#include "flowctrl.h"
#include "rtl-otto.h"

#define RTL930X_FC_PORT_ACT_CTRL(p)		(0xd804 + ((p) << 2))
#define RTL930X_FC_GLB_SYS_UTIL_THR		0xd878
#define RTL930X_FC_GLB_DROP_THR			0xd87c
#define RTL930X_FC_GLB_HI_THR			0xd880
#define RTL930X_FC_GLB_LO_THR			0xd884
#define RTL930X_FC_GLB_FCOFF_HI_THR		0xd888
#define RTL930X_FC_GLB_FCOFF_LO_THR		0xd88c
#define RTL930X_FC_JUMBO_HI_THR			0xd890
#define RTL930X_FC_JUMBO_LO_THR			0xd894
#define RTL930X_FC_JUMBO_FCOFF_HI_THR		0xd898
#define RTL930X_FC_JUMBO_FCOFF_LO_THR		0xd89c
#define RTL930X_FC_JUMBO_THR_ADJUST		0xd8a0
#define RTL930X_FC_PORT_HI_THR(set)		(0xd8a4 + ((set) << 2))
#define RTL930X_FC_PORT_LO_THR(set)		(0xd8b4 + ((set) << 2))
#define RTL930X_FC_PORT_FCOFF_HI_THR(set)	(0xd8c4 + ((set) << 2))
#define RTL930X_FC_PORT_FCOFF_LO_THR(set)	(0xd8d4 + ((set) << 2))
#define RTL930X_FC_PORT_GUAR_THR(set)		(0xd8e4 + ((set) << 2))
#define RTL930X_FC_PORT_THR_SET_SEL		0xd8f4
#define RTL930X_FC_PORT_EGR_DROP_CTRL(p)	(0xc380 + ((p) << 2))
#define RTL930X_FC_CPU_Q_EGR_FORCE_DROP_CTRL	0xc4dc
#define RTL930X_FC_Q_EGR_DROP_THR(q, set)	(0x791c + ((((q) << 2) + (set)) << 2))
#define RTL930X_FC_CPU_Q_EGR_DROP_THR(q)	(0x7dc0 + ((q) << 2))
#define RTL930X_FC_PORT_EGR_DROP_THR_SET_SEL	0x79dc
#define RTL930X_FC_LB_PORT_Q_EGR_DROP_THR	0x79e4
#define RTL930X_SC_P_CTRL			0x7a98

#define RTL930X_FC_THR				GENMASK(11, 0)
#define RTL930X_FC_ON				GENMASK(27, 16)
#define RTL930X_FC_OFF				GENMASK(11, 0)
#define RTL930X_FC_REF_RXCNGST			BIT(1)

#define RTL930X_FC_THR_SETS			4
#define RTL930X_FC_QUEUES			12
#define RTL930X_FC_CPU_QUEUES			32

enum rtldsa_930x_fc_model {
	RTL930X_FC_24G_4XG,
	RTL930X_FC_8XG,
	RTL930X_FC_48G_CASCADE,
	RTL930X_FC_24X2G5_2XG,
	RTL930X_FC_MODELS,
};

struct rtldsa_930x_fc_pair {
	u16 on[RTL930X_FC_MODELS];
	u16 off[RTL930X_FC_MODELS];
};

struct rtldsa_930x_fc_glb {
	u32 reg;
	struct rtldsa_930x_fc_pair thr;
};

struct rtldsa_930x_fc_port_set {
	struct rtldsa_930x_fc_pair hi;
	struct rtldsa_930x_fc_pair lo;
	u16 guar[RTL930X_FC_MODELS];
};

static const u16 rtldsa_930x_fc_glb_drop[RTL930X_FC_MODELS] = {
	0xfe4, 0xff8, 0xfc8, 0xfe4
};

static const u16 rtldsa_930x_fc_jumbo_sys_used[RTL930X_FC_MODELS] = {
	0x540, 0xdc8, 0xa80, 0x540
};

static const u16 rtldsa_930x_fc_allow_pages[RTL930X_FC_MODELS] = {
	0x32, 0x32, 0x32, 0x34
};

static const struct rtldsa_930x_fc_glb rtldsa_930x_fc_glb[] = {
	{ RTL930X_FC_GLB_HI_THR,
	  { { 0x890, 0xa20, 0x542, 0x890 }, { 0x778, 0x9d0, 0x50e, 0x778 } } },
	{ RTL930X_FC_GLB_LO_THR,
	  { { 0x5d4, 0x958, 0x4a6, 0x5d4 }, { 0x4bc, 0x908, 0x472, 0x4bc } } },
	{ RTL930X_FC_GLB_FCOFF_HI_THR,
	  { { 0x890, 0xa20, 0x542, 0x890 }, { 0x778, 0x9d0, 0x50e, 0x778 } } },
	{ RTL930X_FC_GLB_FCOFF_LO_THR,
	  { { 0x5d4, 0x958, 0x4a6, 0x5d4 }, { 0x4bc, 0x908, 0x472, 0x4bc } } },
	{ RTL930X_FC_JUMBO_HI_THR,
	  { { 0x628, 0xd30, 0x542, 0x628 }, { 0x510, 0xce0, 0x50e, 0x510 } } },
	{ RTL930X_FC_JUMBO_LO_THR,
	  { { 0x36c, 0xc68, 0x4a6, 0x36c }, { 0x254, 0xc18, 0x472, 0x254 } } },
	{ RTL930X_FC_JUMBO_FCOFF_HI_THR,
	  { { 0x628, 0xd30, 0x542, 0x628 }, { 0x510, 0xce0, 0x50e, 0x510 } } },
	{ RTL930X_FC_JUMBO_FCOFF_LO_THR,
	  { { 0x36c, 0xc68, 0x4a6, 0x36c }, { 0x254, 0xc18, 0x472, 0x254 } } },
	{ RTL930X_FC_LB_PORT_Q_EGR_DROP_THR,
	  { { 0x4e, 0x4e, 0x4e, 0x4e }, { 0x44, 0x44, 0x44, 0x44 } } },
	{ RTL930X_SC_P_CTRL,
	  { { 0xc8, 0x140, 0x32, 0xc8 }, { 0xc8, 0x140, 0x32, 0xc8 } } },
};

static const struct rtldsa_930x_fc_port_set rtldsa_930x_fc_port_sets[RTL930X_FC_THR_SETS] = {
	{ { { 0xab, 0x11b, 0x64, 0xab }, { 0xa1, 0x111, 0x5a, 0xa1 } },
	  { { 0x28, 0xa0, 0x19, 0x28 }, { 0x1e, 0x78, 0x0f, 0x1e } },
	  { 0x10, 0x10, 0x0c, 0x10 } },
	{ { { 0xab, 0x11b, 0x3ad, 0xab }, { 0xa1, 0x111, 0x35f, 0xa1 } },
	  { { 0x28, 0xa0, 0x32b, 0x28 }, { 0x1e, 0x78, 0x2dd, 0x1e } },
	  { 0x10, 0x10, 0x0c, 0x10 } },
	{ { { 0x2ac, 0x11b, 0x64, 0x2ac }, { 0x284, 0x111, 0x5a, 0x284 } },
	  { { 0xa0, 0xa0, 0x19, 0xa0 }, { 0x78, 0x78, 0x0f, 0x78 } },
	  { 0x10, 0x10, 0x0c, 0x10 } },
	{ { { 0x5e, 0x17d, 0x64, 0x5e }, { 0x54, 0x173, 0x5a, 0x54 } },
	  { { 0x23, 0x23, 0x19, 0x23 }, { 0x19, 0x19, 0x0f, 0x19 } },
	  { 0x10, 0x10, 0x0c, 0x10 } },
};

static const struct rtldsa_930x_fc_pair rtldsa_930x_fc_queue_sets[RTL930X_FC_THR_SETS] = {
	{ { 0x4e, 0x4e, 0x4e, 0x4e }, { 0x44, 0x44, 0x44, 0x44 } },
	{ { 0x4e, 0x118, 0x30c, 0x4e }, { 0x44, 0xf0, 0x2a8, 0x44 } },
	{ { 0x118, 0x4e, 0x4e, 0x118 }, { 0xf0, 0x44, 0x44, 0xf0 } },
	{ { 0x64, 0x64, 0x4e, 0x64 }, { 0x44, 0x44, 0x44, 0x44 } },
};

static const struct rtldsa_930x_fc_pair rtldsa_930x_fc_cpu_queue = {
	{ 0x4e, 0x4e, 0x4e, 0x4e }, { 0x44, 0x44, 0x44, 0x44 }
};

static void rtldsa_930x_fc_pair_write(u32 reg, const struct rtldsa_930x_fc_pair *pair, int model)
{
	sw_w32_mask(RTL930X_FC_ON | RTL930X_FC_OFF,
		    FIELD_PREP(RTL930X_FC_ON, pair->on[model]) |
		    FIELD_PREP(RTL930X_FC_OFF, pair->off[model]), reg);
}

static void rtldsa_930x_fc_set_select(u32 base, int port, int set)
{
	int shift = (port % 16) * 2;

	sw_w32_mask(0x3 << shift, set << shift, base + (port / 16) * 4);
}

static int rtldsa_fc_port_speed(const struct dsa_port *dp)
{
	struct phy_device *phydev = dp->user ? dp->user->phydev : NULL;

	if (of_property_present(dp->dn, "sfp"))
		return SPEED_10000;

	if (!phydev)
		return SPEED_UNKNOWN;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT, phydev->supported))
		return SPEED_10000;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported))
		return SPEED_2500;

	return SPEED_1000;
}

void rtldsa_930x_flowctrl_init(struct rtl838x_switch_priv *priv)
{
	int cpu_port = priv->r->cpu_port;
	int set_10g = 2, model;
	struct dsa_port *dp;

	switch (soc_info.id) {
	case 0x9301:
		model = RTL930X_FC_24G_4XG;
		break;
	case 0x9303:
		model = RTL930X_FC_8XG;
		set_10g = 1;
		break;
	default:
		model = RTL930X_FC_24X2G5_2XG;
		break;
	}

	mutex_lock(&priv->reg_mutex);

	/* 2.5G ports keep the reset selection, like the vendor code leaves them */
	dsa_switch_for_each_user_port(dp, priv->ds) {
		int speed = rtldsa_fc_port_speed(dp);
		int set;

		if (speed == SPEED_10000)
			set = set_10g;
		else if (speed == SPEED_1000)
			set = 0;
		else
			continue;

		rtldsa_930x_fc_set_select(RTL930X_FC_PORT_THR_SET_SEL, dp->index, set);
		rtldsa_930x_fc_set_select(RTL930X_FC_PORT_EGR_DROP_THR_SET_SEL, dp->index, set);
	}

	sw_w32_mask(RTL930X_FC_REF_RXCNGST, 0, RTL930X_FC_PORT_EGR_DROP_CTRL(cpu_port));
	rtldsa_930x_fc_set_select(RTL930X_FC_PORT_THR_SET_SEL, cpu_port, 0);

	sw_w32_mask(RTL930X_FC_THR, rtldsa_930x_fc_glb_drop[model], RTL930X_FC_GLB_DROP_THR);
	sw_w32_mask(RTL930X_FC_THR, rtldsa_930x_fc_jumbo_sys_used[model],
		    RTL930X_FC_JUMBO_THR_ADJUST);

	for (int i = 0; i < ARRAY_SIZE(rtldsa_930x_fc_glb); i++)
		rtldsa_930x_fc_pair_write(rtldsa_930x_fc_glb[i].reg,
					  &rtldsa_930x_fc_glb[i].thr, model);

	dsa_switch_for_each_user_port(dp, priv->ds)
		sw_w32_mask(RTL930X_FC_THR, rtldsa_930x_fc_allow_pages[model],
			    RTL930X_FC_PORT_ACT_CTRL(dp->index));

	for (int set = 0; set < RTL930X_FC_THR_SETS; set++) {
		const struct rtldsa_930x_fc_port_set *ps = &rtldsa_930x_fc_port_sets[set];

		rtldsa_930x_fc_pair_write(RTL930X_FC_PORT_HI_THR(set), &ps->hi, model);
		rtldsa_930x_fc_pair_write(RTL930X_FC_PORT_LO_THR(set), &ps->lo, model);
		rtldsa_930x_fc_pair_write(RTL930X_FC_PORT_FCOFF_HI_THR(set), &ps->hi, model);
		rtldsa_930x_fc_pair_write(RTL930X_FC_PORT_FCOFF_LO_THR(set), &ps->lo, model);
		sw_w32_mask(RTL930X_FC_THR, ps->guar[model], RTL930X_FC_PORT_GUAR_THR(set));

		for (int queue = 0; queue < RTL930X_FC_QUEUES; queue++)
			rtldsa_930x_fc_pair_write(RTL930X_FC_Q_EGR_DROP_THR(queue, set),
						  &rtldsa_930x_fc_queue_sets[set], model);
	}

	for (int queue = 0; queue < RTL930X_FC_CPU_QUEUES; queue++)
		rtldsa_930x_fc_pair_write(RTL930X_FC_CPU_Q_EGR_DROP_THR(queue),
					  &rtldsa_930x_fc_cpu_queue, model);
	sw_w32(GENMASK(RTL930X_FC_CPU_QUEUES - 1, 0), RTL930X_FC_CPU_Q_EGR_FORCE_DROP_CTRL);

	sw_w32_mask(RTL930X_FC_THR, 1, RTL930X_FC_GLB_SYS_UTIL_THR);

	mutex_unlock(&priv->reg_mutex);
}
