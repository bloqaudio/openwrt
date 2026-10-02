// SPDX-License-Identifier: GPL-2.0-only

#include <net/dsa.h>
#include <linux/dcbnl.h>
#include <linux/delay.h>
#include <net/pkt_cls.h>
#include <net/pkt_sched.h>
#include <asm/mach-rtl-otto/mach-rtl-otto.h>

#include "qos.h"
#include "rtl-otto.h"

#define RTL839X_TBL_ACCESS_DATA_2(i)		(0x6120 + (((i) << 2)))

#define RTL838X_SCHED_P_EGR_RATE_CTRL(p)	(0xC008 + (((p) << 7)))

/* QoS */
#define RTL838X_QM_INTPRI2QID_CTRL		(0x5F00)
#define RTL839X_QM_INTPRI2QID_CTRL(q)		(0x1110 + (q << 2))
#define RTL839X_QM_PORT_QNUM(p)			(0x1130 + (((p / 10) << 2)))
#define RTL839X_PRI_SEL_PORT_PRI(p)		(0x10A8 + (((p / 10) << 2)))
#define RTL838X_QM_PKT2CPU_INTPRI_MAP		(0x5F10)
#define RTL839X_QM_PKT2CPU_INTPRI_MAP		(0x1154)
#define RTL838X_PRI_SEL_TBL_CTRL(i)		(0x5FD8 + (((i) << 2)))
#define RTL839X_PRI_SEL_TBL_CTRL(i)		(0x10D0 + (((i) << 2)))
#define RTL838X_QM_PKT2CPU_INTPRI_1		(0x5F08)
#define RTL838X_QM_PKT2CPU_INTPRI_2		(0x5F0C)
#define RTL839X_OAM_CTRL			(0x2100)
#define RTL839X_OAM_PORT_ACT_CTRL(p)		(0x2104 + (((p) << 2)))
#define RTL839X_RMK_PORT_DEI_TAG_CTRL(p)	(0x6A9C + (((p >> 5) << 2)))
#define RTL839X_PRI_SEL_IPRI_REMAP		(0x1080)
#define RTL838X_PRI_SEL_IPRI_REMAP		(0x5F8C)
#define RTL839X_PRI_SEL_DEI2DP_REMAP		(0x10EC)
#define RTL839X_RMK_DEI_CTRL			(0x6AA4)
#define RTL839X_WRED_PORT_THR_CTRL(i)		(0x6084 + ((i) << 2))
#define RTL839X_WRED_QUEUE_THR_CTRL(q, i)	(0x6090 + ((q) * 12) + ((i) << 2))
#define RTL838X_PRI_DSCP_INVLD_CTRL0		(0x5FE8)
#define RTL838X_RMK_IPRI_CTRL			(0xA460)
#define RTL838X_RMK_OPRI_CTRL			(0xA464)
#define RTL838X_SCHED_P_TYPE_CTRL(p)		(0xC04C + (((p) << 7)))
#define RTL838X_SCHED_LB_CTRL(p)		(0xC004 + (((p) << 7)))
#define RTL838X_FC_P_EGR_DROP_CTRL(p)		(0x6B1C + (((p) << 2)))

#define RTL930X_PRI_SEL_TBL_CTRL(group)		(0x9B28 + ((group) * 4))
#define RTL931X_PRI_SEL_TBL_CTRL(group)		(0x9074 + ((group) * 8))

#define RTL93XX_PRI_SEL_TBL_CTRL_OTAG_MASK	GENMASK(19, 16)
#define RTL93XX_PRI_SEL_TBL_CTRL_ITAG_MASK	GENMASK(15, 12)
#define RTL93XX_PRI_SEL_TBL_CTRL_DSCP_MASK	GENMASK(11, 8)
#define RTL93XX_PRI_SEL_TBL_CTRL_PORT_MASK	GENMASK(3, 0)

/* port: 0-23, index: 0-7 */
#define RTL930X_SCHED_PORT_Q_CTRL_SET0(port, index) \
						(0x3D48 + ((port) * 384) + ((index) * 4))
/* port: 24-27, index: 0-11 */
#define RTL930X_SCHED_PORT_Q_CTRL_SET1(port, index) \
						((0xE860 + ((port) - 24) * 48) + ((index) * 4))
/* port: 0-51, index: 0-7 */
#define RTL931X_SCHED_PORT_Q_CTRL_SET0(port, index) \
						(0x2888 + ((port) << 5) + ((index) * 4))
/* port: 52-55, index: 0-11 */
#define RTL931X_SCHED_PORT_Q_CTRL_SET1(port, index) \
						((0x2F08 + ((port) - 52) * 48) + ((index) * 4))

#define RTL930X_QM_INTPRI2QID_CTRL		(0xA320)
#define RTL930X_QM_CPUQID2QID_CTRL		(0xA324)
#define RTL930X_QM_CPUQID2XGQID_CTRL		(0xA334)
#define RTL931X_QM_INTPRI2QID_CTRL		(0xA9D0)
#define RTL931X_QM_CPUQID2QID_CTRL		(0xA9D4)
#define RTL931X_QM_CPUQID2XGSQID_CTRL		(0xA9E4)

#define DSCP_MAP_MAX 64
#define RTLDSA_RED_PAGE_SIZE			256
#define RTLDSA_RED_RATE_SCALE			1023
#define RTLDSA_RED_RATE_MAX			255
#define RTLDSA_RED_QUEUES			12

#define RTL930X_SCHED_Q_STRICT			BIT(7)
#define RTL931X_SCHED_Q_STRICT			BIT(8)
#define RTL93XX_SCHED_Q_WEIGHT			GENMASK(6, 0)

enum scheduler_type {
	WEIGHTED_FAIR_QUEUE = 0,
	WEIGHTED_ROUND_ROBIN,
};

struct rtldsa_trust {
	u8 nsel;
	u8 sel[2];
};

/* One entry for each of the four priority selection groups of RTL93xx */
static const struct rtldsa_trust rtldsa_93xx_trust[] = {
	{ 2, { DCB_APP_SEL_PCP, IEEE_8021QAZ_APP_SEL_DSCP } },
	{ 1, { DCB_APP_SEL_PCP } },
	{ 1, { IEEE_8021QAZ_APP_SEL_DSCP } },
	{ 0 },
};

static int rtldsa_max_available_queue[] = {0, 1, 2, 3, 4, 5, 6, 7};
static int rtldsa_default_queue_weights[] = {1, 1, 1, 1, 1, 1, 1, 1};
static int dot1p_priority_remapping[] = {0, 1, 2, 3, 4, 5, 6, 7};

static void rtl839x_read_scheduling_table(int port)
{
	u32 cmd = 1 << 9 | /* Execute cmd */
		  0 << 8 | /* Read */
		  0 << 6 | /* Table type 0b00 */
		  (port & 0x3f);
	rtl839x_exec_tbl2_cmd(cmd);
}

static void rtl839x_write_scheduling_table(int port)
{
	u32 cmd = 1 << 9 | /* Execute cmd */
		  1 << 8 | /* Write */
		  0 << 6 | /* Table type 0b00 */
		  (port & 0x3f);
	rtl839x_exec_tbl2_cmd(cmd);
}

static void rtl839x_read_out_q_table(int port)
{
	u32 cmd = 1 << 9 | /* Execute cmd */
		  0 << 8 | /* Read */
		  2 << 6 | /* Table type 0b10 */
		  (port & 0x3f);
	rtl839x_exec_tbl2_cmd(cmd);
}

u32 rtldsa_838x_get_egress_rate(struct rtl838x_switch_priv *priv, int port)
{
	if (port > priv->r->cpu_port)
		return 0;

	return sw_r32(RTL838X_SCHED_P_EGR_RATE_CTRL(port)) & 0x3fff;
}

/* Sets the rate limit, 10MBit/s is equal to a rate value of 625 */
int rtldsa_838x_set_egress_rate(struct rtl838x_switch_priv *priv, int port, u32 rate)
{
	u32 old_rate;

	if (port > priv->r->cpu_port)
		return -1;

	old_rate = sw_r32(RTL838X_SCHED_P_EGR_RATE_CTRL(port));
	sw_w32(rate, RTL838X_SCHED_P_EGR_RATE_CTRL(port));

	return old_rate;
}

/* Sets the rate limit, 10MBit/s is equal to a rate value of 625 */
u32 rtldsa_839x_get_egress_rate(struct rtl838x_switch_priv *priv, int port)
{
	u32 rate;

	if (port >= priv->r->cpu_port)
		return 0;

	mutex_lock(&priv->reg_mutex);

	rtl839x_read_scheduling_table(port);

	rate = sw_r32(RTL839X_TBL_ACCESS_DATA_2(7));
	rate <<= 12;
	rate |= sw_r32(RTL839X_TBL_ACCESS_DATA_2(8)) >> 20;

	mutex_unlock(&priv->reg_mutex);
	pr_debug("%s: Getting egress rate on port %d is %d\n", __func__, port, rate);

	return rate;
}

/* Sets the rate limit, 10MBit/s is equal to a rate value of 625, returns previous rate */
int rtldsa_839x_set_egress_rate(struct rtl838x_switch_priv *priv, int port, u32 rate)
{
	u32 old_rate;

	pr_debug("%s: Setting egress rate on port %d to %d\n", __func__, port, rate);
	if (port >= priv->r->cpu_port)
		return -1;

	mutex_lock(&priv->reg_mutex);

	rtl839x_read_scheduling_table(port);

	old_rate = sw_r32(RTL839X_TBL_ACCESS_DATA_2(7)) & 0xff;
	old_rate <<= 12;
	old_rate |= sw_r32(RTL839X_TBL_ACCESS_DATA_2(8)) >> 20;
	sw_w32_mask(0xff, (rate >> 12) & 0xff, RTL839X_TBL_ACCESS_DATA_2(7));
	sw_w32_mask(0xfff << 20, rate << 20, RTL839X_TBL_ACCESS_DATA_2(8));

	rtl839x_write_scheduling_table(port);

	mutex_unlock(&priv->reg_mutex);

	return old_rate;
}

static void rtl838x_setup_prio2queue_matrix(int *min_queues)
{
	u32 v = 0;

	pr_info("Current Intprio2queue setting: %08x\n", sw_r32(RTL838X_QM_INTPRI2QID_CTRL));
	for (int i = 0; i < MAX_PRIOS; i++)
		v |= i << (min_queues[i] * 3);
	sw_w32(v, RTL838X_QM_INTPRI2QID_CTRL);
}

static void rtl839x_setup_prio2queue_matrix(int *min_queues)
{
	pr_info("Current Intprio2queue setting: %08x\n", sw_r32(RTL839X_QM_INTPRI2QID_CTRL(0)));
	for (int i = 0; i < MAX_PRIOS; i++) {
		int q = min_queues[i];

		sw_w32(i << (q * 3), RTL839X_QM_INTPRI2QID_CTRL(q));
	}
}

/* Sets the CPU queue depending on the internal priority of a packet */
static void rtl83xx_setup_prio2queue_cpu_matrix(int reg)
{
	u32 v = 0;

	pr_info("QM_PKT2CPU_INTPRI_MAP: %08x\n", sw_r32(reg));
	for (int i = 0; i < MAX_PRIOS; i++)
		v |= rtldsa_max_available_queue[i] << (i * 3);
	sw_w32(v, reg);
}

static void rtl838x_setup_default_prio2queue(void)
{
	rtl838x_setup_prio2queue_matrix(rtldsa_max_available_queue);
	rtl83xx_setup_prio2queue_cpu_matrix(RTL838X_QM_PKT2CPU_INTPRI_MAP);
}

static void rtl839x_setup_default_prio2queue(void)
{
	rtl839x_setup_prio2queue_matrix(rtldsa_max_available_queue);
	rtl83xx_setup_prio2queue_cpu_matrix(RTL839X_QM_PKT2CPU_INTPRI_MAP);
}

/* Sets the output queue assigned to a port, the port can be the CPU-port */
static void rtl839x_set_egress_queue(int port, int queue)
{
	u32 shift = (port % 10) * 3;
	u32 mask = 0x7 << shift;

	sw_w32_mask(mask, (queue & 0x7) << shift, RTL839X_QM_PORT_QNUM(port));
}

/* Sets the priority assigned of an ingress port, the port can be the CPU-port */
static void rtldsa_839x_set_ingress_priority(int port, int priority)
{
	int shift = ((port % 10) * 3);

	sw_w32_mask(7 << shift, priority << shift, RTL839X_PRI_SEL_PORT_PRI(port));
}

static int rtl839x_get_scheduling_algorithm(struct rtl838x_switch_priv *priv, int port)
{
	u32 v;

	mutex_lock(&priv->reg_mutex);

	rtl839x_read_scheduling_table(port);
	v = sw_r32(RTL839X_TBL_ACCESS_DATA_2(8));

	mutex_unlock(&priv->reg_mutex);

	if (v & BIT(19))
		return WEIGHTED_ROUND_ROBIN;

	return WEIGHTED_FAIR_QUEUE;
}

static void rtl839x_set_scheduling_algorithm(struct rtl838x_switch_priv *priv, int port,
					     enum scheduler_type sched)
{
	enum scheduler_type t = rtl839x_get_scheduling_algorithm(priv, port);
	u32 v, oam_state, oam_port_state;
	u32 count;
	int i, egress_rate;

	mutex_lock(&priv->reg_mutex);
	/* Check whether we need to empty the egress queue of that port due to Errata E0014503 */
	if (sched == WEIGHTED_FAIR_QUEUE && t == WEIGHTED_ROUND_ROBIN && port != priv->r->cpu_port) {
		/* Read Operations, Adminstatrion and Management control register */
		oam_state = sw_r32(RTL839X_OAM_CTRL);

		/* Get current OAM state */
		oam_port_state = sw_r32(RTL839X_OAM_PORT_ACT_CTRL(port));

		/* Disable OAM to block traffice */
		v = sw_r32(RTL839X_OAM_CTRL);
		sw_w32_mask(0, 1, RTL839X_OAM_CTRL);
		v = sw_r32(RTL839X_OAM_CTRL);

		/* Set to trap action OAM forward (bits 1, 2) and OAM Mux Action Drop (bit 0) */
		sw_w32(0x2, RTL839X_OAM_PORT_ACT_CTRL(port));

		/* Set port egress rate to unlimited */
		egress_rate = rtldsa_839x_set_egress_rate(priv, port, 0xFFFFF);

		/* Wait until the egress used page count of that port is 0 */
		i = 0;
		do {
			usleep_range(100, 200);
			rtl839x_read_out_q_table(port);
			count = sw_r32(RTL839X_TBL_ACCESS_DATA_2(6));
			count >>= 20;
			i++;
		} while (i < 3500 && count > 0);
	}

	/* Actually set the scheduling algorithm */
	rtl839x_read_scheduling_table(port);
	sw_w32_mask(BIT(19), sched ? BIT(19) : 0, RTL839X_TBL_ACCESS_DATA_2(8));
	rtl839x_write_scheduling_table(port);

	if (sched == WEIGHTED_FAIR_QUEUE && t == WEIGHTED_ROUND_ROBIN && port != priv->r->cpu_port) {
		/* Restore OAM state to control register */
		sw_w32(oam_state, RTL839X_OAM_CTRL);

		/* Restore trap action state */
		sw_w32(oam_port_state, RTL839X_OAM_PORT_ACT_CTRL(port));

		/* Restore port egress rate */
		rtldsa_839x_set_egress_rate(priv, port, egress_rate);
	}

	mutex_unlock(&priv->reg_mutex);
}

static void rtl839x_set_scheduling_queue_weights(struct rtl838x_switch_priv *priv, int port,
						 int *queue_weights)
{
	mutex_lock(&priv->reg_mutex);

	rtl839x_read_scheduling_table(port);

	for (int i = 0; i < 8; i++) {
		int lsb = 48 + i * 8;
		int low_byte = 8 - (lsb >> 5);
		int start_bit = lsb - (low_byte << 5);
		int high_mask = 0x3ff >> (32 - start_bit);

		sw_w32_mask(0x3ff << start_bit, (queue_weights[i] & 0x3ff) << start_bit,
			    RTL839X_TBL_ACCESS_DATA_2(low_byte));
		if (high_mask)
			sw_w32_mask(high_mask, (queue_weights[i] & 0x3ff) >> (32 - start_bit),
				    RTL839X_TBL_ACCESS_DATA_2(low_byte - 1));
	}

	rtl839x_write_scheduling_table(port);
	mutex_unlock(&priv->reg_mutex);
}

void rtldsa_838x_qos_init(struct rtl838x_switch_priv *priv)
{
	u32 v;

	pr_info("Setting up RTL838X QoS\n");
	pr_info("RTL838X_PRI_SEL_TBL_CTRL(i): %08x\n", sw_r32(RTL838X_PRI_SEL_TBL_CTRL(0)));
	rtl838x_setup_default_prio2queue();

	/* Enable inner (bit 12) and outer (bit 13) priority remapping from DSCP */
	sw_w32_mask(0, BIT(12) | BIT(13), RTL838X_PRI_DSCP_INVLD_CTRL0);

	/* Set default weight for calculating internal priority, in prio selection group 0
	 * Port based (prio 3), Port outer-tag (4), DSCP (5), Inner Tag (6), Outer Tag (7)
	 */
	v = 3 | (4 << 3) | (5 << 6) | (6 << 9) | (7 << 12);
	sw_w32(v, RTL838X_PRI_SEL_TBL_CTRL(0));

	/* Set the inner and outer priority one-to-one to re-marked outer dot1p priority */
	v = 0;
	for (int p = 0; p < 8; p++)
		v |= p << (3 * p);
	sw_w32(v, RTL838X_RMK_OPRI_CTRL);
	sw_w32(v, RTL838X_RMK_IPRI_CTRL);

	v = 0;
	for (int p = 0; p < 8; p++)
		v |= (dot1p_priority_remapping[p] & 0x7) << (p * 3);
	sw_w32(v, RTL838X_PRI_SEL_IPRI_REMAP);

	/* On all ports set scheduler type to WFQ */
	for (int i = 0; i <= priv->r->cpu_port; i++)
		sw_w32(0, RTL838X_SCHED_P_TYPE_CTRL(i));

	/* Enable egress scheduler for CPU-Port */
	sw_w32_mask(0, BIT(8), RTL838X_SCHED_LB_CTRL(priv->r->cpu_port));

	/* Enable egress drop allways on */
	sw_w32_mask(0, BIT(11), RTL838X_FC_P_EGR_DROP_CTRL(priv->r->cpu_port));

	/* Give special trap frames priority 7 (BPDUs) and routing exceptions: */
	sw_w32_mask(0, 7 << 3 | 7, RTL838X_QM_PKT2CPU_INTPRI_2);
	/* Give RMA frames priority 7: */
	sw_w32_mask(0, 7, RTL838X_QM_PKT2CPU_INTPRI_1);
}

void rtldsa_839x_qos_init(struct rtl838x_switch_priv *priv)
{
	u32 v;

	pr_info("Setting up RTL839X QoS\n");
	pr_info("RTL839X_PRI_SEL_TBL_CTRL(i): %08x\n", sw_r32(RTL839X_PRI_SEL_TBL_CTRL(0)));
	rtl839x_setup_default_prio2queue();

	for (int port = 0; port <= priv->r->cpu_port; port++)
		rtl839x_set_egress_queue(port, 7);

	for (int port = 0; port <= priv->r->cpu_port; port++) {
		rtldsa_839x_set_ingress_priority(port, 0);
		rtl839x_set_scheduling_algorithm(priv, port, WEIGHTED_FAIR_QUEUE);
		rtl839x_set_scheduling_queue_weights(priv, port, rtldsa_default_queue_weights);
		/* Do re-marking based on outer tag */
		sw_w32_mask(0, BIT(port % 32), RTL839X_RMK_PORT_DEI_TAG_CTRL(port));
	}

	/* Remap dot1p priorities to internal priority, for this the outer tag needs be re-marked */
	v = 0;
	for (int p = 0; p < 8; p++)
		v |= (dot1p_priority_remapping[p] & 0x7) << (p * 3);
	sw_w32(v, RTL839X_PRI_SEL_IPRI_REMAP);

	/* Configure Drop Precedence for Drop Eligible Indicator (DEI)
	 * Index 0: 0
	 * Index 1: 2
	 * Each indicator is 2 bits long
	 */
	sw_w32(2 << 2, RTL839X_PRI_SEL_DEI2DP_REMAP);

	/* Re-mark DEI: 4 bit-fields of 2 bits each, field 0 is bits 0-1, ... */
	sw_w32((0x1 << 2) | (0x1 << 4), RTL839X_RMK_DEI_CTRL);

	/* Set Congestion avoidance drop probability to 0 for drop precedences 0-2 (bits 24-31)
	 * low threshold (bits 0-11) to 4095 and high threshold (bits 12-23) to 4095
	 * Weighted Random Early Detection (WRED) is used
	 */
	sw_w32(4095 << 12 | 4095, RTL839X_WRED_PORT_THR_CTRL(0));
	sw_w32(4095 << 12 | 4095, RTL839X_WRED_PORT_THR_CTRL(1));
	sw_w32(4095 << 12 | 4095, RTL839X_WRED_PORT_THR_CTRL(2));

	/* Set queue-based congestion avoidance properties, register fields are as
	 * for forward RTL839X_WRED_PORT_THR_CTRL
	 */
	for (int q = 0; q < 8; q++) {
		sw_w32(255 << 24 | 78 << 12 | 68, RTL839X_WRED_QUEUE_THR_CTRL(q, 0));
		sw_w32(255 << 24 | 74 << 12 | 64, RTL839X_WRED_QUEUE_THR_CTRL(q, 0));
		sw_w32(255 << 24 | 70 << 12 | 60, RTL839X_WRED_QUEUE_THR_CTRL(q, 0));
	}
}

static bool rtldsa_trusts(const struct rtldsa_trust *trust, u8 sel)
{
	return memchr(trust->sel, sel, trust->nsel);
}

static u32 rtldsa_93xx_trust_weights(const struct rtldsa_trust *trust)
{
	u32 v = FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_PORT_MASK, 3);

	if (rtldsa_trusts(trust, IEEE_8021QAZ_APP_SEL_DSCP))
		v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_DSCP_MASK, 5);

	if (rtldsa_trusts(trust, DCB_APP_SEL_PCP))
		v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_ITAG_MASK, 6) |
		     FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_OTAG_MASK, 7);

	return v;
}

static void rtldsa_930x_qos_set_group_selector(int port, int group)
{
	sw_w32_mask(RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port),
		    group << RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port),
		    RTL930X_PORT_TBL_IDX_CTRL(port));
}

static void rtldsa_930x_qos_prio2queue_matrix(int *min_queues)
{
	u32 v = 0;

	for (int i = 0; i < MAX_PRIOS; i++)
		v |= i << (min_queues[i] * 3);

	sw_w32(v, RTL930X_QM_INTPRI2QID_CTRL);
}

void rtldsa_930x_queue_sched_set(int port, int queue, u32 weight, bool strict)
{
	u32 v = FIELD_PREP(RTL93XX_SCHED_Q_WEIGHT, weight) | (strict ? RTL930X_SCHED_Q_STRICT : 0);

	if (port < 24)
		sw_w32(v, RTL930X_SCHED_PORT_Q_CTRL_SET0(port, queue));
	else
		sw_w32(v, RTL930X_SCHED_PORT_Q_CTRL_SET1(port, queue));
}

static void rtldsa_930x_qos_set_scheduling_queue_weights(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	dsa_switch_for_each_user_port(dp, priv->ds)
		for (int q = 0; q < MAX_PRIOS; q++)
			rtldsa_930x_queue_sched_set(dp->index, q, rtldsa_default_queue_weights[q],
						    false);
}

void rtldsa_930x_qos_init(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	/* Assign all the ports to the Group-0 */
	dsa_switch_for_each_user_port(dp, priv->ds)
		rtldsa_930x_qos_set_group_selector(dp->index, 0);

	rtldsa_930x_qos_prio2queue_matrix(rtldsa_max_available_queue);

	/* configure priority weights */
	for (int group = 0; group < ARRAY_SIZE(rtldsa_93xx_trust); group++)
		sw_w32(rtldsa_93xx_trust_weights(&rtldsa_93xx_trust[group]),
		       RTL930X_PRI_SEL_TBL_CTRL(group));

	rtldsa_930x_qos_set_scheduling_queue_weights(priv);

	/* queue n of a frame from the CPU is queue n of the port, which has 8 or 12 queues */
	sw_w32(0x00fac688, RTL930X_QM_CPUQID2QID_CTRL);
	sw_w32(0x76543210, RTL930X_QM_CPUQID2XGQID_CTRL);
	sw_w32(0x0000ba98, RTL930X_QM_CPUQID2XGQID_CTRL + 4);
}

static void rtldsa_931x_qos_set_group_selector(int port, int group)
{
	sw_w32_mask(RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port),
		    group << RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port),
		    RTL931X_PORT_TBL_IDX_CTRL(port));
}

static void rtldsa_931x_qos_prio2queue_matrix(int *min_queues)
{
	u32 v = 0;

	for (int i = 0; i < MAX_PRIOS; i++)
		v |= i << (min_queues[i] * 3);

	sw_w32(v, RTL931X_QM_INTPRI2QID_CTRL);
}

void rtldsa_931x_queue_sched_set(int port, int queue, u32 weight, bool strict)
{
	u32 v = FIELD_PREP(RTL93XX_SCHED_Q_WEIGHT, weight) | (strict ? RTL931X_SCHED_Q_STRICT : 0);

	if (port < 52)
		sw_w32(v, RTL931X_SCHED_PORT_Q_CTRL_SET0(port, queue));
	else
		sw_w32(v, RTL931X_SCHED_PORT_Q_CTRL_SET1(port, queue));
}

static void rtldsa_931x_qos_set_scheduling_queue_weights(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	dsa_switch_for_each_user_port(dp, priv->ds)
		for (int q = 0; q < MAX_PRIOS; q++)
			rtldsa_931x_queue_sched_set(dp->index, q, rtldsa_default_queue_weights[q],
						    false);
}

void rtldsa_931x_qos_init(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	/* Assign all the ports to the Group-0 */
	dsa_switch_for_each_user_port(dp, priv->ds)
		rtldsa_931x_qos_set_group_selector(dp->index, 0);

	rtldsa_931x_qos_prio2queue_matrix(rtldsa_max_available_queue);

	/* configure priority weights */
	for (int group = 0; group < ARRAY_SIZE(rtldsa_93xx_trust); group++) {
		sw_w32(rtldsa_93xx_trust_weights(&rtldsa_93xx_trust[group]),
		       RTL931X_PRI_SEL_TBL_CTRL(group) + 4);
		sw_w32(0, RTL931X_PRI_SEL_TBL_CTRL(group));
	}

	rtldsa_931x_qos_set_scheduling_queue_weights(priv);

	/* queue n of a frame from the CPU is queue n of the port, which has 8 or 12 queues */
	sw_w32(0x00fac688, RTL931X_QM_CPUQID2QID_CTRL);
	sw_w32(0x76543210, RTL931X_QM_CPUQID2XGSQID_CTRL);
	sw_w32(0x0000ba98, RTL931X_QM_CPUQID2XGSQID_CTRL + 4);
}

static u32 rtldsa_qos_prio_get(int base, int index)
{
	return (sw_r32(base + (index / 10) * 4) >> ((index % 10) * 3)) & 0x7;
}

static void rtldsa_qos_prio_set(int base, int index, u8 prio)
{
	sw_w32_mask(0x7 << ((index % 10) * 3), prio << ((index % 10) * 3),
		    base + (index / 10) * 4);
}

/* DSA reads the DSCP map when it creates the user ports, which is before qos_init() */
void rtldsa_qos_setup(struct dsa_switch *ds)
{
	struct rtl838x_switch_priv *priv = ds->priv;
	struct dsa_port *dp;

	if (priv->r->storm_set)
		dsa_switch_for_each_user_port(dp, ds)
			for (int type = 0; type < RTLDSA_STORM_TYPES; type++)
				priv->r->storm_set(dp->index, type, 0, 0);

	if (!priv->r->pri_sel_remap_dscp)
		return;

	for (int dscp = 0; dscp < DSCP_MAP_MAX; dscp++)
		rtldsa_qos_prio_set(priv->r->pri_sel_remap_dscp, dscp, dscp >> 3);

	ds->dscp_prio_mapping_is_global = true;
}

int rtldsa_port_get_default_prio(struct dsa_switch *ds, int port)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_port_pri)
		return 0;

	return rtldsa_qos_prio_get(priv->r->pri_sel_port_pri, port);
}

int rtldsa_port_set_default_prio(struct dsa_switch *ds, int port, u8 prio)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_port_pri)
		return -EOPNOTSUPP;

	if (prio >= MAX_PRIOS)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	rtldsa_qos_prio_set(priv->r->pri_sel_port_pri, port, prio);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int rtldsa_port_get_dscp_prio(struct dsa_switch *ds, int port, u8 dscp)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_remap_dscp)
		return -EOPNOTSUPP;

	return rtldsa_qos_prio_get(priv->r->pri_sel_remap_dscp, dscp);
}

int rtldsa_port_add_dscp_prio(struct dsa_switch *ds, int port, u8 dscp, u8 prio)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_remap_dscp)
		return -EOPNOTSUPP;

	if (prio >= MAX_PRIOS)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	rtldsa_qos_prio_set(priv->r->pri_sel_remap_dscp, dscp, prio);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

/* The map is shared by all ports: only an entry that still holds @prio goes back to default */
int rtldsa_port_del_dscp_prio(struct dsa_switch *ds, int port, u8 dscp, u8 prio)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_remap_dscp)
		return -EOPNOTSUPP;

	mutex_lock(&priv->reg_mutex);
	if (rtldsa_qos_prio_get(priv->r->pri_sel_remap_dscp, dscp) == prio)
		rtldsa_qos_prio_set(priv->r->pri_sel_remap_dscp, dscp, dscp >> 3);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int rtldsa_port_get_apptrust(struct dsa_switch *ds, int port, u8 *sel, int *nsel)
{
	struct rtl838x_switch_priv *priv = ds->priv;
	const struct rtldsa_trust *trust;
	u32 v;

	if (!priv->r->pri_sel_port_tbl_idx)
		return -EOPNOTSUPP;

	v = sw_r32(priv->r->pri_sel_port_tbl_idx + (port / 16) * 4);
	trust = &rtldsa_93xx_trust[(v & RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port)) >>
				   RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port)];

	memcpy(sel, trust->sel, trust->nsel);
	*nsel = trust->nsel;

	return 0;
}

int rtldsa_port_set_apptrust(struct dsa_switch *ds, int port, const u8 *sel, int nsel)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	if (!priv->r->pri_sel_port_tbl_idx)
		return -EOPNOTSUPP;

	for (int group = 0; group < ARRAY_SIZE(rtldsa_93xx_trust); group++) {
		const struct rtldsa_trust *trust = &rtldsa_93xx_trust[group];

		if (nsel != trust->nsel || memcmp(sel, trust->sel, nsel))
			continue;

		mutex_lock(&priv->reg_mutex);
		sw_w32_mask(RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port),
			    group << RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port),
			    priv->r->pri_sel_port_tbl_idx + (port / 16) * 4);
		mutex_unlock(&priv->reg_mutex);

		return 0;
	}

	return -EOPNOTSUPP;
}

/* The thresholds are one table of queues for the whole switch and a port has RED on all
 * its queues or on none, so the ports with RED must all want the same table.
 */
static int rtldsa_red_apply(struct rtl838x_switch_priv *priv, int port)
{
	struct rtldsa_red_cfg *wanted = priv->ports[port].red_cfg;
	bool any = false;
	int ret = 0;

	for (int queue = 0; queue < MAX_PRIOS; queue++)
		any |= wanted[queue].max;

	if (!any)
		goto disable;

	if (priv->red_ports & ~BIT_ULL(port)) {
		if (memcmp(wanted, priv->red_cfg, sizeof(priv->red_cfg))) {
			dev_warn(priv->dev, "port %d: RED differs from the one in use\n", port);
			ret = -EBUSY;
			goto disable;
		}
	} else {
		for (int queue = 0; queue < RTLDSA_RED_QUEUES; queue++)
			priv->r->red_queue_set(queue, queue < MAX_PRIOS && wanted[queue].max ?
						      &wanted[queue] : NULL);

		memcpy(priv->red_cfg, wanted, sizeof(priv->red_cfg));
	}

	priv->r->red_port_set(port, true);
	priv->red_ports |= BIT_ULL(port);

	return 0;

disable:
	if (priv->red_ports & BIT_ULL(port)) {
		priv->r->red_port_set(port, false);
		priv->red_ports &= ~BIT_ULL(port);
	}

	return ret;
}

/* A negative @queue stands for all queues of the port, no @p for no RED */
static int rtldsa_red_set(struct rtl838x_switch_priv *priv, int port, int queue,
			  const struct tc_red_qopt_offload_params *p)
{
	struct rtldsa_red_cfg *wanted = priv->ports[port].red_cfg;
	struct rtldsa_red_cfg cfg = {};
	int ret, err = 0;

	if (p) {
		u32 min = DIV_ROUND_UP(p->min, RTLDSA_RED_PAGE_SIZE);
		u32 max = p->max / RTLDSA_RED_PAGE_SIZE;
		u64 rate = ((u64)p->probability * RTLDSA_RED_RATE_SCALE + U32_MAX) >> 32;

		if (!min || min >= max || max > priv->r->red_max_thr ||
		    !rate || rate > RTLDSA_RED_RATE_MAX) {
			err = -EINVAL;
		} else {
			cfg.min = min;
			cfg.max = max;
			cfg.rate = rate;
		}
	}

	for (int i = 0; i < MAX_PRIOS; i++)
		if (queue < 0 || queue == i)
			wanted[i] = cfg;

	ret = rtldsa_red_apply(priv, port);
	if (ret && cfg.max)
		rtldsa_red_set(priv, port, queue, NULL);

	return err ?: ret;
}

/* A qdisc that replaces another one is set up before the old one is destroyed, so every
 * offload remembers the handle of its qdisc and only that qdisc can take it away again.
 * The last entry of the handle arrays is the root, the others are the queues.
 */
static int rtldsa_qdisc_queue(const struct rtldsa_port *p, u32 parent)
{
	unsigned int band = TC_H_MIN(parent);

	if (parent == TC_H_ROOT)
		return MAX_PRIOS;

	if (TC_H_MAJ(parent) != p->ets_handle || !band || band > MAX_PRIOS)
		return -EOPNOTSUPP;

	return MAX_PRIOS - band;
}

static int rtldsa_setup_qdisc_red(struct rtl838x_switch_priv *priv, int port,
				  struct tc_red_qopt_offload *qopt)
{
	struct rtldsa_port *p = &priv->ports[port];
	int queue = rtldsa_qdisc_queue(p, qopt->parent);
	int ret = 0, hw_queue;
	u32 *owner;

	if (!priv->r->red_queue_set || queue < 0)
		return -EOPNOTSUPP;

	owner = &p->red_handle[queue];
	hw_queue = queue == MAX_PRIOS ? -1 : queue;

	mutex_lock(&priv->reg_mutex);

	switch (qopt->command) {
	case TC_RED_REPLACE:
		if (qopt->set.is_ecn) {
			rtldsa_red_set(priv, port, hw_queue, NULL);
			ret = -EOPNOTSUPP;
		} else {
			ret = rtldsa_red_set(priv, port, hw_queue, &qopt->set);
		}

		*owner = ret ? 0 : qopt->handle;
		break;
	case TC_RED_DESTROY:
		if (*owner != qopt->handle)
			break;

		rtldsa_red_set(priv, port, hw_queue, NULL);
		*owner = 0;
		break;
	case TC_RED_STATS:
	case TC_RED_XSTATS:
		if (!(priv->red_ports & BIT_ULL(port)) || *owner != qopt->handle)
			ret = -EOPNOTSUPP;
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	mutex_unlock(&priv->reg_mutex);

	return ret;
}

/* The children of the bands cannot be matched to a queue once their ETS is gone */
static void rtldsa_ets_release_queues(struct rtl838x_switch_priv *priv, int port)
{
	struct rtldsa_port *p = &priv->ports[port];

	for (int queue = 0; queue < MAX_PRIOS; queue++) {
		if (p->tbf_handle[queue])
			priv->r->egress_shaper_set(priv, port, queue, 0, 0);

		if (p->red_handle[queue] && !p->red_handle[MAX_PRIOS])
			rtldsa_red_set(priv, port, queue, NULL);

		p->tbf_handle[queue] = 0;
		p->red_handle[queue] = 0;
	}
}

/* Band 0 is the band ETS serves first and queue 7 the queue the hardware serves first.
 * The hardware maps priority n to queue n for the whole switch, so only the priomap
 * that says the same can be offloaded.
 */
static int rtldsa_setup_qdisc_ets(struct rtl838x_switch_priv *priv, int port,
				  struct tc_ets_qopt_offload *qopt)
{
	struct tc_ets_qopt_offload_replace_params *p = &qopt->replace_params;
	struct rtldsa_port *pp = &priv->ports[port];
	bool offload = false;
	u32 handle;
	int queue;

	if (!priv->r->queue_sched_set || qopt->parent != TC_H_ROOT)
		return -EOPNOTSUPP;

	switch (qopt->command) {
	case TC_ETS_REPLACE:
		offload = p->bands == MAX_PRIOS;
		for (int band = 0; offload && band < MAX_PRIOS; band++)
			offload = p->priomap[MAX_PRIOS - 1 - band] == band &&
				  (!p->quanta[band] ||
				   (p->weights[band] &&
				    p->weights[band] <= FIELD_MAX(RTL93XX_SCHED_Q_WEIGHT)));
		break;
	case TC_ETS_DESTROY:
		if (pp->ets_handle != qopt->handle)
			return 0;
		break;
	case TC_ETS_STATS:
		return pp->ets_handle == qopt->handle ? 0 : -EOPNOTSUPP;
	case TC_ETS_GRAFT:
		queue = MAX_PRIOS - 1 - qopt->graft_params.band;
		handle = qopt->graft_params.child_handle;
		if (pp->ets_handle != qopt->handle || queue < 0)
			return -EOPNOTSUPP;

		/* the default child, or one that took the queue when it was created */
		if (!handle || handle == pp->tbf_handle[queue] || handle == pp->red_handle[queue])
			return 0;

		return -EOPNOTSUPP;
	default:
		return -EOPNOTSUPP;
	}

	mutex_lock(&priv->reg_mutex);

	for (int band = 0; band < MAX_PRIOS; band++) {
		queue = MAX_PRIOS - 1 - band;

		if (!offload)
			priv->r->queue_sched_set(port, queue, rtldsa_default_queue_weights[queue],
						 false);
		else if (p->quanta[band])
			priv->r->queue_sched_set(port, queue, p->weights[band], false);
		else
			priv->r->queue_sched_set(port, queue, 1, true);
	}

	handle = offload ? qopt->handle : 0;
	if (pp->ets_handle != handle)
		rtldsa_ets_release_queues(priv, port);

	pp->ets_handle = handle;

	mutex_unlock(&priv->reg_mutex);

	return (offload || qopt->command == TC_ETS_DESTROY) ? 0 : -EOPNOTSUPP;
}

/* TBF at the root shapes the port, TBF on band n of an offloaded ETS shapes queue 8 - n */
static int rtldsa_setup_qdisc_tbf(struct rtl838x_switch_priv *priv, int port,
				  struct tc_tbf_qopt_offload *qopt)
{
	struct tc_tbf_qopt_offload_replace_params *params = &qopt->replace_params;
	struct rtldsa_port *p = &priv->ports[port];
	int queue = rtldsa_qdisc_queue(p, qopt->parent);
	int ret = 0, hw_queue;
	u32 *owner;

	if (!priv->r->egress_shaper_set || queue < 0)
		return -EOPNOTSUPP;

	owner = &p->tbf_handle[queue];
	hw_queue = queue == MAX_PRIOS ? -1 : queue;

	mutex_lock(&priv->reg_mutex);

	switch (qopt->command) {
	case TC_TBF_REPLACE:
		if (hw_queue < 0 && p->rate_police_egress)
			ret = -EOPNOTSUPP;
		else
			ret = priv->r->egress_shaper_set(priv, port, hw_queue,
							 params->rate.rate_bytes_ps,
							 params->max_size);

		if (ret && *owner)
			priv->r->egress_shaper_set(priv, port, hw_queue, 0, 0);

		*owner = ret ? 0 : qopt->handle;
		break;
	case TC_TBF_DESTROY:
		if (*owner != qopt->handle)
			break;

		priv->r->egress_shaper_set(priv, port, hw_queue, 0, 0);
		*owner = 0;
		break;
	case TC_TBF_STATS:
		ret = *owner == qopt->handle ? 0 : -EOPNOTSUPP;
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	mutex_unlock(&priv->reg_mutex);

	return ret;
}

int rtldsa_port_setup_tc(struct dsa_switch *ds, int port, enum tc_setup_type type,
			 void *type_data)
{
	struct rtl838x_switch_priv *priv = ds->priv;

	switch (type) {
	case TC_SETUP_QDISC_TBF:
		return rtldsa_setup_qdisc_tbf(priv, port, type_data);
	case TC_SETUP_QDISC_ETS:
		return rtldsa_setup_qdisc_ets(priv, port, type_data);
	case TC_SETUP_QDISC_RED:
		return rtldsa_setup_qdisc_red(priv, port, type_data);
	default:
		return -EOPNOTSUPP;
	}
}
