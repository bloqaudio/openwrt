// SPDX-License-Identifier: GPL-2.0-only

#include <asm/mach-rtl838x/mach-rtl83xx.h>
#include <linux/delay.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>
#include <linux/iopoll.h>
#include <linux/seq_file.h>
#include <linux/sort.h>

#include "rtl83xx.h"

#define RTL931X_VLAN_PORT_TAG_STS_INTERNAL			0x0
#define RTL931X_VLAN_PORT_TAG_STS_UNTAG				0x1
#define RTL931X_VLAN_PORT_TAG_STS_TAGGED			0x2
#define RTL931X_VLAN_PORT_TAG_STS_PRIORITY_TAGGED		0x3

#define RTL931X_VLAN_PORT_TAG_CTRL_BASE				0x4860
/* port 0-56 */
#define RTL931X_VLAN_PORT_TAG_CTRL(port) \
	(RTL931X_VLAN_PORT_TAG_CTRL_BASE + (port << 2))
#define RTL931X_VLAN_PORT_TAG_EGR_OTAG_STS_MASK			GENMASK(13, 12)
#define RTL931X_VLAN_PORT_TAG_EGR_ITAG_STS_MASK			GENMASK(11, 10)
#define RTL931X_VLAN_PORT_TAG_EGR_OTAG_KEEP_MASK		GENMASK(9, 9)
#define RTL931X_VLAN_PORT_TAG_EGR_ITAG_KEEP_MASK		GENMASK(8, 8)
#define RTL931X_VLAN_PORT_TAG_IGR_OTAG_KEEP_MASK		GENMASK(7, 7)
#define RTL931X_VLAN_PORT_TAG_IGR_ITAG_KEEP_MASK		GENMASK(6, 6)
#define RTL931X_VLAN_PORT_TAG_OTPID_IDX_MASK			GENMASK(5, 4)
#define RTL931X_VLAN_PORT_TAG_OTPID_KEEP_MASK			GENMASK(3, 3)
#define RTL931X_VLAN_PORT_TAG_ITPID_IDX_MASK			GENMASK(2, 1)
#define RTL931X_VLAN_PORT_TAG_ITPID_KEEP_MASK			GENMASK(0, 0)

#define RTL931X_VLAN_TPID_8021Q					0x8100
#define RTL931X_VLAN_TPID_8021AD				0x88a8
#define RTL931X_VLAN_TPID_PAIR(otpid, itpid) \
	(((u32)(otpid) << 16) | (itpid))

/* Definition of the RTL931X-specific template field IDs as used in the PIE */
enum template_field_id {
	TEMPLATE_FIELD_SPM0 = 1,
	TEMPLATE_FIELD_SPM1 = 2,
	TEMPLATE_FIELD_SPM2 = 3,
	TEMPLATE_FIELD_SPM3 = 4,
	TEMPLATE_FIELD_DMAC0 = 9,
	TEMPLATE_FIELD_DMAC1 = 10,
	TEMPLATE_FIELD_DMAC2 = 11,
	TEMPLATE_FIELD_SMAC0 = 12,
	TEMPLATE_FIELD_SMAC1 = 13,
	TEMPLATE_FIELD_SMAC2 = 14,
	TEMPLATE_FIELD_ETHERTYPE = 15,
	TEMPLATE_FIELD_OTAG = 16,
	TEMPLATE_FIELD_ITAG = 17,
	TEMPLATE_FIELD_SIP0 = 18,
	TEMPLATE_FIELD_SIP1 = 19,
	TEMPLATE_FIELD_DIP0 = 20,
	TEMPLATE_FIELD_DIP1 = 21,
	TEMPLATE_FIELD_IP_TOS_PROTO = 22,
	TEMPLATE_FIELD_L4_SPORT = 23,
	TEMPLATE_FIELD_L4_DPORT = 24,
	TEMPLATE_FIELD_L34_HEADER = 25,
	TEMPLATE_FIELD_TCP_INFO = 26,
	TEMPLATE_FIELD_SIP2 = 34,
	TEMPLATE_FIELD_SIP3 = 35,
	TEMPLATE_FIELD_SIP4 = 36,
	TEMPLATE_FIELD_SIP5 = 37,
	TEMPLATE_FIELD_SIP6 = 38,
	TEMPLATE_FIELD_SIP7 = 39,
	TEMPLATE_FIELD_DIP2 = 42,
	TEMPLATE_FIELD_DIP3 = 43,
	TEMPLATE_FIELD_DIP4 = 44,
	TEMPLATE_FIELD_DIP5 = 45,
	TEMPLATE_FIELD_DIP6 = 46,
	TEMPLATE_FIELD_DIP7 = 47,
	TEMPLATE_FIELD_FLOW_LABEL = 49,
	TEMPLATE_FIELD_DSAP_SSAP = 50,
	TEMPLATE_FIELD_FWD_VID = 52,
	TEMPLATE_FIELD_RANGE_CHK = 53,
	TEMPLATE_FIELD_SLP = 55,
	TEMPLATE_FIELD_DLP = 56,
	TEMPLATE_FIELD_META_DATA = 57,
	TEMPLATE_FIELD_FIRST_MPLS1 = 60,
	TEMPLATE_FIELD_FIRST_MPLS2 = 61,
	TEMPLATE_FIELD_DPM3 = 8,
};

/* The meaning of TEMPLATE_FIELD_VLAN depends on phase and the configuration in
 * RTL931X_PIE_CTRL. We use always the same definition and map to the inner VLAN tag:
 */
#define TEMPLATE_FIELD_VLAN TEMPLATE_FIELD_ITAG

/* Number of fixed templates predefined in the RTL9300 SoC */
#define N_FIXED_TEMPLATES 5
/* RTL931x specific predefined templates */
static enum template_field_id fixed_templates[N_FIXED_TEMPLATES][N_FIXED_FIELDS_RTL931X] = {
	{
		TEMPLATE_FIELD_DMAC0, TEMPLATE_FIELD_DMAC1, TEMPLATE_FIELD_DMAC2,
		TEMPLATE_FIELD_SMAC0, TEMPLATE_FIELD_SMAC1, TEMPLATE_FIELD_SMAC2,
		TEMPLATE_FIELD_VLAN, TEMPLATE_FIELD_IP_TOS_PROTO, TEMPLATE_FIELD_DSAP_SSAP,
		TEMPLATE_FIELD_ETHERTYPE, TEMPLATE_FIELD_SPM0, TEMPLATE_FIELD_SPM1,
		TEMPLATE_FIELD_SPM2, TEMPLATE_FIELD_SPM3
	}, {
		TEMPLATE_FIELD_SIP0, TEMPLATE_FIELD_SIP1, TEMPLATE_FIELD_DIP0,
		TEMPLATE_FIELD_DIP1, TEMPLATE_FIELD_IP_TOS_PROTO, TEMPLATE_FIELD_TCP_INFO,
		TEMPLATE_FIELD_L4_SPORT, TEMPLATE_FIELD_L4_DPORT, TEMPLATE_FIELD_VLAN,
		TEMPLATE_FIELD_RANGE_CHK, TEMPLATE_FIELD_SPM0, TEMPLATE_FIELD_SPM1,
		TEMPLATE_FIELD_SPM2, TEMPLATE_FIELD_SPM3
	}, {
		TEMPLATE_FIELD_DMAC0, TEMPLATE_FIELD_DMAC1, TEMPLATE_FIELD_DMAC2,
		TEMPLATE_FIELD_VLAN, TEMPLATE_FIELD_ETHERTYPE, TEMPLATE_FIELD_IP_TOS_PROTO,
		TEMPLATE_FIELD_SIP0, TEMPLATE_FIELD_SIP1, TEMPLATE_FIELD_DIP0,
		TEMPLATE_FIELD_DIP1, TEMPLATE_FIELD_L4_SPORT, TEMPLATE_FIELD_L4_DPORT,
		TEMPLATE_FIELD_META_DATA, TEMPLATE_FIELD_SLP
	}, {
		TEMPLATE_FIELD_DIP0, TEMPLATE_FIELD_DIP1, TEMPLATE_FIELD_DIP2,
		TEMPLATE_FIELD_DIP3, TEMPLATE_FIELD_DIP4, TEMPLATE_FIELD_DIP5,
		TEMPLATE_FIELD_DIP6, TEMPLATE_FIELD_DIP7, TEMPLATE_FIELD_IP_TOS_PROTO,
		TEMPLATE_FIELD_TCP_INFO, TEMPLATE_FIELD_L4_SPORT, TEMPLATE_FIELD_L4_DPORT,
		TEMPLATE_FIELD_RANGE_CHK, TEMPLATE_FIELD_SLP
	}, {
		TEMPLATE_FIELD_SIP0, TEMPLATE_FIELD_SIP1, TEMPLATE_FIELD_SIP2,
		TEMPLATE_FIELD_SIP3, TEMPLATE_FIELD_SIP4, TEMPLATE_FIELD_SIP5,
		TEMPLATE_FIELD_SIP6, TEMPLATE_FIELD_SIP7, TEMPLATE_FIELD_META_DATA,
		TEMPLATE_FIELD_VLAN, TEMPLATE_FIELD_SPM0, TEMPLATE_FIELD_SPM1,
		TEMPLATE_FIELD_SPM2, TEMPLATE_FIELD_SPM3
	},
};

inline void rtl931x_exec_tbl0_cmd(u32 cmd)
{
	sw_w32(cmd, RTL931X_TBL_ACCESS_CTRL_0);
	do { } while (sw_r32(RTL931X_TBL_ACCESS_CTRL_0) & (1 << 20));
}

inline void rtl931x_exec_tbl1_cmd(u32 cmd)
{
	sw_w32(cmd, RTL931X_TBL_ACCESS_CTRL_1);
	do { } while (sw_r32(RTL931X_TBL_ACCESS_CTRL_1) & (1 << 17));
}

inline int rtl931x_tbl_access_data_0(int i)
{
	return RTL931X_TBL_ACCESS_DATA_0(i);
}

static void rtl931x_vlan_profile_dump(int index)
{
	u64 profile[4];

	if (index < 0 || index > 15)
		return;

	/* Each flood portmask is split upper-half-first: 25 bits for ports
	 * 56..32 in the lower-offset word, then 32 bits for ports 31..0.
	 */
	profile[0] = sw_r32(RTL931X_VLAN_PROFILE_SET(index));
	profile[1] = (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 4) & 0x1FFFFFFULL) << 32 |
		     (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 8) & 0xFFFFFFFF);
	profile[2] = (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 12) & 0x1FFFFFFULL) << 32 |
		     (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 16) & 0xFFFFFFFF);
	profile[3] = (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 20) & 0x1FFFFFFULL) << 32 |
		     (sw_r32(RTL931X_VLAN_PROFILE_SET(index) + 24) & 0xFFFFFFFF);

	pr_debug("VLAN %d: L2 learning: %d, L2 Unknown MultiCast Field %llx, IPv4 Unknown MultiCast Field %llx, IPv6 Unknown MultiCast Field: %llx\n",
		 index, (u32)(profile[0] & (3 << 14)), profile[1], profile[2], profile[3]);
}

static void rtl931x_stp_get(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[])
{
	u32 cmd = 1 << 20 | /* Execute cmd */
		  0 << 19 | /* Read */
		  5 << 15 | /* Table type 0b101 */
		  (msti & 0x3fff);
	priv->r->exec_tbl0_cmd(cmd);

	for (int i = 0; i < 4; i++)
		port_state[i] = sw_r32(priv->r->tbl_access_data_0(i));
}

static void rtl931x_stp_set(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[])
{
	u32 cmd = 1 << 20 | /* Execute cmd */
		  1 << 19 | /* Write */
		  5 << 15 | /* Table type 0b101 */
		  (msti & 0x3fff);
	for (int i = 0; i < 4; i++)
		sw_w32(port_state[i], priv->r->tbl_access_data_0(i));
	priv->r->exec_tbl0_cmd(cmd);
}

static inline int rtldsa_931x_trk_mbr_ctr(int group)
{
	/* DSA LAG ids are 1-based; hardware trunks start at 0. Map them so
	 * the first bond uses hardware trunk 0, the path every vendor
	 * implementation exercises. TRK_MBR_CTRL is indexed by the local
	 * trunk slot; we use the identity mapping slot == trunk id.
	 */
	return RTL931X_TRK_MBR_CTRL + ((group - 1) << 3);
}

static void rtl931x_vlan_tables_read(u32 vlan, struct rtl838x_vlan_info *info)
{
	u32 v, w, x, y;
	/* Read VLAN table (3) via register 0 */
	struct table_reg *r = rtl_table_get(RTL9310_TBL_0, 3);

	rtl_table_read(r, vlan);
	v = sw_r32(rtl_table_data(r, 0));
	w = sw_r32(rtl_table_data(r, 1));
	x = sw_r32(rtl_table_data(r, 2));
	y = sw_r32(rtl_table_data(r, 3));
	rtl_table_release(r);

	pr_debug("VLAN_READ %d: %08x %08x %08x %08x\n", vlan, v, w, x, y);
	info->member_ports = ((u64)v) << 25 | (w >> 7);
	info->profile_id = (x >> 16) & 0xf;
	info->fid = w & 0x7f;				/* AKA MSTI depending on context */
	info->hash_uc_fid = !!(x & BIT(31));
	info->hash_mc_fid = !!(x & BIT(30));
	info->if_id = (x >> 20) & 0x3ff;
	info->multicast_grp_mask = x & 0xffff;
	if (y & BIT(31))
		info->l2_tunnel_list_id = y >> 18;
	else
		info->l2_tunnel_list_id = -1;
	pr_debug("%s read member %016llx, profile-id %d, uc %d, mc %d, intf-id %d\n", __func__,
		 info->member_ports, info->profile_id, info->hash_uc_fid, info->hash_mc_fid,
		 info->if_id);

	/* Read UNTAG table via table register 3 */
	r = rtl_table_get(RTL9310_TBL_3, 0);
	rtl_table_read(r, vlan);
	info->untagged_ports = ((u64)sw_r32(rtl_table_data(r, 0))) << 25;
	info->untagged_ports |= sw_r32(rtl_table_data(r, 1)) >> 7;

	rtl_table_release(r);
}

static void rtl931x_vlan_set_tagged(u32 vlan, struct rtl838x_vlan_info *info)
{
	struct table_reg *r;
	u32 v, w, x, y;

	v = info->member_ports >> 25;
	w = (info->member_ports & GENMASK(24, 0)) << 7;
	w |= info->fid & 0x7f;
	x = info->hash_uc_fid ? BIT(31) : 0;
	x |= info->hash_mc_fid ? BIT(30) : 0;
	x |= (info->if_id & 0x3ff) << 20;
	x |= (info->profile_id & 0xf) << 16;
	x |= info->multicast_grp_mask & 0xffff;
	if (info->l2_tunnel_list_id >= 0) {
		y = info->l2_tunnel_list_id << 18;
		y |= BIT(31);
	} else {
		y = 0;
	}

	r = rtl_table_get(RTL9310_TBL_0, 3);
	sw_w32(v, rtl_table_data(r, 0));
	sw_w32(w, rtl_table_data(r, 1));
	sw_w32(x, rtl_table_data(r, 2));
	sw_w32(y, rtl_table_data(r, 3));

	rtl_table_write(r, vlan);
	rtl_table_release(r);
}

static void rtl931x_vlan_set_untagged(u32 vlan, u64 portmask)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_3, 0);

	rtl839x_set_port_reg_be(portmask << 7, rtl_table_data(r, 0));
	rtl_table_write(r, vlan);
	rtl_table_release(r);
}

static inline int rtl931x_mac_force_mode_ctrl(int p)
{
	return RTL931X_MAC_FORCE_MODE_CTRL + (p << 2);
}

static inline int rtl931x_mac_port_ctrl(int p)
{
	return RTL931X_MAC_L2_PORT_CTRL + (p << 7);
}

static inline int rtl931x_l2_port_new_salrn(int p)
{
	return RTL931X_L2_PORT_NEW_SALRN(p);
}

static inline int rtl931x_l2_port_new_sa_fwd(int p)
{
	return RTL931X_L2_PORT_NEW_SA_FWD(p);
}

static int rtldsa_931x_get_mirror_config(struct rtldsa_mirror_config *config,
					 int group, int port)
{
	config->ctrl = RTL931X_MIR_CTRL + group * 4;
	config->spm = RTL931X_MIR_SPM_CTRL + group * 8;
	config->dpm = RTL931X_MIR_DPM_CTRL + group * 8;

	/* Enable mirroring to destination port */
	config->val = BIT(0);
	config->val |= port << 9;

	/* mirror mode: let mirrored packets follow TX settings of
	 * mirroring port
	 */
	config->val |= BIT(5);

	/* direction of traffic to be mirrored when a packet
	 * hits both SPM and DPM ports: prefer egress
	 */
	config->val |= BIT(4);

	return 0;
}

static int rtldsa_931x_port_rate_police_add(struct dsa_switch *ds, int port,
					    const struct flow_action_entry *act,
					    bool ingress)
{
	struct rtl838x_switch_priv *priv = ds->priv;
	u32 burst;
	u64 rate;
	u32 addr;

	/* rate has unit 16000 bit */
	rate = div_u64(act->police.rate_bytes_ps, 2000);
	if (rate > RTL93XX_BANDWIDTH_CTRL_RATE_MAX)
		dev_warn(priv->dev,
			 "port %d %s policer rate %llu Bps clamped to %u Bps\n",
			 port, ingress ? "ingress" : "egress",
			 act->police.rate_bytes_ps,
			 (u32)(RTL93XX_BANDWIDTH_CTRL_RATE_MAX * 2000U));
	rate = min_t(u64, rate, RTL93XX_BANDWIDTH_CTRL_RATE_MAX);
	rate |= RTL93XX_BANDWIDTH_CTRL_ENABLE;

	if (act->police.burst > RTL931X_BANDWIDTH_CTRL_MAX_BURST)
		dev_warn(priv->dev,
			 "port %d %s policer burst %u bytes clamped to %u bytes\n",
			 port, ingress ? "ingress" : "egress", act->police.burst,
			 (u32)RTL931X_BANDWIDTH_CTRL_MAX_BURST);
	burst = min_t(u32, act->police.burst, RTL931X_BANDWIDTH_CTRL_MAX_BURST);

	if (ingress)
		addr = RTL931X_BANDWIDTH_CTRL_INGRESS(port);
	else
		addr = RTL931X_EGBW_PORT_CTRL(port);

	sw_w32(burst, addr + 4);
	sw_w32(rate, addr);

	return 0;
}

static int rtldsa_931x_port_rate_police_del(struct dsa_switch *ds, int port,
					    struct flow_cls_offload *cls,
					    bool ingress)
{
	u32 addr;

	if (ingress)
		addr = RTL931X_BANDWIDTH_CTRL_INGRESS(port);
	else
		addr = RTL931X_EGBW_PORT_CTRL(port);

	if (ingress) {
		sw_w32_mask(RTL93XX_BANDWIDTH_CTRL_ENABLE, 0, addr);
	} else {
		/* Match root-TBF teardown: a stale burst cap still gates egress
		 * with EN clear, so restore rate-wide-open and the reset burst.
		 */
		sw_w32(RTL931X_EGBW_Q_RATE_M, addr);
		sw_w32(RTL931X_EGBW_LB_RESET_BURST, addr + 4);
	}

	return 0;
}

irqreturn_t rtl931x_switch_irq(int irq, void *dev_id)
{
	struct dsa_switch *ds = dev_id;
	u32 status = sw_r32(RTL931X_ISR_GLB_SRC);
	u64 ports = rtl839x_get_port_reg_le(RTL931X_ISR_PORT_LINK_STS_CHG);
	u64 link;

	/* Clear status */
	rtl839x_set_port_reg_le(ports, RTL931X_ISR_PORT_LINK_STS_CHG);
	pr_debug("RTL931X Link change: status: %x, ports %016llx\n", status, ports);

	link = rtl839x_get_port_reg_le(RTL931X_MAC_LINK_STS);
	/* Must re-read this to get correct status */
	link = rtl839x_get_port_reg_le(RTL931X_MAC_LINK_STS);
	pr_debug("RTL931X Link change: status: %x, link status %016llx\n", status, link);

	for (int i = 0; i < 56; i++) {
		if (ports & BIT_ULL(i)) {
			if (link & BIT_ULL(i)) {
				pr_debug("%s port %d up\n", __func__, i);
				dsa_port_phylink_mac_change(ds, i, true);
			} else {
				pr_debug("%s port %d down\n", __func__, i);
				dsa_port_phylink_mac_change(ds, i, false);
			}
		}
	}

	return IRQ_HANDLED;
}

void rtl931x_print_matrix(void)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 1);

	for (int i = 0; i < 64; i++) {
		rtl_table_read(r, i);
		pr_info("> %08x %08x\n", sw_r32(rtl_table_data(r, 0)),
			sw_r32(rtl_table_data(r, 1)));
	}
	rtl_table_release(r);
}

static void rtldsa_931x_set_receive_management_action(int port, rma_ctrl_t type,
						      action_type_t action)
{
	u32 shift;
	u32 value;
	u32 reg;

	/* hack for value mapping */
	if (type == GRATARP && action == COPY2CPU)
		action = TRAP2MASTERCPU;

	/* PTP doesn't allow to flood to all ports */
	if (action == FLOODALL &&
	    (type == PTP || type == PTP_UDP || type == PTP_ETH2)) {
		pr_warn("%s: Port flooding not supported for PTP\n", __func__);
		return;
	}

	switch (action) {
	case FORWARD:
		value = 0;
		break;
	case DROP:
		value = 1;
		break;
	case TRAP2CPU:
		value = 2;
		break;
	case TRAP2MASTERCPU:
		value = 3;
		break;
	case FLOODALL:
		value = 4;
		break;
	default:
		return;
	}

	switch (type) {
	case BPDU:
		reg = RTL931X_RMA_BPDU_CTRL + (port / 10) * 4;
		shift = (port % 10) * 3;
		sw_w32_mask(GENMASK(shift + 2, shift), value << shift, reg);
		break;
	case PTP:
		reg = RTL931X_RMA_PTP_CTRL + port * 4;

		/* udp */
		sw_w32_mask(GENMASK(3, 2), value << 2, reg);

		/* eth2 */
		sw_w32_mask(GENMASK(1, 0), value, reg);
		break;
	case PTP_UDP:
		reg = RTL931X_RMA_PTP_CTRL + port * 4;
		sw_w32_mask(GENMASK(3, 2), value << 2, reg);
		break;
	case PTP_ETH2:
		reg = RTL931X_RMA_PTP_CTRL + port * 4;
		sw_w32_mask(GENMASK(1, 0), value, reg);
		break;
	case LLDP:
		reg = RTL931X_RMA_LLDP_CTRL + (port / 10) * 4;
		shift = (port % 10) * 3;
		sw_w32_mask(GENMASK(shift + 2, shift), value << shift, reg);
		break;
	case EAPOL:
		reg = RTL931X_RMA_EAPOL_CTRL + (port / 10) * 4;
		shift = (port % 10) * 3;
		sw_w32_mask(GENMASK(shift + 2, shift), value << shift, reg);
		break;
	case GRATARP:
		reg = RTL931X_TRAP_ARP_GRAT_PORT_ACT + (port / 16) * 4;
		shift = (port % 16) * 2;
		sw_w32_mask(GENMASK(shift + 1, shift), value << shift, reg);
		break;
	}
}

/* Enable traffic between a source port and a destination port matrix */
static void rtl931x_traffic_set(int source, u64 dest_matrix)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 1);

	sw_w32(dest_matrix >> (32 - 7), rtl_table_data(r, 0));
	sw_w32(dest_matrix << 7, rtl_table_data(r, 1));
	rtl_table_write(r, source);
	rtl_table_release(r);
}

static void rtl931x_traffic_enable(int source, int dest)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 1);

	rtl_table_read(r, source);
	sw_w32_mask(0, BIT((dest + 7) % 32), rtl_table_data(r, (dest + 7) / 32 ? 0 : 1));
	rtl_table_write(r, source);
	rtl_table_release(r);
}

static void rtl931x_traffic_disable(int source, int dest)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 1);

	rtl_table_read(r, source);
	sw_w32_mask(BIT((dest + 7) % 32), 0, rtl_table_data(r, (dest + 7) / 32 ? 0 : 1));
	rtl_table_write(r, source);
	rtl_table_release(r);
}

static u64 rtldsa_931x_l2_hash_seed(u64 mac, u32 vid)
{
	return (u64)vid << 48 | mac;
}

/* Calculate both the block 0 and the block 1 hash by applyingthe same hash
 * algorithm as the one used currently by the ASIC to the seed, and return
 * both hashes in the lower and higher word of the return value since only 12 bit of
 * the hash are significant.
 */
static u32 rtl931x_l2_hash_key(struct rtl838x_switch_priv *priv, u64 seed)
{
	u32 h, h0, h1, h2, h3, h4, k0, k1;

	h0 = seed & 0xfff;
	h1 = (seed >> 12) & 0xfff;
	h2 = (seed >> 24) & 0xfff;
	h3 = (seed >> 36) & 0xfff;
	h4 = (seed >> 48) & 0xfff;
	h4 = ((h4 & 0x7) << 9) | ((h4 >> 3) & 0x1ff);
	k0 = h0 ^ h1 ^ h2 ^ h3 ^ h4;

	h0 = seed & 0xfff;
	h0 = ((h0 & 0x1ff) << 3) | ((h0 >> 9) & 0x7);
	h1 = (seed >> 12) & 0xfff;
	h1 = ((h1 & 0x3f) << 6) | ((h1 >> 6) & 0x3f);
	h2 = (seed >> 24) & 0xfff;
	h3 = (seed >> 36) & 0xfff;
	h3 = ((h3 & 0x3f) << 6) | ((h3 >> 6) & 0x3f);
	h4 = (seed >> 48) & 0xfff;
	k1 = h0 ^ h1 ^ h2 ^ h3 ^ h4;

	/* Algorithm choice for block 0 */
	if (sw_r32(RTL931X_L2_CTRL) & BIT(0))
		h = k1;
	else
		h = k0;

	/* Algorithm choice for block 1
	 * Since k0 and k1 are < 4096, adding 4096 will offset the hash into the second
	 * half of hash-space
	 * 4096 is in fact the hash-table size 32768 divided by 4 hashes per bucket
	 * divided by 2 to divide the hash space in 2
	 */
	if (sw_r32(RTL931X_L2_CTRL) & BIT(1))
		h |= (k1 + 4096) << 16;
	else
		h |= (k0 + 4096) << 16;

	return h;
}

/* Fills an L2 entry structure from the SoC registers */
static void rtl931x_fill_l2_entry(u32 r[], struct rtl838x_l2_entry *e)
{
	pr_debug("In %s valid?\n", __func__);
	e->valid = !!(r[0] & BIT(31));
	if (!e->valid)
		return;

	pr_debug("%s: entry valid, raw: %08x %08x %08x %08x\n", __func__, r[0], r[1], r[2], r[3]);
	e->is_ip_mc = false;
	e->is_ipv6_mc = false;

	e->mac[0] = r[0] >> 8;
	e->mac[1] = r[0];
	e->mac[2] = r[1] >> 24;
	e->mac[3] = r[1] >> 16;
	e->mac[4] = r[1] >> 8;
	e->mac[5] = r[1];

	e->is_open_flow = !!(r[0] & BIT(30));
	e->is_pe_forward = !!(r[0] & BIT(29));
	e->next_hop = !!(r[2] & BIT(30));
	e->rvid = (r[0] >> 16) & 0xfff;

	/* Is it a unicast entry? check multicast bit */
	if (!(e->mac[0] & 1)) {
		e->type = L2_UNICAST;
		e->is_l2_tunnel = !!(r[2] & BIT(31));
		e->is_static = !!(r[2] & BIT(13));
		e->port = (r[2] >> 19) & 0x3ff;
		/* Check for trunk port */
		if (r[2] & BIT(29)) {
			e->is_trunk = true;
			e->trunk = e->port & 0xff;
		} else {
			e->is_trunk = false;
			e->stack_dev = (e->port >> 6) & 0xf;
			e->port = e->port & 0x3f;
		}

		e->block_da = !!(r[2] & BIT(14));
		e->block_sa = !!(r[2] & BIT(15));
		e->suspended = !!(r[2] & BIT(12));
		e->age = (r[2] >> 16) & 7;

		/* HW doesn't use VID but FID for as key */
		e->vid = (r[0] >> 16) & 0xfff;

		if (e->is_l2_tunnel)
			e->l2_tunnel_id = ((r[2] & 0xff) << 4) | (r[3] >> 28);
		/* TODO: Implement VLAN conversion */
	} else {
		e->type = L2_MULTICAST;
		e->is_local_forward = !!(r[2] & BIT(31));
		e->is_remote_forward = !!(r[2] & BIT(17));
		e->mc_portmask_index = (r[2] >> 18) & 0xfff;
		e->l2_tunnel_list_id = (r[2] >> 4) & 0x1fff;
		e->vid = e->rvid;
	}
}

/* Fills the 3 SoC table registers r[] with the information of in the rtl838x_l2_entry */
static void rtl931x_fill_l2_row(u32 r[], struct rtl838x_l2_entry *e)
{
	u32 port;

	if (!e->valid) {
		r[0] = r[1] = r[2] = r[3] = 0;
		return;
	}

	r[3] = 0;

	r[0] = BIT(31); /* Set valid bit */

	r[0] |= ((u32)e->mac[0]) << 8 |
	       ((u32)e->mac[1]);
	r[1] = ((u32)e->mac[2]) << 24 |
	       ((u32)e->mac[3]) << 16 |
		   ((u32)e->mac[4]) << 8 |
		   ((u32)e->mac[5]);

	r[0] |= e->is_open_flow ? BIT(30) : 0;
	r[0] |= e->is_pe_forward ? BIT(29) : 0;
	r[0] |= e->hash_msb ? BIT(28) : 0;
	r[2] = e->next_hop ? BIT(30) : 0;
	r[0] |= (e->rvid & 0xfff) << 16;

	if (e->type == L2_UNICAST) {
		r[2] |= e->is_l2_tunnel ? BIT(31) : 0;
		r[2] |= e->is_static ? BIT(13) : 0;

		if (e->is_trunk) {
			r[2] |= BIT(29);
			port = e->trunk & 0xff;
		} else {
			port = e->port & 0x3f;
			port |= (e->stack_dev & 0xf) << 6;
		}

		r[2] |= (port & 0x3ff) << 19;
		r[2] |= e->block_da ? BIT(14) : 0;
		r[2] |= e->block_sa ? BIT(15) : 0;
		r[2] |= e->suspended ? BIT(12) : 0;
		r[2] |= (e->age & 0x7) << 16;
		if (e->is_l2_tunnel) {
			r[2] |= (e->l2_tunnel_id >> 4) & 0xff;
			r[3] |= (e->l2_tunnel_id & 0xf) << 28;
		}
	} else { /* L2_MULTICAST */
		/* Local forwarding must be enabled for the portmask to apply;
		 * remote forwarding is only meaningful in stacked setups.
		 */
		r[2] |= BIT(31);
		r[2] |= (e->mc_portmask_index & 0xfff) << 18;
	}
}

/* Read an L2 UC or MC entry out of a hash bucket of the L2 forwarding table
 * hash is the id of the bucket and pos is the position of the entry in that bucket
 * The data read from the SoC is filled into rtl838x_l2_entry
 */
static u64 rtl931x_read_l2_entry_using_hash(u32 hash, u32 pos, struct rtl838x_l2_entry *e)
{
	u32 r[4];
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 0);
	u32 idx;
	u64 mac;
	u64 seed;

	pr_debug("%s: hash %08x, pos: %d\n", __func__, hash, pos);

	/* On the RTL93xx, 2 different hash algorithms are used making it a total of
	 * 8 buckets that need to be searched, 4 for each hash-half
	 * Use second hash space when bucket is between 4 and 8
	 */
	if (pos >= 4) {
		pos -= 4;
		hash >>= 16;
	} else {
		hash &= 0xffff;
	}

	idx = (0 << 14) | (hash << 2) | pos; /* Search SRAM, with hash and at pos in bucket */
	pr_debug("%s: NOW hash %08x, pos: %d\n", __func__, hash, pos);

	rtl_table_read(q, idx);
	for (int i = 0; i < 4; i++)
		r[i] = sw_r32(rtl_table_data(q, i));

	rtl_table_release(q);

	rtl931x_fill_l2_entry(r, e);

	pr_debug("%s: valid: %d, nh: %d\n", __func__, e->valid, e->next_hop);
	if (!e->valid)
		return 0;

	mac = ((u64)e->mac[0]) << 40 |
	      ((u64)e->mac[1]) << 32 |
	      ((u64)e->mac[2]) << 24 |
	      ((u64)e->mac[3]) << 16 |
	      ((u64)e->mac[4]) << 8 |
	      ((u64)e->mac[5]);

	seed = rtldsa_931x_l2_hash_seed(mac, e->rvid);
	pr_debug("%s: mac %016llx, seed %016llx\n", __func__, mac, seed);

	/* return vid with concatenated mac as unique id */
	return seed;
}

static u64 rtl931x_read_cam(int idx, struct rtl838x_l2_entry *e)
{
	u32 r[4];
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 1);

	rtl_table_read(q, idx);
	for (int i = 0; i < 4; i++)
		r[i] = sw_r32(rtl_table_data(q, i));

	rtl_table_release(q);
	rtl931x_fill_l2_entry(r, e);
	if (!e->valid)
		return 0;

	/* return mac with concatenated fid as unique id */
	return ((((u64)(r[0] & 0xffff) << 32) | (u64)r[1]) << 12) | e->vid;
}

static void rtl931x_write_cam(int idx, struct rtl838x_l2_entry *e)
{
	u32 r[4];
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 1);

	rtl931x_fill_l2_row(r, e);

	for (int i = 0; i < 4; i++)
		sw_w32(r[i], rtl_table_data(q, i));
	rtl_table_write(q, idx);
	rtl_table_release(q);
}

static void rtl931x_write_l2_entry_using_hash(u32 hash, u32 pos, struct rtl838x_l2_entry *e)
{
	u32 r[4];
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 0);
	u32 idx = (0 << 14) | (hash << 2) | pos; /* Access SRAM, with hash and at pos in bucket */
	int hash_algo_id;

	pr_debug("%s: hash %d, pos %d\n", __func__, hash, pos);
	pr_debug("%s: index %d -> mac %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, idx,
		 e->mac[0], e->mac[1], e->mac[2], e->mac[3], e->mac[4], e->mac[5]);

	if (idx < 0x4000)
		hash_algo_id = sw_r32(RTL931X_L2_CTRL) & BIT(0);
	else
		hash_algo_id = (sw_r32(RTL931X_L2_CTRL) & BIT(1)) >> 1;

	if (hash_algo_id == 0)
		e->hash_msb = (e->rvid >> 2) & 0x1;
	else
		e->hash_msb = (e->rvid >> 11) & 0x1;

	rtl931x_fill_l2_row(r, e);
	pr_debug("%s: %d: %08x %08x %08x\n", __func__, idx, r[0], r[1], r[2]);

	for (int i = 0; i < 4; i++)
		sw_w32(r[i], rtl_table_data(q, i));

	rtl_table_write(q, idx);
	rtl_table_release(q);
}

static void rtl931x_vlan_fwd_on_inner(int port, bool is_set)
{
	/* Always set all tag modes to fwd based on either inner or outer tag */
	if (is_set)
		sw_w32_mask(0xf, 0, RTL931X_VLAN_PORT_FWD + (port << 2));
	else
		sw_w32_mask(0, 0xf, RTL931X_VLAN_PORT_FWD + (port << 2));
}

static void rtl931x_vlan_profile_setup(int profile)
{
	u32 p[7];

	pr_debug("In %s\n", __func__);

	if (profile > 15)
		return;

	/* Unicast routing is enabled per L3 ingress interface on this family,
	 * not in the VLAN profile - there is nothing to set for it here.
	 */
	p[0] = sw_r32(RTL931X_VLAN_PROFILE_SET(profile));

	p[1] = 0x1FFFFFF; /* L2 unknwon MC flooding portmask all ports, including the CPU-port */
	p[2] = 0xFFFFFFFF;
	p[3] = 0x1FFFFFF; /* IPv4 unknwon MC flooding portmask */
	p[4] = 0xFFFFFFFF;
	p[5] = 0x1FFFFFF; /* IPv6 unknwon MC flooding portmask */
	p[6] = 0xFFFFFFFF;

	for (int i = 0; i < 7; i++)
		sw_w32(p[i], RTL931X_VLAN_PROFILE_SET(profile) + i * 4);
	pr_debug("Leaving %s\n", __func__);
}

/* The two 14-bit fields (100M/10M and 1G/2.5G/5G/10G) reset to 12288; only
 * ever RMW them, the global length-check enable is left untouched.
 */
void rtl931x_port_max_frame_set(int port, int frame_len)
{
	frame_len = min(frame_len, RTL931X_MAX_FRAME_LEN);

	/* The per-port register array covers ports 0-55 only; the CPU port
	 * has its own register with RX/TX length fields, which resets to
	 * 1598 rather than the 12288 of the regular ports.
	 */
	if (port == 56)
		sw_w32_mask(0x0fffffff, (frame_len << 14) | frame_len,
			    RTL931X_MAC_L2_CPU_MAX_LEN_CTRL);
	else
		sw_w32_mask(0x0fffffff, (frame_len << 14) | frame_len,
			    RTL931X_MAC_L2_PORT_MAX_LEN_CTRL(port));
}

static void rtl931x_l2_learning_setup(void)
{
	/* Portmask for flooding broadcast traffic */
	rtl839x_set_port_reg_be(0x1FFFFFFFFFFFFFF, RTL931X_L2_BC_FLD_PMSK);

	/* Portmask for flooding unicast traffic with unknown destination */
	rtl839x_set_port_reg_be(0x1FFFFFFFFFFFFFF, RTL931X_L2_UNKN_UC_FLD_PMSK);

	/* Limit learning to maximum: 64k entries, after that just flood (bits 0-2) */
	sw_w32((0xffff << 3) | FORWARD, RTL931X_L2_LRN_CONSTRT_CTRL);
}

static void rtldsa_931x_enable_learning(int port, bool enable)
{
	/* Limit learning to maximum: 64k entries */
	sw_w32_mask(GENMASK(18, 3), enable ? (0xfffe << 3) : 0,
		    RTL931X_L2_LRN_PORT_CONSTRT_CTRL + port * 4);
}

/* There is deliberately no unknown-multicast counterpart: the hardware
 * offers no per-port control over it. The VLAN profile flood portmasks,
 * the profile lookup-miss actions and the per-port multicast lookup-miss
 * actions were all measured to have no effect on this family, and the
 * vendor firmware exposes lookup-miss and flood-port configuration for
 * broadcast and unicast only. Leaving the operation unimplemented makes
 * the bridge keep handling multicast flooding itself instead of assuming
 * the switch honours the flag. Unknown multicast can be rate limited
 * through storm control instead.
 */
static void rtldsa_931x_enable_bcast_flood(int port, bool enable)
{
	rtl839x_mask_port_reg_be(BIT_ULL(port), enable ? BIT_ULL(port) : 0,
				 RTL931X_L2_BC_FLD_PMSK);
}

/* Unknown-unicast flooding is gated per egress port by the flood portmask;
 * the learn-limit exceed action shares no relationship with it.
 */
static void rtldsa_931x_enable_flood(int port, bool enable)
{
	rtl839x_mask_port_reg_be(BIT_ULL(port), enable ? BIT_ULL(port) : 0,
				 RTL931X_L2_UNKN_UC_FLD_PMSK);
}

static const u32 rtl931x_storm_ctrl_base[] = {
	[RTLDSA_STORM_UC] = RTL931X_STORM_PORT_UC_CTRL(0),
	[RTLDSA_STORM_MC] = RTL931X_STORM_PORT_MC_CTRL(0),
	[RTLDSA_STORM_BC] = RTL931X_STORM_PORT_BC_CTRL(0),
};

static const u32 rtl931x_storm_lb_rst_base[] = {
	[RTLDSA_STORM_UC] = RTL931X_STORM_PORT_UC_LB_RST(0),
	[RTLDSA_STORM_MC] = RTL931X_STORM_PORT_MC_LB_RST(0),
	[RTLDSA_STORM_BC] = RTL931X_STORM_PORT_BC_LB_RST(0),
};

/* Program the per-port storm control rate of one traffic class, in packets
 * per second; 0 disables the limiter. One RATE unit is exactly 1 pps, as the
 * setup programs the PPS leaky-bucket tick/token values the SDK derives for
 * the running system clock (dal_mango_construct.c). The disable path
 * restores the measured reset posture (rate wide open, burst 0x8000, EN
 * clear) instead of zeros, and the enable path programs the SDK packet-mode
 * default burst: on RTL930x a zeroed burst silently blocked the class
 * entirely, and the reset burst of 0x8000 packets would make a fresh limiter
 * look broken for its first 32K packets. The TYPE selection is a separate
 * knob and is preserved here. The leaky bucket is reset after any change,
 * as it keeps stale credit otherwise (SDK
 * dal_mango_rate_portStormCtrlRate_set).
 */
int rtl931x_storm_port_rate_set(struct rtl838x_switch_priv *priv, int port,
				enum rtldsa_storm_class class, u32 pps)
{
	u32 addr, v;

	if (class < RTLDSA_STORM_UC || class > RTLDSA_STORM_BC ||
	    port < 0 || port >= priv->cpu_port)
		return -EINVAL;
	if (pps > RTL931X_STORM_RATE_M)
		return -ERANGE;

	addr = rtl931x_storm_ctrl_base[class] + (port << 3);

	mutex_lock(&priv->reg_mutex);
	v = sw_r32(addr) & RTL931X_STORM_TYPE_INCL_KNOWN;
	if (pps)
		v |= RTL931X_STORM_EN | pps;
	else
		v |= RTL931X_STORM_RATE_M;
	sw_w32(v, addr);
	sw_w32(pps ? RTL931X_STORM_DFLT_BURST_PPS : RTL931X_STORM_RESET_BURST,
	       addr + 4);
	sw_w32(BIT(port % 32),
	       rtl931x_storm_lb_rst_base[class] + ((port >> 5) << 2));
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

u32 rtl931x_storm_port_rate_get(int port, enum rtldsa_storm_class class)
{
	u32 v;

	if (class < RTLDSA_STORM_UC || class > RTLDSA_STORM_BC)
		return 0;

	v = sw_r32(rtl931x_storm_ctrl_base[class] + (port << 3));

	return (v & RTL931X_STORM_EN) ? (v & RTL931X_STORM_RATE_M) : 0;
}

/* Select whether the UC/MC limiter counts unknown-destination traffic only
 * or all traffic of the class (SDK dal_mango_rate_portStormCtrlTypeSel_set:
 * STORM_SEL_UNKNOWN vs STORM_SEL_UNKNOWN_AND_KNOWN). Broadcast always counts
 * everything and has no TYPE bit.
 *
 * The bit behaves exactly as documented for unicast: measured against an
 * unlearned destination MAC, unknown-only limits the flooded traffic and
 * leaves learnt traffic untouched, and clearing the selection then limits
 * both. Multicast is different -- unknown-only was measured to match no
 * traffic at all, with and without IGMP snooping, so a multicast limiter
 * left in that mode is silently dead. That is the same silicon gap seen
 * from the flood-mask side, where no per-port unknown-multicast flood
 * control has any effect: this family never classifies multicast as
 * unknown. Multicast is therefore pinned to counting everything, and
 * asking for unknown-only is refused rather than quietly doing nothing.
 */
int rtl931x_storm_port_type_set(struct rtl838x_switch_priv *priv, int port,
				enum rtldsa_storm_class class, bool incl_known)
{
	if (class < RTLDSA_STORM_UC || class > RTLDSA_STORM_BC ||
	    port < 0 || port >= priv->cpu_port)
		return -EINVAL;
	if (class == RTLDSA_STORM_BC)
		return -EOPNOTSUPP;
	if (class == RTLDSA_STORM_MC && !incl_known)
		return -EOPNOTSUPP;

	mutex_lock(&priv->reg_mutex);
	sw_w32_mask(RTL931X_STORM_TYPE_INCL_KNOWN,
		    incl_known ? RTL931X_STORM_TYPE_INCL_KNOWN : 0,
		    rtl931x_storm_ctrl_base[class] + (port << 3));
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

bool rtl931x_storm_port_type_get(int port, enum rtldsa_storm_class class)
{
	if (class < RTLDSA_STORM_UC || class >= RTLDSA_STORM_BC)
		return false;

	return !!(sw_r32(rtl931x_storm_ctrl_base[class] + (port << 3)) &
		  RTL931X_STORM_TYPE_INCL_KNOWN);
}

/* Bring per-port storm control into a known state: PPS mode on all ports,
 * every limiter disabled in the measured reset posture (rate wide open,
 * burst 0x8000, EN clear), unknown-destination-only for UC (TYPE clear) and
 * count-everything for MC, whose unknown-only mode matches no traffic on
 * this family (see rtl931x_storm_port_type_set).
 * Unlike Longan, whose SDK never touches the storm leaky-bucket timers, the
 * Mango SDK programs the global PPS tick/token at init from the system
 * clock (dal_mango_construct.c); with those values one RATE unit is exactly
 * 1 pps. The reset values are logged first, for reference. STORM_LB_CTRL
 * (the byte-mode timer) is left alone as all ports are pinned to packet
 * mode. The CPU port is left untouched so trapped/injected traffic is
 * never storm-limited.
 */
void rtl931x_storm_control_init(struct rtl838x_switch_priv *priv)
{
	u64 port_mask = GENMASK_ULL(priv->cpu_port - 1, 0);
	u32 tick, tkn;

	pr_info("%s: STORM_LB_CTRL %08x, STORM_LB_PPS_CTRL %08x\n", __func__,
		sw_r32(RTL931X_STORM_LB_CTRL), sw_r32(RTL931X_STORM_LB_PPS_CTRL));

	switch (FIELD_GET(RTL931X_SYS_CLK_SEL_M,
			  sw_r32(RTL931X_MAC_L2_GLOBAL_CTRL2))) {
	case 1: /* 325 MHz */
		tick = RTL931X_STORM_LB_PPS_TICK_325M;
		tkn = RTL931X_STORM_LB_PPS_TKN_325M;
		break;
	case 2: /* 175 MHz */
		tick = RTL931X_STORM_LB_PPS_TICK_175M;
		tkn = RTL931X_STORM_LB_PPS_TKN_175M;
		break;
	default: /* 650 MHz */
		tick = RTL931X_STORM_LB_PPS_TICK_650M;
		tkn = RTL931X_STORM_LB_PPS_TKN_650M;
		break;
	}
	sw_w32(FIELD_PREP(RTL931X_STORM_LB_PPS_TICK_M, tick) |
	       FIELD_PREP(RTL931X_STORM_LB_PPS_TKN_M, tkn),
	       RTL931X_STORM_LB_PPS_CTRL);

	/* All ports count packets (PPS), not bytes: two mode words */
	sw_w32(0, RTL931X_STORM_PORT_CTRL(0));
	sw_w32(0, RTL931X_STORM_PORT_CTRL(32));

	/* Reset posture on every port and class, with a leaky-bucket reset */
	for (int p = 0; p < priv->cpu_port; p++) {
		for (enum rtldsa_storm_class c = RTLDSA_STORM_UC;
		     c <= RTLDSA_STORM_BC; c++)
			rtl931x_storm_port_rate_set(priv, p, c, 0);

		rtl931x_storm_port_type_set(priv, p, RTLDSA_STORM_MC, true);
	}

	/* Clear stale exceed flags (write-1-to-clear, two words per class) */
	sw_w32((u32)port_mask, RTL931X_STORM_PORT_UC_EXCEED(0));
	sw_w32((u32)(port_mask >> 32), RTL931X_STORM_PORT_UC_EXCEED(32));
	sw_w32((u32)port_mask, RTL931X_STORM_PORT_MC_EXCEED(0));
	sw_w32((u32)(port_mask >> 32), RTL931X_STORM_PORT_MC_EXCEED(32));
	sw_w32((u32)port_mask, RTL931X_STORM_PORT_BC_EXCEED(0));
	sw_w32((u32)(port_mask >> 32), RTL931X_STORM_PORT_BC_EXCEED(32));
}

static u64 rtl931x_read_mcast_pmask(int idx)
{
	u64 portmask;
	/* Read MC_PMSK (2) via register RTL9310_TBL_0 */
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 2);

	rtl_table_read(q, idx);
	portmask = sw_r32(rtl_table_data(q, 0));
	portmask <<= 32;
	portmask |= sw_r32(rtl_table_data(q, 1));
	portmask >>= 7;
	rtl_table_release(q);

	pr_debug("%s: Index idx %d has portmask %016llx\n", __func__, idx, portmask);

	return portmask;
}

static void rtl931x_write_mcast_pmask(int idx, u64 portmask)
{
	u64 pm = portmask;

	/* Access MC_PMSK (2) via register RTL9310_TBL_0 */
	struct table_reg *q = rtl_table_get(RTL9310_TBL_0, 2);

	pr_debug("%s: Index idx %d has portmask %016llx\n", __func__, idx, pm);
	pm <<= 7;
	sw_w32((u32)(pm >> 32), rtl_table_data(q, 0));
	sw_w32((u32)pm, rtl_table_data(q, 1));
	rtl_table_write(q, idx);
	rtl_table_release(q);
}

static int rtl931x_set_ageing_time(unsigned long msec)
{
	int t = sw_r32(RTL931X_L2_AGE_CTRL);

	t &= 0x1FFFFF;
	t = (t * 8) / 10;
	pr_debug("L2 AGING time: %d sec\n", t);

	t = (msec / 100 + 7) / 8;
	t = t > 0x1FFFFF ? 0x1FFFFF : t;
	sw_w32_mask(0x1FFFFF, t, RTL931X_L2_AGE_CTRL);
	pr_debug("Dynamic aging for ports: %x\n", sw_r32(RTL931X_L2_PORT_AGE_CTRL));

	return 0;
}

static void rtl931x_pie_lookup_enable(struct rtl838x_switch_priv *priv, int index)
{
	int block = index / PIE_BLOCK_SIZE;

	sw_w32_mask(0, BIT(block), RTL931X_PIE_BLK_LOOKUP_CTRL);
}

/* Fills the data in the intermediate representation in the pie_rule structure
 * into a data field for a given template field field_type
 * TODO: This function looks very similar to the function of the rtl9300, but
 * since it uses the physical template_field_id, which are different for each
 * SoC and there are other field types, it is actually not. If we would also use
 * an intermediate representation for a field type, we would could have one
 * pie_data_fill function for all SoCs, provided we have also for each SoC a
 * function to map between physical and intermediate field type
 */
static int rtl931x_pie_data_fill(enum template_field_id field_type, struct pie_rule *pr, u16 *data, u16 *data_m)
{
	*data = *data_m = 0;

	switch (field_type) {
	case TEMPLATE_FIELD_SPM0:
		*data = pr->spm;
		*data_m = pr->spm_m;
		break;
	case TEMPLATE_FIELD_SPM1:
		*data = pr->spm >> 16;
		*data_m = pr->spm_m >> 16;
		break;
	case TEMPLATE_FIELD_SPM2:
		*data = pr->spm >> 32;
		*data_m = pr->spm_m >> 32;
		break;
	case TEMPLATE_FIELD_SPM3:
		*data = pr->spm >> 48;
		*data_m = pr->spm_m >> 48;
		break;
	case TEMPLATE_FIELD_OTAG:
		*data = pr->otag;
		*data_m = pr->otag_m;
		break;
	case TEMPLATE_FIELD_SMAC0:
		*data = pr->smac[4];
		*data = (*data << 8) | pr->smac[5];
		*data_m = pr->smac_m[4];
		*data_m = (*data_m << 8) | pr->smac_m[5];
		break;
	case TEMPLATE_FIELD_SMAC1:
		*data = pr->smac[2];
		*data = (*data << 8) | pr->smac[3];
		*data_m = pr->smac_m[2];
		*data_m = (*data_m << 8) | pr->smac_m[3];
		break;
	case TEMPLATE_FIELD_SMAC2:
		*data = pr->smac[0];
		*data = (*data << 8) | pr->smac[1];
		*data_m = pr->smac_m[0];
		*data_m = (*data_m << 8) | pr->smac_m[1];
		break;
	case TEMPLATE_FIELD_DMAC0:
		*data = pr->dmac[4];
		*data = (*data << 8) | pr->dmac[5];
		*data_m = pr->dmac_m[4];
		*data_m = (*data_m << 8) | pr->dmac_m[5];
		break;
	case TEMPLATE_FIELD_DMAC1:
		*data = pr->dmac[2];
		*data = (*data << 8) | pr->dmac[3];
		*data_m = pr->dmac_m[2];
		*data_m = (*data_m << 8) | pr->dmac_m[3];
		break;
	case TEMPLATE_FIELD_DMAC2:
		*data = pr->dmac[0];
		*data = (*data << 8) | pr->dmac[1];
		*data_m = pr->dmac_m[0];
		*data_m = (*data_m << 8) | pr->dmac_m[1];
		break;
	case TEMPLATE_FIELD_ETHERTYPE:
		*data = pr->ethertype;
		*data_m = pr->ethertype_m;
		break;
	case TEMPLATE_FIELD_ITAG:
		*data = pr->itag;
		*data_m = pr->itag_m;
		break;
	case TEMPLATE_FIELD_SIP0:
		if (pr->is_ipv6) {
			*data = pr->sip6.s6_addr16[7];
			*data_m = pr->sip6_m.s6_addr16[7];
		} else {
			*data = pr->sip;
			*data_m = pr->sip_m;
		}
		break;
	case TEMPLATE_FIELD_SIP1:
		if (pr->is_ipv6) {
			*data = pr->sip6.s6_addr16[6];
			*data_m = pr->sip6_m.s6_addr16[6];
		} else {
			*data = pr->sip >> 16;
			*data_m = pr->sip_m >> 16;
		}
		break;
	case TEMPLATE_FIELD_SIP2:
	case TEMPLATE_FIELD_SIP3:
	case TEMPLATE_FIELD_SIP4:
	case TEMPLATE_FIELD_SIP5:
	case TEMPLATE_FIELD_SIP6:
	case TEMPLATE_FIELD_SIP7:
		*data = pr->sip6.s6_addr16[5 - (field_type - TEMPLATE_FIELD_SIP2)];
		*data_m = pr->sip6_m.s6_addr16[5 - (field_type - TEMPLATE_FIELD_SIP2)];
		break;
	case TEMPLATE_FIELD_DIP0:
		if (pr->is_ipv6) {
			*data = pr->dip6.s6_addr16[7];
			*data_m = pr->dip6_m.s6_addr16[7];
		} else {
			*data = pr->dip;
			*data_m = pr->dip_m;
		}
		break;
	case TEMPLATE_FIELD_DIP1:
		if (pr->is_ipv6) {
			*data = pr->dip6.s6_addr16[6];
			*data_m = pr->dip6_m.s6_addr16[6];
		} else {
			*data = pr->dip >> 16;
			*data_m = pr->dip_m >> 16;
		}
		break;
	case TEMPLATE_FIELD_DIP2:
	case TEMPLATE_FIELD_DIP3:
	case TEMPLATE_FIELD_DIP4:
	case TEMPLATE_FIELD_DIP5:
	case TEMPLATE_FIELD_DIP6:
	case TEMPLATE_FIELD_DIP7:
		*data = pr->dip6.s6_addr16[5 - (field_type - TEMPLATE_FIELD_DIP2)];
		*data_m = pr->dip6_m.s6_addr16[5 - (field_type - TEMPLATE_FIELD_DIP2)];
		break;
	case TEMPLATE_FIELD_IP_TOS_PROTO:
		*data = pr->tos_proto;
		*data_m = pr->tos_proto_m;
		break;
	case TEMPLATE_FIELD_L4_SPORT:
		*data = pr->sport;
		*data_m = pr->sport_m;
		break;
	case TEMPLATE_FIELD_L4_DPORT:
		*data = pr->dport;
		*data_m = pr->dport_m;
		break;
	case TEMPLATE_FIELD_DSAP_SSAP:
		*data = pr->dsap_ssap;
		*data_m = pr->dsap_ssap_m;
		break;
	case TEMPLATE_FIELD_TCP_INFO:
		*data = pr->tcp_info;
		*data_m = pr->tcp_info_m;
		break;
	case TEMPLATE_FIELD_RANGE_CHK:
		pr_debug("TEMPLATE_FIELD_RANGE_CHK: not configured\n");
		break;
	default:
		pr_debug("%s: unknown field %d\n", __func__, field_type);
		return -1;
	}

	return 0;
}

/* Reads the intermediate representation of the templated match-fields of the
 * PIE rule in the pie_rule structure and fills in the raw data fields in the
 * raw register space r[].
 * The register space configuration size is identical for the RTL8380/90 and RTL9300,
 * however the RTL931X has 2 more registers / fields and the physical field-ids are different
 * on all SoCs
 * On the RTL9300 the mask fields are not word-aligend!
 */
static void rtl931x_write_pie_templated(u32 r[], struct pie_rule *pr, enum template_field_id t[])
{
	for (int i = 0; i < N_FIXED_FIELDS_RTL931X; i++) {
		u16 data, data_m;
		int dbit = 480 + 16 * i;	/* FIELD_i */
		int mbit = 240 + 16 * i;	/* BMSK_FIELD_i */

		rtl931x_pie_data_fill(t[i], pr, &data, &data_m);

		/* The 704-bit entry sits in r[0..21] top-down; both the data
		 * and mask fields are half-word aligned on the RTL931X.
		 */
		r[21 - dbit / 32] |= ((u32)data) << (dbit % 32);
		r[21 - mbit / 32] |= ((u32)data_m) << (mbit % 32);
	}
}

// Currently unused
// static void rtl931x_read_pie_fixed_fields(u32 r[], struct pie_rule *pr)
// {
// 	pr->mgnt_vlan = r[7] & BIT(31);
// 	if (pr->phase == PHASE_IACL)
// 		pr->dmac_hit_sw = r[7] & BIT(30);
// 	else  /* TODO: EACL/VACL phase handling */
// 		pr->content_too_deep = r[7] & BIT(30);
// 	pr->not_first_frag = r[7]  & BIT(29);
// 	pr->frame_type_l4 = (r[7] >> 26) & 7;
// 	pr->frame_type = (r[7] >> 24) & 3;
// 	pr->otag_fmt = (r[7] >> 23) & 1;
// 	pr->itag_fmt = (r[7] >> 22) & 1;
// 	pr->otag_exist = (r[7] >> 21) & 1;
// 	pr->itag_exist = (r[7] >> 20) & 1;
// 	pr->frame_type_l2 = (r[7] >> 18) & 3;
// 	pr->igr_normal_port = (r[7] >> 17) & 1;
// 	pr->tid = (r[7] >> 16) & 1;

// 	pr->mgnt_vlan_m = r[14] & BIT(15);
// 	if (pr->phase == PHASE_IACL)
// 		pr->dmac_hit_sw_m = r[14] & BIT(14);
// 	else
// 		pr->content_too_deep_m = r[14] & BIT(14);
// 	pr->not_first_frag_m = r[14] & BIT(13);
// 	pr->frame_type_l4_m = (r[14] >> 10) & 7;
// 	pr->frame_type_m = (r[14] >> 8) & 3;
// 	pr->otag_fmt_m = r[14] & BIT(7);
// 	pr->itag_fmt_m = r[14] & BIT(6);
// 	pr->otag_exist_m = r[14] & BIT(5);
// 	pr->itag_exist_m = r[14] & BIT (4);
// 	pr->frame_type_l2_m = (r[14] >> 2) & 3;
// 	pr->igr_normal_port_m = r[14] & BIT(1);
// 	pr->tid_m = r[14] & 1;

// 	pr->valid = r[15] & BIT(31);
// 	pr->cond_not = r[15] & BIT(30);
// 	pr->cond_and1 = r[15] & BIT(29);
// 	pr->cond_and2 = r[15] & BIT(28);
// }

static void rtl931x_write_pie_fixed_fields(u32 r[],  struct pie_rule *pr)
{
	r[7] |= pr->mgnt_vlan ? BIT(31) : 0;
	if (pr->phase == PHASE_IACL)
		r[7] |= pr->dmac_hit_sw ? BIT(30) : 0;
	else
		r[7] |= pr->content_too_deep ? BIT(30) : 0;
	r[7] |= pr->not_first_frag ? BIT(29) : 0;
	r[7] |= ((u32)(pr->frame_type_l4 & 0x7)) << 26;
	r[7] |= ((u32)(pr->frame_type & 0x3)) << 24;
	r[7] |= pr->otag_fmt ? BIT(23) : 0;
	r[7] |= pr->itag_fmt ? BIT(22) : 0;
	r[7] |= pr->otag_exist ? BIT(21) : 0;
	r[7] |= pr->itag_exist ? BIT(20) : 0;
	r[7] |= ((u32)(pr->frame_type_l2 & 0x3)) << 18;
	r[7] |= pr->igr_normal_port ? BIT(17) : 0;
	r[7] |= ((u32)(pr->tid & 0x1)) << 16;

	r[14] |= pr->mgnt_vlan_m ? BIT(15) : 0;
	if (pr->phase == PHASE_IACL)
		r[14] |= pr->dmac_hit_sw_m ? BIT(14) : 0;
	else
		r[14] |= pr->content_too_deep_m ? BIT(14) : 0;
	r[14] |= pr->not_first_frag_m ? BIT(13) : 0;
	r[14] |= ((u32)(pr->frame_type_l4_m & 0x7)) << 10;
	r[14] |= ((u32)(pr->frame_type_m & 0x3)) << 8;
	r[14] |= pr->otag_fmt_m ? BIT(7) : 0;
	r[14] |= pr->itag_fmt_m ? BIT(6) : 0;
	r[14] |= pr->otag_exist_m ? BIT(5) : 0;
	r[14] |= pr->itag_exist_m ? BIT(4) : 0;
	r[14] |= ((u32)(pr->frame_type_l2_m & 0x3)) << 2;
	r[14] |= pr->igr_normal_port_m ? BIT(1) : 0;
	r[14] |= (u32)(pr->tid_m & 0x1);

	r[15] |= pr->valid ? BIT(31) : 0;
	r[15] |= pr->cond_not ? BIT(30) : 0;
	r[15] |= pr->cond_and1 ? BIT(29) : 0;
	r[15] |= pr->cond_and2 ? BIT(28) : 0;
}

static void rtl931x_write_pie_action(u32 r[],  struct pie_rule *pr)
{
	/* Either drop or forward */
	if (pr->drop) {
		r[15] |= BIT(11) | BIT(12) | BIT(13); /* Do Green, Yellow and Red drops */
		/* Actually DROP, not PERMIT in Green / Yellow / Red */
		r[16] |= BIT(27) | BIT(28) | BIT(29);
	} else {
		r[15] |= pr->fwd_sel ? BIT(14) : 0;
		r[16] |= pr->fwd_act << 24;
		r[16] |= BIT(21); /* We overwrite any drop */
	}
	if (pr->phase == PHASE_VACL)
		r[16] |= pr->fwd_sa_lrn ? BIT(22) : 0;
	r[15] |= pr->bypass_sel ? BIT(10) : 0;
	r[15] |= pr->nopri_sel ? BIT(21) : 0;
	r[15] |= pr->tagst_sel ? BIT(20) : 0;
	r[15] |= pr->ovid_sel ? BIT(18) : 0;
	r[15] |= pr->ivid_sel ? BIT(16) : 0;
	r[15] |= pr->meter_sel ? BIT(27) : 0;
	r[15] |= pr->mir_sel ? BIT(15) : 0;
	r[15] |= pr->log_sel ? BIT(26) : 0;

	r[16] |= ((u32)(pr->fwd_data & 0xfff)) << 9;
/*	r[15] |= pr->log_octets ? BIT(31) : 0; */
	r[15] |= (u32)(pr->meter_data) >> 2;
	r[16] |= (((u32)(pr->meter_data) >> 7) & 0x3) << 29;

	r[16] |= ((u32)(pr->ivid_act & 0x3)) << 21;
	r[15] |= ((u32)(pr->ivid_data & 0xfff)) << 9;
	r[16] |= ((u32)(pr->ovid_act & 0x3)) << 30;
	r[16] |= ((u32)(pr->ovid_data & 0xfff)) << 16;
	r[16] |= ((u32)(pr->mir_data & 0x3)) << 6;
	r[17] |= ((u32)(pr->tagst_data & 0xf)) << 28;
	r[17] |= ((u32)(pr->nopri_data & 0x7)) << 25;
	r[17] |= pr->bypass_ibc_sc ? BIT(16) : 0;
}

static void rtl931x_pie_rule_dump_raw(u32 r[])
{
	pr_debug("Raw IACL table entry:\n");
	pr_debug("r 0 - 7: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		 r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	pr_debug("r 8 - 15: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		 r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);
	pr_debug("r 16 - 18: %08x %08x %08x\n", r[16], r[17], r[18]);
	pr_debug("Match  : %08x %08x %08x %08x %08x %08x\n", r[0], r[1], r[2], r[3], r[4], r[5]);
	pr_debug("Fixed  : %06x\n", r[6] >> 8);
	pr_debug("Match M: %08x %08x %08x %08x %08x %08x\n",
		 (r[6] << 24) | (r[7] >> 8), (r[7] << 24) | (r[8] >> 8), (r[8] << 24) | (r[9] >> 8),
		 (r[9] << 24) | (r[10] >> 8), (r[10] << 24) | (r[11] >> 8),
		 (r[11] << 24) | (r[12] >> 8));
	pr_debug("R[13]:   %08x\n", r[13]);
	pr_debug("Fixed M: %06x\n", ((r[12] << 16) | (r[13] >> 16)) & 0xffffff);
	pr_debug("Valid / not / and1 / and2 : %1x\n", (r[13] >> 12) & 0xf);
	pr_debug("r 13-16: %08x %08x %08x %08x\n", r[13], r[14], r[15], r[16]);
}

static int rtl931x_pie_rule_write(struct rtl838x_switch_priv *priv, int idx, struct pie_rule *pr)
{
	/* Access IACL table (0) via register 1, the table size is 4096 */
	struct table_reg *q = rtl_table_get(RTL9310_TBL_1, 0);
	u32 r[22];
	int block = idx / PIE_BLOCK_SIZE;
	u32 t_select = sw_r32(RTL931X_PIE_BLK_TMPLTE_CTRL(block));

	pr_debug("%s: %d, t_select: %08x\n", __func__, idx, t_select);

	for (int i = 0; i < 22; i++)
		r[i] = 0;

	if (!pr->valid) {
		rtl_table_write(q, idx);
		rtl_table_release(q);
		return 0;
	}
	rtl931x_write_pie_fixed_fields(r, pr);

	pr_debug("%s: template %d\n", __func__, (t_select >> (pr->tid * 4)) & 0xf);
	rtl931x_write_pie_templated(r, pr, fixed_templates[(t_select >> (pr->tid * 4)) & 0xf]);

	rtl931x_write_pie_action(r, pr);

	rtl931x_pie_rule_dump_raw(r);

	for (int i = 0; i < 22; i++)
		sw_w32(r[i], rtl_table_data(q, i));

	rtl_table_write(q, idx);
	rtl_table_release(q);

	return 0;
}

static bool rtl931x_pie_templ_has(int t, enum template_field_id field_type)
{
	for (int i = 0; i < N_FIXED_FIELDS_RTL931X; i++) {
		enum template_field_id ft = fixed_templates[t][i];

		if (field_type == ft)
			return true;
	}

	return false;
}

/* Verify that the rule pr is compatible with a given template t in block
 * Note that this function is SoC specific since the values of e.g. TEMPLATE_FIELD_SIP0
 * depend on the SoC
 */
static int rtl931x_pie_verify_template(struct rtl838x_switch_priv *priv,
				       struct pie_rule *pr, int t, int block)
{
	int i;

	if (!pr->is_ipv6 && pr->sip_m && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SIP0))
		return -1;

	if (!pr->is_ipv6 && pr->dip_m && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_DIP0))
		return -1;

	if (pr->is_ipv6) {
		if ((pr->sip6_m.s6_addr32[0] ||
		     pr->sip6_m.s6_addr32[1] ||
		     pr->sip6_m.s6_addr32[2] ||
		     pr->sip6_m.s6_addr32[3]) &&
		    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SIP2))
			return -1;
		if ((pr->dip6_m.s6_addr32[0] ||
		     pr->dip6_m.s6_addr32[1] ||
		     pr->dip6_m.s6_addr32[2] ||
		     pr->dip6_m.s6_addr32[3]) &&
		    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_DIP2))
			return -1;
	}

	if (ether_addr_to_u64(pr->smac) && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SMAC0))
		return -1;

	if (ether_addr_to_u64(pr->dmac) && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_DMAC0))
		return -1;

	/* A rule whose match fields are absent from the template would be
	 * programmed without them and match more traffic than requested.
	 * Reject the template so selection falls through to one that fits
	 * (templates 1-3 carry the L4 port fields).
	 */
	if (pr->sport_m && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_L4_SPORT))
		return -1;

	if (pr->dport_m && !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_L4_DPORT))
		return -1;

	/* The source port mask used for per-port scoping spans SPM0-3
	 * (16 ports each); without the matching template field the rule
	 * would match on more ports than requested.
	 */
	if ((pr->spm_m & 0xffffULL) &&
	    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SPM0))
		return -1;

	if ((pr->spm_m & 0xffff0000ULL) &&
	    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SPM1))
		return -1;

	if ((pr->spm_m & 0xffff00000000ULL) &&
	    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SPM2))
		return -1;

	if ((pr->spm_m & 0xffff000000000000ULL) &&
	    !rtl931x_pie_templ_has(t, TEMPLATE_FIELD_SPM3))
		return -1;

	/* TODO: Check more */

	i = find_first_zero_bit(&priv->pie_use_bm[block * 4], PIE_BLOCK_SIZE);

	if (i >= PIE_BLOCK_SIZE)
		return -1;

	return i + PIE_BLOCK_SIZE * block;
}

static void rtl931x_packet_cntr_clear(int counter);

static int rtl931x_pie_rule_add(struct rtl838x_switch_priv *priv, struct pie_rule *pr)
{
	int idx, block, j;
	int min_block = 0;
	int max_block = priv->n_pie_blocks / 2;

	if (pr->is_egress) {
		min_block = max_block;
		max_block = priv->n_pie_blocks;
	}
	pr_debug("In %s\n", __func__);

	mutex_lock(&priv->pie_mutex);

	for (block = min_block; block < max_block; block++) {
		for (j = 0; j < 2; j++) {
			int t = (sw_r32(RTL931X_PIE_BLK_TMPLTE_CTRL(block)) >> (j * 4)) & 0xf;

			pr_debug("Testing block %d, template %d, template id %d\n", block, j, t);
			pr_debug("%s: %08x\n",
				 __func__, sw_r32(RTL931X_PIE_BLK_TMPLTE_CTRL(block)));
			idx = rtl931x_pie_verify_template(priv, pr, t, block);
			if (idx >= 0)
				break;
		}
		if (j < 2)
			break;
	}

	/* Ingress rules only ever search the first half of the blocks, so the
	 * exhaustion test has to be against max_block: comparing against
	 * n_pie_blocks never fires for them and the code below would then run
	 * with the -1 that verify_template returned on its last attempt.
	 */
	if (block >= max_block) {
		mutex_unlock(&priv->pie_mutex);
		return -EOPNOTSUPP;
	}

	pr_debug("Using block: %d, index %d, template-id %d\n", block, idx, j);
	set_bit(idx, priv->pie_use_bm);

	pr->valid = true;
	pr->tid = j;  /* Mapped to template number */
	pr->tid_m = 0x1;
	pr->id = idx;

	/* Mango carries no counter-index field in the IACL action, only the
	 * ACT_MSK_LOG enable: the FLOW_CNTR entry at the rule's own physical
	 * PIE entry index does the counting (SDK dal_mango_acl_statCnt_get
	 * reads FLOW_CNTRt at the physical entry index). Point the caller's
	 * counter handle at it; the logically allocated packet counter index
	 * cannot be programmed into hardware on this family and is unused.
	 * The rewritten handle is a PIE entry index, not a counter-bitmap
	 * allocation, and must never be released into packet_cntr_use_bm;
	 * the only releasing caller (L3 route teardown) is RTL930x-only.
	 */
	pr->log_data = idx;
	if (pr->log_sel)
		pr->packet_cntr = idx;

	rtl931x_pie_lookup_enable(priv, idx);
	rtl931x_pie_rule_write(priv, idx, pr);

	/* Start the rule's counter from a defined all-zero state; this also
	 * programs CNTR_MODE 0 (packet+byte counting) in case the FLOW_CNTR
	 * entry held garbage, and drops stale counts of a previous rule at
	 * this index (SDK _dal_mango_acl_tblEntry_del clears FLOW_CNTRt
	 * together with the PIE entry).
	 */
	if (pr->log_sel)
		rtl931x_packet_cntr_clear(idx);

	mutex_unlock(&priv->pie_mutex);

	return 0;
}

/* Delete a range of Packet Inspection Engine rules */
static int rtl931x_pie_rule_del(struct rtl838x_switch_priv *priv, int index_from, int index_to)
{
	u32 v = (index_from << 1) | (index_to << 13) | BIT(0);

	pr_debug("%s: from %d to %d\n", __func__, index_from, index_to);
	mutex_lock(&priv->reg_mutex);

	/* Write from-to and execute bit into control register */
	sw_w32(v, RTL931X_PIE_CLR_CTRL);

	/* Wait until command has completed */
	do {
	} while (sw_r32(RTL931X_PIE_CLR_CTRL) & BIT(0));

	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static void rtl931x_pie_rule_rm(struct rtl838x_switch_priv *priv, struct pie_rule *pr)
{
	int idx = pr->id;

	rtl931x_pie_rule_del(priv, idx, idx);
	clear_bit(idx, priv->pie_use_bm);
}

/* Number of FLOW_CNTR samples taken for the median when a counter keeps
 * changing between reads (SDK _dal_mango_acl_cntr_read,
 * DAL_MANGO_ACL_CNTR_OPER_MAX).
 */
#define RTL931X_FLOW_CNTR_SAMPLES	20

static int rtl931x_u64_cmp(const void *a, const void *b)
{
	const u64 l = *(const u64 *)a, r = *(const u64 *)b;

	if (l < r)
		return -1;
	return l > r;
}

/* Read the packet counter of a FLOW_CNTR entry. FLOW_CNTRt is table
 * access set 1, type 2, 4096 entries of 3 data words, one per physical
 * PIE entry and indexed by it (SDK rtk_mango_table_list.c,
 * dal_mango_acl_statCnt_get). Fields (SDK RTL9310_FLOW_CNTR_FIELDS):
 * PKT_CNTR is a 36-bit field at entry bits 58-93, BYTE_CNTR 42 bits at
 * 16-57, CNTR_MODE at 94-95. Table data word 0 carries the highest entry
 * word, so PKT_CNTR bits 0-5 sit in data word 1 bits 31-26 and bits 6-35
 * in data word 0 bits 29-0.
 */
static u64 rtl931x_flow_cntr_packets(struct table_reg *q, int entry)
{
	u32 hi, mid;

	rtl_table_read(q, entry);
	hi = sw_r32(rtl_table_data(q, 0));	/* entry bits 64-95 */
	mid = sw_r32(rtl_table_data(q, 1));	/* entry bits 32-63 */

	return ((u64)(hi & GENMASK(29, 0)) << 6) | (mid >> 26);
}

static u32 rtl931x_packet_cntr_read(int counter)
{
	/* Access FLOW_CNTR table (type 2) via register RTL9310_TBL_1 */
	struct table_reg *q = rtl_table_get(RTL9310_TBL_1, 2);
	u64 samples[RTL931X_FLOW_CNTR_SAMPLES];
	u64 first, second;

	first = rtl931x_flow_cntr_packets(q, counter);
	second = rtl931x_flow_cntr_packets(q, counter);
	if (first == second) {
		rtl_table_release(q);
		return (u32)first;
	}

	/* Mango counter-read erratum (SDK _dal_mango_acl_cntr_read): a read
	 * can return a corrupted value while the entry is being updated, so
	 * a single read is not reliable on this family under traffic. The
	 * SDK takes 20 samples and returns the median; its fast path
	 * additionally gates on the PIE entry hit status, which two
	 * agreeing reads subsume - no hit between reads means a stable
	 * counter.
	 */
	for (int i = 0; i < RTL931X_FLOW_CNTR_SAMPLES; i++)
		samples[i] = rtl931x_flow_cntr_packets(q, counter);
	rtl_table_release(q);

	sort(samples, RTL931X_FLOW_CNTR_SAMPLES, sizeof(samples[0]),
	     rtl931x_u64_cmp, NULL);

	/* The hardware counter is 36 bits wide; like the 930x op only the
	 * low 32 bits are returned, tc statistics cope with the wrap.
	 */
	return (u32)samples[RTL931X_FLOW_CNTR_SAMPLES / 2];
}

static void rtl931x_packet_cntr_clear(int counter)
{
	/* Access FLOW_CNTR table (type 2) via register RTL9310_TBL_1 */
	struct table_reg *q = rtl_table_get(RTL9310_TBL_1, 2);

	/* SDK dal_mango_acl_statCnt_clear zeroes the whole entry; CNTR_MODE
	 * 0 is packet+byte counting, so the counter keeps counting after a
	 * clear.
	 */
	for (int i = 0; i < 3; i++)
		sw_w32(0, rtl_table_data(q, i));
	rtl_table_write(q, counter);

	rtl_table_release(q);
}

static void rtl931x_pie_init(struct rtl838x_switch_priv *priv)
{
	u32 template_selectors;

	mutex_init(&priv->pie_mutex);

	pr_debug("%s\n", __func__);
	/* Enable ACL lookup on all ports, including CPU_PORT */
	for (int i = 0; i <= priv->cpu_port; i++)
		sw_w32(1, RTL931X_ACL_PORT_LOOKUP_CTRL(i));

	/* Include IPG in metering */
	sw_w32_mask(0, 1, RTL931X_METER_GLB_CTRL);

	/* Delete all present rules, block size is 128 on all SoC families */
	rtl931x_pie_rule_del(priv, 0, priv->n_pie_blocks * 128 - 1);

	/* Assign first half blocks 0-7 to VACL phase, second half to IACL */
	/* 3 bits are used for each block, values for PIE blocks are */
	/* 6: Disabled, 0: VACL, 1: IACL, 2: EACL */
	/* And for OpenFlow Flow blocks: 3: Ingress Flow table 0, */
	/* 4: Ingress Flow Table 3, 5: Egress flow table 0 */
	for (int i = 0; i < priv->n_pie_blocks; i++) {
		int pos = (i % 10) * 3;
		u32 r = RTL931X_PIE_BLK_PHASE_CTRL + 4 * (i / 10);

		if (i < priv->n_pie_blocks / 2)
			sw_w32_mask(0x7 << pos, 0, r);
		else
			sw_w32_mask(0x7 << pos, 1 << pos, r);
	}

	/* Enable predefined templates 0, 1 for first quarter of all blocks */
	template_selectors = 0 | (1 << 4);
	for (int i = 0; i < priv->n_pie_blocks / 4; i++)
		sw_w32(template_selectors, RTL931X_PIE_BLK_TMPLTE_CTRL(i));

	/* Enable predefined templates 2, 3 for second quarter of all blocks */
	template_selectors = 2 | (3 << 4);
	for (int i = priv->n_pie_blocks / 4; i < priv->n_pie_blocks / 2; i++)
		sw_w32(template_selectors, RTL931X_PIE_BLK_TMPLTE_CTRL(i));

	/* Enable predefined templates 0, 1 for third quater of all blocks */
	template_selectors = 0 | (1 << 4);
	for (int i = priv->n_pie_blocks / 2; i < priv->n_pie_blocks * 3 / 4; i++)
		sw_w32(template_selectors, RTL931X_PIE_BLK_TMPLTE_CTRL(i));

	/* Enable predefined templates 2, 3 for fourth quater of all blocks */
	template_selectors = 2 | (3 << 4);
	for (int i = priv->n_pie_blocks * 3 / 4; i < priv->n_pie_blocks; i++)
		sw_w32(template_selectors, RTL931X_PIE_BLK_TMPLTE_CTRL(i));
}

static void rtl931x_vlan_port_keep_tag_set(int port, bool keep_outer, bool keep_inner)
{
	sw_w32(FIELD_PREP(RTL931X_VLAN_PORT_TAG_EGR_OTAG_STS_MASK,
			  keep_outer ? RTL931X_VLAN_PORT_TAG_STS_TAGGED : RTL931X_VLAN_PORT_TAG_STS_UNTAG) |
	       FIELD_PREP(RTL931X_VLAN_PORT_TAG_EGR_ITAG_STS_MASK,
			  keep_inner ? RTL931X_VLAN_PORT_TAG_STS_TAGGED : RTL931X_VLAN_PORT_TAG_STS_UNTAG),
	       RTL931X_VLAN_PORT_TAG_CTRL(port));
}

static void rtl931x_vlan_qinq_setup(struct rtl838x_switch_priv *priv)
{
	int port, tpid;

	/* Mango provides four paired outer/inner TPID entries. Keep every
	 * pair deterministic: standard S-TAG outside, standard C-TAG inside.
	 */
	for (tpid = 0; tpid < 4; tpid++)
		sw_w32(RTL931X_VLAN_TPID_PAIR(RTL931X_VLAN_TPID_8021AD,
					      RTL931X_VLAN_TPID_8021Q),
		       RTL931X_VLAN_TAG_TPID_CTRL(tpid));

	/* Egress VLAN conversion must be enabled for the outer-tag policy to
	 * take effect.
	 */
	sw_w32_mask(0, RTL931X_PKT_ENCAP_MISC_CTRL_EVC_TCAM_EN,
		    RTL931X_PKT_ENCAP_MISC_CTRL);

	/* The normal port posture recognizes only C-TAGs. S-TAG recognition
	 * is enabled when a user port joins an 802.1ad bridge. The CPU port
	 * recognizes both so software-injected S-TAGs select the outer VID.
	 */
	for (port = 0; port <= priv->cpu_port; port++) {
		sw_w32(BIT(0), RTL931X_VLAN_PORT_ITAG_TPID_CMP_MSK(port));
		sw_w32(port == priv->cpu_port ? BIT(0) : 0,
		       RTL931X_VLAN_PORT_OTAG_TPID_CMP_MSK(port));
	}

	/* CPU-injected untagged/C-TAG frames retain the existing inner-VID
	 * forwarding behaviour; S-TAG and double-tagged frames use the SVID.
	 */
	sw_w32_mask(GENMASK(3, 0), BIT(3) | BIT(2),
		    RTL931X_VLAN_PORT_FWD + (priv->cpu_port << 2));
}

static void rtl931x_vlan_port_qinq_set(int port, bool enable)
{
	u32 tag_sts;

	/* All frame classes on a provider-bridge port forward on the outer
	 * VID. A normal 802.1Q port forwards every class on the inner VID.
	 */
	rtl931x_vlan_fwd_on_inner(port, !enable);

	/* Pair 0 is 0x88a8/0x8100. Normal ports deliberately do not parse an
	 * S-TAG, preserving the pre-QinQ 802.1Q posture.
	 */
	sw_w32(BIT(0), RTL931X_VLAN_PORT_ITAG_TPID_CMP_MSK(port));
	sw_w32(enable ? BIT(0) : 0,
	       RTL931X_VLAN_PORT_OTAG_TPID_CMP_MSK(port));

	/* Inserted outer tags take pair-0's 0x88a8 TPID rather than retaining
	 * an ingress TPID. The same zero value is valid after leaving QinQ.
	 */
	sw_w32_mask(RTL931X_VLAN_PORT_TAG_OTPID_IDX_MASK |
		    RTL931X_VLAN_PORT_TAG_OTPID_KEEP_MASK, 0,
		    RTL931X_VLAN_PORT_TAG_CTRL(port));

	if (!enable) {
		/* Existing 802.1Q posture: outer untagged, inner table-tagged. */
		rtl931x_vlan_port_keep_tag_set(port, false, true);
		return;
	}

	/* Provider-bridge posture: the VLAN table controls the outer S-TAG;
	 * preserve an ingress C-TAG transparently across the switch.
	 */
	tag_sts = FIELD_PREP(RTL931X_VLAN_PORT_TAG_EGR_OTAG_STS_MASK,
			     RTL931X_VLAN_PORT_TAG_STS_TAGGED) |
		  FIELD_PREP(RTL931X_VLAN_PORT_TAG_EGR_ITAG_STS_MASK,
			     RTL931X_VLAN_PORT_TAG_STS_INTERNAL) |
		  RTL931X_VLAN_PORT_TAG_EGR_ITAG_KEEP_MASK |
		  RTL931X_VLAN_PORT_TAG_IGR_ITAG_KEEP_MASK;
	sw_w32_mask(RTL931X_VLAN_PORT_TAG_EGR_OTAG_STS_MASK |
		    RTL931X_VLAN_PORT_TAG_EGR_ITAG_STS_MASK |
		    RTL931X_VLAN_PORT_TAG_EGR_OTAG_KEEP_MASK |
		    RTL931X_VLAN_PORT_TAG_EGR_ITAG_KEEP_MASK |
		    RTL931X_VLAN_PORT_TAG_IGR_OTAG_KEEP_MASK |
		    RTL931X_VLAN_PORT_TAG_IGR_ITAG_KEEP_MASK,
		    tag_sts, RTL931X_VLAN_PORT_TAG_CTRL(port));
}

static void rtl931x_vlan_port_pvidmode_set(int port, enum pbvlan_type type, enum pbvlan_mode mode)
{
	if (type == PBVLAN_TYPE_INNER)
		sw_w32_mask(0x3 << 12, mode << 12, RTL931X_VLAN_PORT_IGR_CTRL + (port << 2));
	else
		sw_w32_mask(0x3 << 26, mode << 26, RTL931X_VLAN_PORT_IGR_CTRL + (port << 2));
}

static void rtl931x_vlan_port_pvid_set(int port, enum pbvlan_type type, int pvid)
{
	if (type == PBVLAN_TYPE_INNER)
		sw_w32_mask(0xfff, pvid, RTL931X_VLAN_PORT_IGR_CTRL + (port << 2));
	else
		sw_w32_mask(0xfff << 14, pvid << 14, RTL931X_VLAN_PORT_IGR_CTRL + (port << 2));
}

static int rtldsa_931x_vlan_port_fast_age(struct rtl838x_switch_priv *priv, int port, u16 vid)
{
	u32 val;

	sw_w32(vid << 20, RTL931X_L2_TBL_FLUSH_CTRL + 4);

	val = 0;
	val |= port << 11;
	val |= BIT(24); /* compare port id */
	val |= BIT(26); /* compare VID */
	val |= BIT(28); /* status - trigger flush */
	sw_w32(val, RTL931X_L2_TBL_FLUSH_CTRL);

	do { } while (sw_r32(RTL931X_L2_TBL_FLUSH_CTRL) & BIT(28));

	return 0;
}

static void rtl931x_set_igr_filter(int port, enum igr_filter state)
{
	sw_w32_mask(0x3 << ((port & 0xf) << 1), state << ((port & 0xf) << 1),
		    RTL931X_VLAN_PORT_IGR_FLTR + (((port >> 4) << 2)));
}

static void rtl931x_set_egr_filter(int port,  enum egr_filter state)
{
	sw_w32_mask(0x1 << (port % 0x20), state << (port % 0x20),
		    RTL931X_VLAN_PORT_EGR_FLTR + (((port >> 5) << 2)));
}

/* Which of the two TRK_HASH_CTRL mask sets each trunk is bound to, by
 * hardware trunk id. Programmed by rtl931x_set_distribution_algorithm()
 * and written into the LAG table entry (L2/IP4/IP6_HASH_MSK_IDX fields)
 * by rtl931x_trunk_egr_ports_set(), mirroring the SDK's
 * dal_mango_trunk_distributionAlgorithmTypeBind_set().
 */
static u8 rtl931x_trk_hash_idx[MAX_LAGS];

static void rtl931x_set_distribution_algorithm(int group, int algoidx, u32 algomsk)
{
	u32 l2msk = 0, l3msk = 0;

	/* The L2 mask (bits 3-0) hashes bridged/non-IP frames, the L3 mask
	 * (bits 13-4) IP frames (RTL9310_TRK_HASH_CTRL L2_HASH_MSK/L3_HASH_MSK
	 * in swcore_rtl9310.h). Both must be populated: an empty L2 mask
	 * hashes every bridged frame identically and the trunk degenerates
	 * to a single member.
	 */
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_SMAC_BIT) {
		l2msk |= TRUNK_DISTRIBUTION_ALGO_L2_SMAC_BIT;
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_SMAC_BIT;
	}
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_DMAC_BIT) {
		l2msk |= TRUNK_DISTRIBUTION_ALGO_L2_DMAC_BIT;
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_DMAC_BIT;
	}
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_SIP_BIT)
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_SIP_BIT;
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_DIP_BIT)
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_DIP_BIT;
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_SRC_L4PORT_BIT)
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_SRC_L4PORT_BIT;
	if (algomsk & TRUNK_DISTRIBUTION_ALGO_DST_L4PORT_BIT)
		l3msk |= TRUNK_DISTRIBUTION_ALGO_L3_DST_L4PORT_BIT;

	/* Mango has only two hash mask sets (RTL9310_TRK_HASH_CTRL index
	 * 0-1), while the DSA layer asks for 0 (L2) / 1 (L23) / 2 (L34) -
	 * fold L34 onto set 1, there is no third set to hold it.
	 */
	algoidx = min(algoidx, 1);

	sw_w32(l2msk | (l3msk << 4), RTL931X_TRK_HASH_CTRL + (algoidx << 2));

	/* Neutral per-field hash shifts (RTL9310_TRK_SHFT_CTRL @0xBA7C). */
	sw_w32(0, RTL931X_TRK_SHFT_CTRL);

	/* Stand-alone trunk mode, local-first member preference and the
	 * non-terminate-mode tunnel hash source for plain bridged traffic
	 * (RTL9310_TRK_CTRL @0xBA78 bits 2/4/0; dal_mango_trunk_mode_set()
	 * forces LOCAL_FIRST on in stand-alone mode). Without bit 0 the TX
	 * hash never varies and every flow exits the first member.
	 */
	sw_w32(BIT(0) | BIT(2) | BIT(4), RTL931X_TRK_CTRL);

	/* The local trunk table generator matches LAG-table slots against
	 * this box's stacking device ID; our slots carry devID 0, so the
	 * box's own ID must be 0 as well or the generator finds no local
	 * members at all (RTL9310_STK_GBL_CTRL MY_DEV_ID @0x1448 bits 4-7).
	 */
	sw_w32_mask(0xf << 4, 0, RTL931X_STK_GBL_CTRL);

	if (group > 0 && group <= MAX_LAGS)
		rtl931x_trk_hash_idx[group - 1] = algoidx;
}

/* Map a source port to a trunk group (SRC_TRK_MAP table, one entry per
 * port): ingress traffic on the port is then attributed to the trunk for
 * L2 learning, source-port filtering and non-unicast forwarding.
 * SRC_TRK_MAP is table set 0 type 13 with TRK_ID_VALID @31 and a 7 bit
 * TRK_ID @24 (RTL9310_SRC_TRK_MAP_FIELDS in rtk_mango_tableField_list.c;
 * dal_mango_trunk_srcPortMap_set()).
 */
static void rtl931x_trunk_srcmap_set(int port, bool valid, int group)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_0, 13);
	u32 v = 0;

	if (WARN_ON(!r))
		return;

	group -= 1;	/* 1-based DSA LAG id -> hardware trunk */

	if (valid)
		v = BIT(31) | (group & 0x7f) << 24;

	sw_w32(v, rtl_table_data(r, 0));
	rtl_table_write(r, port);
	rtl_table_release(r);

	/* The local (same-unit) forwarding logic keeps its own per-port
	 * map with the same information: LOCAL_PORT_TRK_MAP @0x4CAC,
	 * IS_TRK_MBR @7, 7 bit TRK_ID @0 (RTL9310_LOCAL_PORT_TRK_MAP in
	 * swcore_rtl9310.h; _dal_mango_trunk_localPort_set()).
	 */
	sw_w32(valid ? BIT(7) | (group & 0x7f) : 0,
	       RTL931X_LOCAL_PORT_TRK_MAP + (port << 2));
}

/* Helper for the 96 bit LAG table entry: bit 0 is the LSB of the last
 * data word, bit 95 the MSB of the first (table word order per
 * RTL9310_LAG_FIELDS / table access data registers).
 */
static void rtl931x_lag_entry_set(u32 w[3], int lsp, int len, u32 val)
{
	for (int i = 0; i < len; i++)
		if (val & BIT(i))
			w[2 - ((lsp + i) >> 5)] |= BIT((lsp + i) & 0x1f);
}

/* Bit offset of each 6 bit TRK_PORTn field in the 96 bit LAG entry
 * (RTL9310_LAG_FIELDS in rtk_mango_tableField_list.c). The 10 bit
 * {TRK_DEVn, TRK_PORTn} slots do not straddle the 32 bit word
 * boundaries, so there are 2 bit reserved gaps at bits 30-31 and 62-63 -
 * slots 0-2 sit at 10n, slots 3-5 at 10n+2, slots 6-7 at 10n+4.
 */
static const u8 rtl931x_lag_trk_port_lsp[8] = {
	0, 10, 20, 32, 42, 52, 64, 74,
};

/* Program the egress candidate list of a trunk (LAG table, table set 2
 * type 0): NUM_TX_CANDI members, each a {devID, port} pair the TX hash
 * result indexes into. Without this the hash selects from an empty list
 * and unicast towards the trunk is not forwarded. Unused slots carry the
 * invalid port 0x3f (INVALID_TRUNK_MEMBER_PORT in the SDK), unused
 * devIDs stay 0 (_dal_mango_trunk_egrPort_set()).
 */
static void rtl931x_trunk_egr_ports_set(int group, u64 members)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 0);
	u32 w[3] = { 0 };
	u32 idx;
	int n = 0;

	if (WARN_ON(!r))
		return;

	/* group is a 1-based DSA LAG id and indexes rtl931x_trk_hash_idx[]
	 * once decremented. The silicon has 128 trunks, so raising
	 * num_lag_ids past MAX_LAGS is a plausible future change - catch it
	 * here rather than corrupting memory quietly.
	 */
	if (WARN_ON(group < 1 || group > MAX_LAGS)) {
		rtl_table_release(r);
		return;
	}

	group -= 1;	/* 1-based DSA LAG id -> hardware trunk */

	for (int p = 0; p < RTL931X_CPU_PORT && n < 8; p++) {
		if (members & BIT_ULL(p)) {
			/* TRK_PORTn at its slot offset, TRK_DEVn (0) above it */
			rtl931x_lag_entry_set(w, rtl931x_lag_trk_port_lsp[n], 6, p);
			n++;
		}
	}
	for (int s = n; s < 8; s++)
		rtl931x_lag_entry_set(w, rtl931x_lag_trk_port_lsp[s], 6, 0x3f);

	rtl931x_lag_entry_set(w, 89, 4, n);	/* NUM_TX_CANDI */

	/* Bind all traffic classes of this trunk to the hash mask set its
	 * distribution algorithm was programmed into: L2/IP4/IP6_HASH_MSK_IDX
	 * at bits 88/87/86 (RTL9310_LAG_FIELDS).
	 */
	idx = rtl931x_trk_hash_idx[group] & 1;
	rtl931x_lag_entry_set(w, 88, 1, idx);
	rtl931x_lag_entry_set(w, 87, 1, idx);
	rtl931x_lag_entry_set(w, 86, 1, idx);

	for (int i = 0; i < 3; i++)
		sw_w32(w[i], rtl_table_data(r, i));

	rtl_table_write(r, group);
	rtl_table_release(r);

	/* Allow SA learning for trunk-attributed traffic - the per-trunk
	 * learning constraint powers up at zero, which suppresses all
	 * learning on the trunk (same 0x7ffe limit the ports use).
	 * L2_LRN_TRK_CONSTRT_CTRL @0xCB34 is indexed by trunk gid with
	 * CONSTRT_NUM @3-18 and ACT @0-2 (0 = forward, per
	 * dal_mango_l2_limitAction_set()).
	 */
	sw_w32(0x7ffe << 3, RTL931X_L2_LRN_TRK_CONSTRT_CTRL + (group << 2));

	/* Bind the local trunk slot to the trunk ID: TRK_ID_CTRL @0xB800,
	 * TRK_VALID @7, 7 bit TRK_ID @0 (RTL9310_TRK_ID_CTRL in
	 * swcore_rtl9310.h). TRK_MBR_CTRL and the local table are indexed
	 * by local slot, not by trunk ID; without a valid binding the
	 * local-table refresh generates nothing and TX hashing degenerates
	 * to the first LAG candidate. We use the identity mapping
	 * slot == trunk id, so the SDK's 128-gid scan over its 52-entry
	 * localTrunkID array has no counterpart (and no overflow) here.
	 */
	sw_w32(members ? BIT(7) | (group & 0x7f) : 0,
	       RTL931X_TRK_ID_CTRL + (group << 2));

	/* Regenerate the internal local trunk table (TRK_LOCAL_TBL @0xBA84)
	 * - it is hardware-maintained: membership changes only take effect
	 * after poking the self-clearing TRK_LOCAL_TBL_REFRESH @0xBA80 bit 0
	 * (dal_mango_trunk_tbl_refresh()).
	 */
	sw_w32(BIT(0), RTL931X_TRK_LOCAL_TBL_REFRESH);
	if (readx_poll_timeout(sw_r32, RTL931X_TRK_LOCAL_TBL_REFRESH, idx,
			       !(idx & BIT(0)), 20, 10000))
		pr_err("%s: trunk %d local table refresh timed out\n",
		       __func__, group);
}

static void rtldsa_931x_led_get_forced(const struct device_node *node,
				       const u8 leds_in_set[4],
				       u8 forced_leds_per_port[RTL931X_CPU_PORT])
{
	DECLARE_BITMAP(mask, RTL931X_CPU_PORT);
	unsigned int port;
	char set_str[36];
	u64 pm;

	for (u8 set = 0; set < 4; set++) {
		snprintf(set_str, sizeof(set_str), "realtek,led-set%d-force-port-mask", set);
		if (of_property_read_u64(node, set_str, &pm))
			continue;

		bitmap_from_arr64(mask, &pm, RTL931X_CPU_PORT);

		for_each_set_bit(port, mask, RTL931X_CPU_PORT)
			forced_leds_per_port[port] = leds_in_set[set];
	}
}

static void rtldsa_931x_led_init(struct rtl838x_switch_priv *priv)
{
	u8 forced_leds_per_port[RTL931X_CPU_PORT] = {};
	u64 pm_copper = 0, pm_fiber = 0;
	struct device *dev = priv->dev;
	struct device_node *node;
	u8 leds_in_set[4] = {};

	node = of_find_compatible_node(NULL, NULL, "realtek,rtl9300-leds");
	if (!node) {
		dev_dbg(dev, "No compatible LED node found\n");
		return;
	}

	for (int set = 0; set < 4; set++) {
		char set_name[16] = {0};
		u32 set_config[4];
		int leds_in_this_set = 0;

		/* Reset LED set configuration */
		sw_w32(0, RTL931X_LED_SETX_0_CTRL(set));
		sw_w32(0, RTL931X_LED_SETX_1_CTRL(set));

		/* Each LED set has (up to) 4 LEDs, and each LED is configured
		 * with 16 bits. So each 32 bit register holds configuration for
		 * 2 LEDs. Therefore, each set requires 2 registers for
		 * configuring all 4 LEDs.
		 */
		snprintf(set_name, sizeof(set_name), "led_set%d", set);
		leds_in_this_set = of_property_count_u32_elems(node, set_name);

		if (leds_in_this_set <= 0 || leds_in_this_set > ARRAY_SIZE(set_config)) {
			if (leds_in_this_set != -EINVAL) {
				dev_err(dev, "%s invalid, skipping this set, leds_in_this_set=%d, should be (0, %d]\n",
					set_name, leds_in_this_set, ARRAY_SIZE(set_config));
			}

			continue;
		}

		dev_info(dev, "%s has %d LEDs configured\n", set_name, leds_in_this_set);
		leds_in_set[set] = leds_in_this_set;

		if (of_property_read_u32_array(node, set_name, set_config, leds_in_this_set))
			break;

		/* Write configuration for selected LEDs */
		for (int i = 0, led = leds_in_this_set - 1; led >= 0; led--, i++) {
			sw_w32_mask(0xffff << RTL931X_LED_SET_LEDX_SHIFT(led),
				    (0xffff & set_config[i]) << RTL931X_LED_SET_LEDX_SHIFT(led),
				    RTL931X_LED_SETX_LEDY(set, led));
		}
	}

	rtldsa_931x_led_get_forced(node, leds_in_set, forced_leds_per_port);

	for (int i = 0; i < priv->cpu_port; i++) {
		int pos = (i << 1) % 32;
		u32 set;

		sw_w32_mask(0x3 << pos, 0, RTL931X_LED_PORT_FIB_SET_SEL_CTRL(i));
		sw_w32_mask(0x3 << pos, 0, RTL931X_LED_PORT_COPR_SET_SEL_CTRL(i));

		/* Skip port if not present (auto-detect) or not in forced mask */
		if (!priv->ports[i].phy && !priv->pcs[i] && !(forced_leds_per_port[i]))
			continue;

		if (forced_leds_per_port[i] > 0)
			priv->ports[i].leds_on_this_port = forced_leds_per_port[i];

		/* 0x0 = 1 led, 0x1 = 2 leds, 0x2 = 3 leds, 0x3 = 4 leds per port */
		sw_w32_mask(0x3 << pos, (priv->ports[i].leds_on_this_port - 1) << pos,
			    RTL931X_LED_PORT_NUM_CTRL(i));

		if (priv->ports[i].phy_is_integrated)
			pm_fiber |= BIT_ULL(i);
		else
			pm_copper |= BIT_ULL(i);

		set = priv->ports[i].led_set;
		sw_w32_mask(0, set << pos, RTL931X_LED_PORT_COPR_SET_SEL_CTRL(i));
		sw_w32_mask(0, set << pos, RTL931X_LED_PORT_FIB_SET_SEL_CTRL(i));
	}

	/* Set LED mode to serial (0x1) */
	sw_w32_mask(0x3, 0x1, RTL931X_LED_GLB_CTRL);

	if (of_property_read_bool(node, "active-low"))
		sw_w32_mask(RTL931X_LED_GLB_ACTIVE_LOW, 0, RTL931X_LED_GLB_CTRL);
	else
		sw_w32_mask(0, RTL931X_LED_GLB_ACTIVE_LOW, RTL931X_LED_GLB_CTRL);

	rtl839x_set_port_reg_le(pm_copper, RTL931X_LED_PORT_COPR_MASK_CTRL);
	rtl839x_set_port_reg_le(pm_fiber, RTL931X_LED_PORT_FIB_MASK_CTRL);
	rtl839x_set_port_reg_le(pm_copper | pm_fiber, RTL931X_LED_PORT_COMBO_MASK_CTRL);

	for (int i = 0; i < 32; i++)
		dev_dbg(dev, "%08x: %08x\n", 0xbb000600 + i * 4, sw_r32(0x0600 + i * 4));
}

static u64 rtldsa_931x_stat_port_table_read(int port, unsigned int mib_size,
					    unsigned int mib_offset, bool is_pvt)
{
	struct table_reg *r;
	int field_offset;
	u64 ret = 0;

	if (is_pvt) {
		r = rtl_table_get(RTL9310_TBL_5, 1);
		field_offset = 27;
	} else {
		r = rtl_table_get(RTL9310_TBL_5, 0);
		field_offset = 52;
	}

	rtl_table_read(r, port);

	if (mib_size == 2) {
		ret = sw_r32(rtl_table_data(r, field_offset - (mib_offset + 1)));
		ret <<= 32;
	}

	ret |= sw_r32(rtl_table_data(r, field_offset - mib_offset));

	rtl_table_release(r);

	return ret;
}

/* Flow-control construction from _dal_mango_flowctrl_init_config(). Mango has
 * no model table: these are the named constants in dal_mango_construct.h.
 * Addresses and fields come from rtk_mango_{reg,regField}_list.c, whose
 * FC_PORT_EGR_DROP_CTRL/SWRED entries at 0xa800/0x27c4/0x27f4 already match
 * this driver's hardware-verified QoS register map.
 */
#define RTL931X_IGBW_Q_DROP_THR(q)		(0xe1e8 + ((q) << 2))
#define RTL931X_FC_PORT_ACT_CTRL(p)		(0x504c + ((p) << 2))
#define RTL931X_FC_GLB_SYS_UTIL_THR		0x5130
#define RTL931X_FC_GLB_DROP_THR			0x5134
#define RTL931X_FC_GLB_HI_THR			0x5138
#define RTL931X_FC_GLB_LO_THR			0x513c
#define RTL931X_FC_GLB_FCOFF_HI_THR		0x5140
#define RTL931X_FC_GLB_FCOFF_LO_THR		0x5144
#define RTL931X_FC_JUMBO_HI_THR			0x5148
#define RTL931X_FC_JUMBO_LO_THR			0x514c
#define RTL931X_FC_JUMBO_FCOFF_HI_THR		0x5150
#define RTL931X_FC_JUMBO_FCOFF_LO_THR		0x5154
#define RTL931X_FC_PORT_HI_THR			0x515c
#define RTL931X_FC_PORT_LO_THR			0x516c
#define RTL931X_FC_PORT_FCOFF_HI_THR		0x517c
#define RTL931X_FC_PORT_FCOFF_LO_THR		0x518c
#define RTL931X_FC_PORT_GUAR_THR			0x519c
#define RTL931X_FC_PORT_THR_SET_SEL		0x51ac
#define RTL931X_FC_Q_EGR_DROP_THR		0x2618
#define RTL931X_FC_PORT_EGR_DROP_THR_SET_SEL	0x2728
#define RTL931X_FC_REPCT_FCOFF_THR		0x8180
#define RTL931X_FC_HOL_PRVNT_CTRL		0xa8e4

#define RTL931X_FC_THR_ON_M			GENMASK(28, 16)
#define RTL931X_FC_THR_OFF_M			GENMASK(12, 0)
#define RTL931X_FC_THR_M			GENMASK(12, 0)
#define RTL931X_FC_ALLOW_PAGE_CNT_M		GENMASK(12, 0)

#define RTL931X_IGBW_QUEUE_COUNT		3
#define RTL931X_FC_QUEUE_COUNT			8
#define RTL931X_FC_EGR_DROP_THR_SET_COUNT	3
#define RTL931X_FC_JUMBO_PORT_THR_SET		3
#define RTL931X_FC_JUMBO_EGR_DROP_THR_SET	2

/* dal_mango_construct.h constants used by
 * _dal_mango_flowctrl_init_config().
 */
#define RTL931X_IGBW_Q_DROP_THR_HIGH		220
#define RTL931X_IGBW_Q_DROP_THR_LOW		210
#define RTL931X_FC_JUMBO_Q_DROP_THR_ON		300
#define RTL931X_FC_JUMBO_Q_DROP_THR_OFF		220
#define RTL931X_FC_JUMBO_SYS_HI_ON		3572
#define RTL931X_FC_JUMBO_SYS_HI_OFF		3397
#define RTL931X_FC_JUMBO_SYS_LO_ON		2510
#define RTL931X_FC_JUMBO_SYS_LO_OFF		2335
#define RTL931X_FC_JUMBO_FCOFF_SYS_HI_ON	2000
#define RTL931X_FC_JUMBO_FCOFF_SYS_HI_OFF	1825
#define RTL931X_FC_JUMBO_FCOFF_SYS_LO_ON	1000
#define RTL931X_FC_JUMBO_FCOFF_SYS_LO_OFF	825
#define RTL931X_FC_JUMBO_PORT_HI_ON		200
#define RTL931X_FC_JUMBO_PORT_HI_OFF		130
#define RTL931X_FC_JUMBO_PORT_LO_ON		110
#define RTL931X_FC_JUMBO_PORT_LO_OFF		35
#define RTL931X_FC_JUMBO_FCOFF_PORT_HI_ON	200
#define RTL931X_FC_JUMBO_FCOFF_PORT_HI_OFF	130
#define RTL931X_FC_JUMBO_FCOFF_PORT_LO_ON	105
#define RTL931X_FC_JUMBO_FCOFF_PORT_LO_OFF	35
#define RTL931X_FC_JUMBO_PORT_GUAR		0
#define RTL931X_FC_PORT_LO_ON			25
#define RTL931X_FC_REPCT_DROP_ON		78
#define RTL931X_FC_REPCT_DROP_OFF		68
#define RTL931X_FC_ALLOW_PAGE_CNT		40

static void rtl931x_fc_field_write(u32 reg, u32 mask, u32 value)
{
	sw_w32_mask(mask, (value << __ffs(mask)) & mask, reg);
}

static u32 rtl931x_fc_field_read(u32 reg, u32 mask)
{
	return (sw_r32(reg) & mask) >> __ffs(mask);
}

static u32 rtl931x_fc_queue_drop_thr(int queue, int set)
{
	return RTL931X_FC_Q_EGR_DROP_THR +
	       ((queue * RTL931X_FC_EGR_DROP_THR_SET_COUNT + set) << 2);
}

static u32 rtl931x_fc_get_selector(u32 base, int port)
{
	u32 shift = (port & 0xf) << 1;
	u32 reg = base + ((port >> 4) << 2);

	return (sw_r32(reg) >> shift) & 0x3;
}

static void rtl931x_flow_control_init(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;
	u32 reg;

	mutex_lock(&priv->reg_mutex);

	/* Set ingress queue drop thresholds. */
	for (int queue = 0; queue < RTL931X_IGBW_QUEUE_COUNT; queue++) {
		reg = RTL931X_IGBW_Q_DROP_THR(queue);
		rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
				       RTL931X_IGBW_Q_DROP_THR_HIGH);
		rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
				       RTL931X_IGBW_Q_DROP_THR_LOW);
	}

	/* The vendor has the ordinary FC_GLB_HI_THR.OFF write under #if 0.
	 * Deliberately preserve that omission rather than inventing a value.
	 */

	/* Set jumbo-mode egress queue drop thresholds, table index 2. */
	for (int queue = 0; queue < RTL931X_FC_QUEUE_COUNT; queue++) {
		reg = rtl931x_fc_queue_drop_thr(queue,
						RTL931X_FC_JUMBO_EGR_DROP_THR_SET);
		rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
				       RTL931X_FC_JUMBO_Q_DROP_THR_ON);
		rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
				       RTL931X_FC_JUMBO_Q_DROP_THR_OFF);
	}

	/* Set jumbo-mode system thresholds. */
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_HI_THR, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_SYS_HI_ON);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_HI_THR, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_SYS_HI_OFF);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_LO_THR, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_SYS_LO_ON);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_LO_THR, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_SYS_LO_OFF);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_FCOFF_HI_THR,
			       RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_FCOFF_SYS_HI_ON);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_FCOFF_HI_THR,
			       RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_FCOFF_SYS_HI_OFF);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_FCOFF_LO_THR,
			       RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_FCOFF_SYS_LO_ON);
	rtl931x_fc_field_write(RTL931X_FC_JUMBO_FCOFF_LO_THR,
			       RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_FCOFF_SYS_LO_OFF);

	/* Set jumbo-mode port thresholds, table index 3. */
	reg = RTL931X_FC_PORT_HI_THR + (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_PORT_HI_ON);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_PORT_HI_OFF);
	reg = RTL931X_FC_PORT_LO_THR + (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_PORT_LO_ON);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_PORT_LO_OFF);
	reg = RTL931X_FC_PORT_GUAR_THR + (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_M,
			       RTL931X_FC_JUMBO_PORT_GUAR);
	reg = RTL931X_FC_PORT_FCOFF_HI_THR +
	      (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_FCOFF_PORT_HI_ON);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_FCOFF_PORT_HI_OFF);
	reg = RTL931X_FC_PORT_FCOFF_LO_THR +
	      (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_ON_M,
			       RTL931X_FC_JUMBO_FCOFF_PORT_LO_ON);
	rtl931x_fc_field_write(reg, RTL931X_FC_THR_OFF_M,
			       RTL931X_FC_JUMBO_FCOFF_PORT_LO_OFF);

	/* Adjust the low-on threshold in ordinary port sets 0 through 2. */
	for (int set = 0; set < RTL931X_FC_JUMBO_PORT_THR_SET; set++) {
		rtl931x_fc_field_write(RTL931X_FC_PORT_LO_THR + (set << 2),
				       RTL931X_FC_THR_ON_M,
				       RTL931X_FC_PORT_LO_ON);
		rtl931x_fc_field_write(RTL931X_FC_PORT_FCOFF_LO_THR + (set << 2),
				       RTL931X_FC_THR_ON_M,
				       RTL931X_FC_PORT_LO_ON);
	}

	/* Set replication queue drop thresholds. */
	rtl931x_fc_field_write(RTL931X_FC_REPCT_FCOFF_THR,
			       RTL931X_FC_THR_ON_M, RTL931X_FC_REPCT_DROP_ON);
	rtl931x_fc_field_write(RTL931X_FC_REPCT_FCOFF_THR,
			       RTL931X_FC_THR_OFF_M, RTL931X_FC_REPCT_DROP_OFF);

	/* HWP_PORT_TRAVS_EXCEPT_CPU skips FE ports in the SDK. RTL931x ports
	 * represented by this DSA driver are GE or faster, so every DSA user
	 * port receives the vendor's allowance while absent ports stay untouched.
	 */
	dsa_switch_for_each_user_port(dp, priv->ds) {
		if (rtl83xx_port_max_speed(priv, dp->index) <= SPEED_100)
			continue;

		rtl931x_fc_field_write(RTL931X_FC_PORT_ACT_CTRL(dp->index),
				       RTL931X_FC_ALLOW_PAGE_CNT_M,
				       RTL931X_FC_ALLOW_PAGE_CNT);
	}

	/* RTK_DEFAULT_FC_HOL_PKT_{BC,L2_MC,IP_MC,UNKN_UC}_STATUS are all
	 * DISABLED. Preserve unrelated upper bits in FC_HOL_PRVNT_CTRL.
	 */
	sw_w32_mask(GENMASK(3, 0), 0, RTL931X_FC_HOL_PRVNT_CTRL);

	/* Mango's SWRED construction is a separate vendor function and is not
	 * ported: W8.7 proved those controls inert on both families.
	 */
	mutex_unlock(&priv->reg_mutex);
}

static void rtl931x_fc_dump_pair(struct seq_file *m, const char *name, u32 reg)
{
	seq_printf(m, "%s on %u off %u raw %08x\n", name,
		   rtl931x_fc_field_read(reg, RTL931X_FC_THR_ON_M),
		   rtl931x_fc_field_read(reg, RTL931X_FC_THR_OFF_M), sw_r32(reg));
}

static void rtl931x_flow_control_dump(struct rtl838x_switch_priv *priv,
				      int port, struct seq_file *m)
{
	u32 reg;

	mutex_lock(&priv->reg_mutex);
	seq_puts(m, "family mango\n");
	seq_printf(m, "glb_sys_util %u raw %08x source reset\n",
		   rtl931x_fc_field_read(RTL931X_FC_GLB_SYS_UTIL_THR,
					 RTL931X_FC_THR_M),
		   sw_r32(RTL931X_FC_GLB_SYS_UTIL_THR));
	seq_printf(m, "glb_drop %u raw %08x source reset\n",
		   rtl931x_fc_field_read(RTL931X_FC_GLB_DROP_THR,
					 RTL931X_FC_THR_M),
		   sw_r32(RTL931X_FC_GLB_DROP_THR));
	rtl931x_fc_dump_pair(m, "glb_hi_vendor_write_disabled",
			     RTL931X_FC_GLB_HI_THR);
	rtl931x_fc_dump_pair(m, "glb_lo_source_reset", RTL931X_FC_GLB_LO_THR);
	rtl931x_fc_dump_pair(m, "glb_fcoff_hi_source_reset",
			     RTL931X_FC_GLB_FCOFF_HI_THR);
	rtl931x_fc_dump_pair(m, "glb_fcoff_lo_source_reset",
			     RTL931X_FC_GLB_FCOFF_LO_THR);
	rtl931x_fc_dump_pair(m, "jumbo_hi", RTL931X_FC_JUMBO_HI_THR);
	rtl931x_fc_dump_pair(m, "jumbo_lo", RTL931X_FC_JUMBO_LO_THR);
	rtl931x_fc_dump_pair(m, "jumbo_fcoff_hi", RTL931X_FC_JUMBO_FCOFF_HI_THR);
	rtl931x_fc_dump_pair(m, "jumbo_fcoff_lo", RTL931X_FC_JUMBO_FCOFF_LO_THR);
	reg = RTL931X_FC_PORT_HI_THR + (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_dump_pair(m, "jumbo_port_hi", reg);
	reg = RTL931X_FC_PORT_LO_THR + (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_dump_pair(m, "jumbo_port_lo", reg);
	reg = RTL931X_FC_PORT_FCOFF_HI_THR +
	      (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_dump_pair(m, "jumbo_port_fcoff_hi", reg);
	reg = RTL931X_FC_PORT_FCOFF_LO_THR +
	      (RTL931X_FC_JUMBO_PORT_THR_SET << 2);
	rtl931x_fc_dump_pair(m, "jumbo_port_fcoff_lo", reg);
	rtl931x_fc_dump_pair(m, "replication_drop", RTL931X_FC_REPCT_FCOFF_THR);
	seq_printf(m, "hol_prevent %x\n",
		   (u32)(sw_r32(RTL931X_FC_HOL_PRVNT_CTRL) & GENMASK(3, 0)));
	seq_printf(m, "port %d thr_set %u egr_drop_thr_set %u allow_page_count %u\n",
		   port, rtl931x_fc_get_selector(RTL931X_FC_PORT_THR_SET_SEL, port),
		   rtl931x_fc_get_selector(RTL931X_FC_PORT_EGR_DROP_THR_SET_SEL,
					   port),
		   rtl931x_fc_field_read(RTL931X_FC_PORT_ACT_CTRL(port),
					 RTL931X_FC_ALLOW_PAGE_CNT_M));
	mutex_unlock(&priv->reg_mutex);
}

/* Remaining Mango construction rows. Register addresses and fields are from
 * rtk_mango_{reg,regField}_list.c.
 */
#define RTL931X_VENDOR_PARSER_CTRL		0x056c
#define RTL931X_VENDOR_VXLAN_GPE_UDP_PORT_M	GENMASK(31, 16)
#define RTL931X_VENDOR_MALFORMED_PKT_ACT_M	GENMASK(6, 5)
#define RTL931X_VENDOR_PARSER_CANT_HANDLE_ACT_M	GENMASK(4, 3)
#define RTL931X_VENDOR_WL_OFFSET_EN		BIT(2)
#define RTL931X_VENDOR_PPPOE_PARSE_EN		BIT(1)
#define RTL931X_VENDOR_RFC1042_OUI_IGNORE	BIT(0)

#define RTL931X_VENDOR_METER_BYTE_TB_CTRL	0x4178
#define RTL931X_VENDOR_METER_PKT_TB_CTRL		0x417c
#define RTL931X_VENDOR_IGBW_LB_CTRL		0xe004
#define RTL931X_VENDOR_TICK_M			GENMASK(31, 16)
#define RTL931X_VENDOR_TOKEN_M			GENMASK(15, 0)

#define RTL931X_VENDOR_IGBW_PORT_SCHED_BASE	0xe914
#define RTL931X_VENDOR_IGBW_PORT_SCHED(p, q) \
	(RTL931X_VENDOR_IGBW_PORT_SCHED_BASE + \
	 ((((p) * RTL931X_IGBW_QUEUE_COUNT) + (q)) << 2))
#define RTL931X_VENDOR_IGBW_SP			BIT(8)
#define RTL931X_VENDOR_IGBW_WEIGHT_M		GENMASK(4, 0)

struct rtl931x_vendor_rate_defaults {
	u16 mhz;
	u16 byte_tick;
	u16 byte_token;
	u16 packet_tick;
	u16 packet_token;
	u16 ingress_tick;
	u16 ingress_token;
};

/* Indexed by MAC_L2_GLOBAL_CTRL2.SYS_CLK_SEL. The SDK treats every value
 * other than the explicit 325 MHz and 175 MHz selectors as 650 MHz.
 */
static const struct rtl931x_vendor_rate_defaults rtl931x_vendor_rates[] = {
	{
		.mhz = 650,
		.byte_tick = 53,
		.byte_token = 171,
		.packet_tick = 620,
		.packet_token = 1,
		.ingress_tick = 53,
		.ingress_token = 171,
	},
	{
		.mhz = 325,
		.byte_tick = 53,
		.byte_token = 342,
		.packet_tick = 310,
		.packet_token = 1,
		.ingress_tick = 53,
		.ingress_token = 342,
	},
	{
		.mhz = 175,
		.byte_tick = 123,
		.byte_token = 1474,
		.packet_tick = 167,
		.packet_token = 1,
		.ingress_tick = 123,
		.ingress_token = 1474,
	},
};

static const struct rtl931x_vendor_rate_defaults *
rtl931x_vendor_rate_defaults_get(u32 sys_clk)
{
	if (sys_clk == 1 || sys_clk == 2)
		return &rtl931x_vendor_rates[sys_clk];

	return &rtl931x_vendor_rates[0];
}

static void rtl931x_vendor_init(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	mutex_lock(&priv->reg_mutex);

	/* Preserve every other parser field, including the live VXLAN port and
	 * word-length offset setting.
	 */
	sw_w32_mask(RTL931X_VENDOR_PPPOE_PARSE_EN,
		    RTL931X_VENDOR_PPPOE_PARSE_EN, RTL931X_VENDOR_PARSER_CTRL);

	/* dsa_switch_for_each_user_port() is the driver's equivalent of the
	 * vendor HWP_PORT_TRAVS_EXCEPT_CPU: absent MACs and CPU port 56 remain
	 * untouched. SP is independent of the reset WEIGHT field.
	 */
	dsa_switch_for_each_user_port(dp, priv->ds) {
		for (int queue = 0; queue < RTL931X_IGBW_QUEUE_COUNT; queue++)
			sw_w32_mask(RTL931X_VENDOR_IGBW_SP,
				    RTL931X_VENDOR_IGBW_SP,
				    RTL931X_VENDOR_IGBW_PORT_SCHED(dp->index,
								    queue));
	}

	/* METER_BYTE_TB_CTRL, METER_PKT_TB_CTRL, and IGBW_LB_CTRL already match
	 * the clock-selected vendor values on live Mango hardware, so do not
	 * rewrite them; debugfs derives their expected values from SYS_CLK_SEL.
	 * The initialized meter state rules out an uninitialized meter as the
	 * reason SWRED is inert.
	 */

	mutex_unlock(&priv->reg_mutex);
}

static void rtl931x_vendor_rate_dump(struct seq_file *m, const char *name,
				     u32 reg, u16 vendor_tick,
				     u16 vendor_token)
{
	u32 raw = sw_r32(reg);

	seq_printf(m, "%s addr 0x%08x raw 0x%08x tick %u token %u "
		   "vendor_tick %u vendor_token %u status already_vendor\n",
		   name, reg, raw, (u32)FIELD_GET(RTL931X_VENDOR_TICK_M, raw),
		   (u32)FIELD_GET(RTL931X_VENDOR_TOKEN_M, raw), vendor_tick,
		   vendor_token);
}

static void rtl931x_vendor_sched_dump(struct seq_file *m, int port, int queue,
				      const char *status)
{
	u32 reg = RTL931X_VENDOR_IGBW_PORT_SCHED(port, queue);
	u32 raw = sw_r32(reg);

	seq_printf(m, "igbw_port_sched port %d queue %d addr 0x%08x "
		   "raw 0x%08x sp %u status %s weight %u "
		   "weight_status reset_deliberate\n",
		   port, queue, reg, raw, !!(raw & RTL931X_VENDOR_IGBW_SP),
		   status, (u32)FIELD_GET(RTL931X_VENDOR_IGBW_WEIGHT_M, raw));
}

static void rtl931x_vendor_init_dump(struct rtl838x_switch_priv *priv,
				     struct seq_file *m)
{
	const struct rtl931x_vendor_rate_defaults *rates;
	struct dsa_port *dp;
	u32 clock_raw;
	u32 sys_clk;
	u32 raw;

	mutex_lock(&priv->reg_mutex);
	seq_puts(m, "family mango\n");

	clock_raw = sw_r32(RTL931X_MAC_L2_GLOBAL_CTRL2);
	sys_clk = (u32)FIELD_GET(RTL931X_SYS_CLK_SEL_M, clock_raw);
	rates = rtl931x_vendor_rate_defaults_get(sys_clk);
	seq_printf(m, "system_clock addr 0x%08x raw 0x%08x sys_clk_sel %u "
		   "mhz %u\n",
		   RTL931X_MAC_L2_GLOBAL_CTRL2, clock_raw, sys_clk, rates->mhz);

	raw = sw_r32(RTL931X_VENDOR_PARSER_CTRL);
	seq_printf(m, "parser_ctrl addr 0x%08x raw 0x%08x pppoe_parse_en %u "
		   "vxlan_gpe_udp_port %u malformed_pkt_act %u "
		   "parser_cant_handle_act %u wl_offset_en %u "
		   "rfc1042_oui_ignore %u status programmed\n",
		   RTL931X_VENDOR_PARSER_CTRL, raw,
		   !!(raw & RTL931X_VENDOR_PPPOE_PARSE_EN),
		   (u32)FIELD_GET(RTL931X_VENDOR_VXLAN_GPE_UDP_PORT_M, raw),
		   (u32)FIELD_GET(RTL931X_VENDOR_MALFORMED_PKT_ACT_M, raw),
		   (u32)FIELD_GET(RTL931X_VENDOR_PARSER_CANT_HANDLE_ACT_M, raw),
		   !!(raw & RTL931X_VENDOR_WL_OFFSET_EN),
		   !!(raw & RTL931X_VENDOR_RFC1042_OUI_IGNORE));

	rtl931x_vendor_rate_dump(m, "meter_byte_tb",
				 RTL931X_VENDOR_METER_BYTE_TB_CTRL,
				 rates->byte_tick, rates->byte_token);
	rtl931x_vendor_rate_dump(m, "meter_packet_tb",
				 RTL931X_VENDOR_METER_PKT_TB_CTRL,
				 rates->packet_tick, rates->packet_token);
	rtl931x_vendor_rate_dump(m, "igbw_lb", RTL931X_VENDOR_IGBW_LB_CTRL,
				 rates->ingress_tick, rates->ingress_token);
	raw = sw_r32(RTL931X_FC_REPCT_FCOFF_THR);
	seq_printf(m, "repeater_flow_control addr 0x%08x raw 0x%08x "
		   "drop_on %u drop_off %u status programmed\n",
		   RTL931X_FC_REPCT_FCOFF_THR, raw,
		   (u32)FIELD_GET(RTL931X_FC_THR_ON_M, raw),
		   (u32)FIELD_GET(RTL931X_FC_THR_OFF_M, raw));

	dsa_switch_for_each_user_port(dp, priv->ds) {
		for (int queue = 0; queue < RTL931X_IGBW_QUEUE_COUNT; queue++)
			rtl931x_vendor_sched_dump(m, dp->index, queue,
						  "programmed");
	}
	for (int queue = 0; queue < RTL931X_IGBW_QUEUE_COUNT; queue++)
		rtl931x_vendor_sched_dump(m, priv->cpu_port, queue,
					  "reset_deliberate");

	mutex_unlock(&priv->reg_mutex);
}

static void rtldsa_931x_qos_set_group_selector(int port, int group)
{
	sw_w32_mask(RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port),
		    group << RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port),
		    RTL931X_PORT_TBL_IDX_CTRL(port));
}

void rtldsa_931x_qos_setup_default_dscp2queue_map(void)
{
	u32 queue;

	/* The default mapping between dscp and queue is based on
	 * the first 3 bits indicate the precedence (prio = dscp >> 3).
	 */
	for (int i = 0; i < DSCP_MAP_MAX; i++) {
		queue = (i >> 3) << RTL93XX_REMAP_DSCP_INTPRI_DSCP_OFFSET(i);
		sw_w32_mask(RTL93XX_REMAP_DSCP_INTPRI_DSCP_MASK(i),
			    queue, RTL931X_REMAP_DSCP(i));
	}
}

static void rtldsa_931x_qos_prio2queue_matrix(int *min_queues)
{
	u32 v = 0;

	for (int i = 0; i < MAX_PRIOS; i++)
		v |= i << (min_queues[i] * 3);

	sw_w32(v, RTL931X_QM_INTPRI2QID_CTRL);
}

/* Default (port-based) internal priority of a port: 3 bits per port in
 * PRI_SEL_REMAP_PORT (SDK dal_mango_qos_priRemap_set, PRI_SRC_PB_PRI).
 */
int rtl931x_qos_default_prio_get(int port)
{
	return (sw_r32(RTL931X_PRI_SEL_PORT_PRI(port)) >> ((port % 10) * 3)) & 0x7;
}

int rtl931x_qos_default_prio_set(struct rtl838x_switch_priv *priv, int port,
				 u8 prio)
{
	if (prio >= MAX_PRIOS)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	sw_w32_mask(0x7 << ((port % 10) * 3), prio << ((port % 10) * 3),
		    RTL931X_PRI_SEL_PORT_PRI(port));
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

/* DSCP to internal priority remapping: 3 bits per DSCP value in
 * PRI_SEL_REMAP_DSCP (SDK dal_mango_qos_priRemap_set, PRI_SRC_DSCP). The
 * table is global to the switch, the caller-facing DSA op is per port.
 */
int rtl931x_qos_dscp_prio_get(int dscp)
{
	return (sw_r32(RTL931X_REMAP_DSCP(dscp)) >>
		RTL93XX_REMAP_DSCP_INTPRI_DSCP_OFFSET(dscp)) & 0x7;
}

int rtl931x_qos_dscp_prio_set(struct rtl838x_switch_priv *priv, int dscp,
			      u8 prio)
{
	if (prio >= MAX_PRIOS)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	sw_w32_mask(RTL93XX_REMAP_DSCP_INTPRI_DSCP_MASK(dscp),
		    prio << RTL93XX_REMAP_DSCP_INTPRI_DSCP_OFFSET(dscp),
		    RTL931X_REMAP_DSCP(dscp));
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

/* Per-queue scheduling control: WEIGHT (1-127, shared by WFQ and WRR)
 * and STRICT_EN, marking the queue strict-priority instead of weighted
 * (SDK dal_mango_qos_schedulingQueue_set and
 * dal_mango_qos_portQueueStrictEnable_set). STRICT_EN is bit 8 on this
 * family (bit 7 on RTL930x). Ports 0-51 are served by register SET0,
 * ports 52-55 by SET1.
 */
void rtl931x_qos_queue_sched_set(int port, int queue, u8 weight, bool strict)
{
	u32 v = weight & RTL931X_SCHED_Q_WEIGHT_M;

	if (strict)
		v |= RTL931X_SCHED_Q_STRICT_EN;

	if (port < 52)
		sw_w32(v, RTL931X_SCHED_PORT_Q_CTRL_SET0(port, queue));
	else
		sw_w32(v, RTL931X_SCHED_PORT_Q_CTRL_SET1(port, queue));
}

/* Weighted scheduling algorithm of a port: 0 = WFQ (byte-count),
 * 1 = WRR (packet-count), one bit per port in SCHED_PORT_ALGO_CTRL, 32
 * ports per word (SDK dal_mango_qos_schedulingAlgorithm_set).
 * Strict-priority queues always win over weighted queues regardless of
 * this selection.
 */
int rtl931x_qos_sched_algo_get(int port)
{
	return !!(sw_r32(RTL931X_SCHED_PORT_ALGO_CTRL(port)) & BIT(port % 32));
}

void rtl931x_qos_sched_algo_set(int port, bool wrr)
{
	sw_w32_mask(BIT(port % 32), wrr ? BIT(port % 32) : 0,
		    RTL931X_SCHED_PORT_ALGO_CTRL(port));
}

/* Boot-time default scheduling of a port: all queues weighted with
 * weight 1, weighted algorithm WFQ.
 */
void rtl931x_qos_port_sched_defaults(int port)
{
	for (int q = 0; q < 8; q++)
		rtl931x_qos_queue_sched_set(port, q, 1, false);

	rtl931x_qos_sched_algo_set(port, false);
}

void rtl931x_qos_sched_defaults(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	dsa_switch_for_each_user_port(dp, priv->ds)
		rtl931x_qos_port_sched_defaults(dp->index);
}

/* Set a field of up to 32 bits at entry bit position lsp in a table entry
 * held as words[0] = entry bits 31:0. Fields may straddle a word boundary
 * (the 20-bit rate fields do for odd queues).
 */
static void rtl931x_qos_entry_field_set(u32 *words, int lsp, int len, u32 val)
{
	u32 mask = BIT(len) - 1;
	int w = lsp >> 5;
	int off = lsp & 31;

	words[w] &= ~(mask << off);
	words[w] |= (val & mask) << off;
	if (off + len > 32) {
		int rem = off + len - 32;

		words[w + 1] &= ~(BIT(rem) - 1);
		words[w + 1] |= (val & mask) >> (len - rem);
	}
}

/* Program the maximum egress bandwidth leaky bucket of a queue. The
 * per-queue buckets live in the EGR_Q_BW table, one 29-word entry per
 * port (SDK dal_mango_rate_portEgrQueueBwCtrl{Enable,Rate,BurstSize}_set);
 * the table data window is big-endian, so data register i holds entry
 * bits [32 * (28 - i) + 31 : 32 * (28 - i)]. A rate of 0 disables the
 * bucket and restores the reset posture: rate wide open, SDK default
 * burst. The burst cap gates egress even with the enable bit clear, so
 * disabling by writing zeros would block the queue entirely. The SDK
 * requires the burst to hold at least 8 tokens of the global leaky-bucket
 * tick/token register EGBW_LB_CTRL; the same is enforced here.
 */
int rtl931x_qos_queue_shaper_set(struct rtl838x_switch_priv *priv, int port,
				 int queue, u64 rate_bytes_ps, u32 burst)
{
	u32 entry[RTL931X_EGR_Q_BW_WORDS];
	struct table_reg *r;
	u32 rate = 0, tkn, min_burst;
	int ret;

	if (rate_bytes_ps) {
		tkn = sw_r32(RTL931X_EGBW_LB_CTRL) & RTL931X_EGBW_LB_TKN_M;
		min_burst = 8 * tkn;
		ret = rtl83xx_qos_shaper_validate(priv, port, queue,
						 rate_bytes_ps, min_burst,
						 RTL931X_EGBW_Q_BURST_M, &rate,
						 &burst);
		if (ret)
			return ret;
	}

	r = rtl_table_get(RTL9310_TBL_4, 0);
	rtl_table_read(r, port);
	for (int w = 0; w < RTL931X_EGR_Q_BW_WORDS; w++)
		entry[w] = sw_r32(rtl_table_data(r, RTL931X_EGR_Q_BW_WORDS - 1 - w));

	if (rate_bytes_ps) {
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_BW_LSP(queue),
					    RTL931X_EGR_Q_BW_MAX_BW_LEN, rate);
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_LB_BURST_LSP(queue),
					    RTL931X_EGR_Q_BW_MAX_LB_BURST_LEN, burst);
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_BW_EN_LSP(queue),
					    1, 1);
	} else {
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_BW_LSP(queue),
					    RTL931X_EGR_Q_BW_MAX_BW_LEN,
					    RTL931X_EGBW_Q_RATE_M);
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_LB_BURST_LSP(queue),
					    RTL931X_EGR_Q_BW_MAX_LB_BURST_LEN,
					    RTL931X_EGBW_LB_RESET_BURST);
		rtl931x_qos_entry_field_set(entry,
					    RTL931X_EGR_Q_BW_MAX_BW_EN_LSP(queue),
					    1, 0);
	}

	for (int w = 0; w < RTL931X_EGR_Q_BW_WORDS; w++)
		sw_w32(entry[w], rtl_table_data(r, RTL931X_EGR_Q_BW_WORDS - 1 - w));
	rtl_table_write(r, port);
	rtl_table_release(r);

	return 0;
}

/* Program the port-level maximum egress bandwidth leaky bucket. A rate of
 * 0 disables the bucket and restores the reset posture (rate wide open,
 * SDK default burst), as the burst cap gates egress even with the enable
 * bit clear. The SDK never writes the port rate directly: it keeps a
 * shadow and refills the hardware rate with the number of fitting
 * share-mode assured queue rates on top ([SS-972] in dal_mango_rate.c).
 * Assured queue bandwidth is never programmed by this driver, so the
 * refill term is always zero and writing the rate directly is equivalent.
 */
int rtl931x_qos_port_shaper_set(struct rtl838x_switch_priv *priv, int port,
				u64 rate_bytes_ps, u32 burst)
{
	u32 addr = RTL931X_EGBW_PORT_CTRL(port);
	u32 rate = 0, tkn, min_burst;
	int ret;

	if (rate_bytes_ps) {
		tkn = sw_r32(RTL931X_EGBW_LB_CTRL) & RTL931X_EGBW_LB_TKN_M;
		min_burst = 8 * tkn;
		ret = rtl83xx_qos_shaper_validate(priv, port, -1,
						 rate_bytes_ps, min_burst,
						 RTL931X_EGBW_Q_BURST_M, &rate,
						 &burst);
		if (ret)
			return ret;
	}

	/* 64-bit entry, high word first: the low address carries RATE
	 * (bits 32-51) and EN (bit 52), the high address carries BURST.
	 * Disabling restores the reset posture (rate wide open, reset
	 * burst) rather than zeros, because the burst cap gates egress
	 * even with the enable bit clear.
	 */
	mutex_lock(&priv->reg_mutex);
	if (rate_bytes_ps) {
		sw_w32(RTL931X_EGBW_Q_EN | rate, addr);
		sw_w32(burst & RTL931X_EGBW_Q_BURST_M, addr + 4);
	} else {
		sw_w32(RTL931X_EGBW_Q_RATE_M, addr);
		sw_w32(RTL931X_EGBW_LB_RESET_BURST, addr + 4);
	}
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

/* Program the SWRED thresholds and drop probability of a queue (all
 * queues if queue is negative) and switch the port from tail drop to
 * SWRED. The threshold and drop-rate tables are global to the switch and
 * shared by all SWRED-enabled ports, so the queues that are not being
 * configured are explicitly set to a never-drop configuration (maximum
 * thresholds, drop rate 0) instead of being left at unknown reset or
 * stale values. Cross-port use with differing parameters is
 * last-writer-wins per queue; users should keep RED parameters consistent
 * across ports. The same values are written to all three drop
 * precedences: nothing in the driver assigns drop precedences today, so
 * all traffic is DP 0, and tc-red has no drop-precedence concept either.
 */
int rtl931x_qos_swred_set(struct rtl838x_switch_priv *priv, int port, int queue,
			  u32 min_pages, u32 max_pages, u8 probability)
{
	u32 v = FIELD_PREP(RTL931X_SWRED_THR_MAX_M, max_pages) |
		FIELD_PREP(RTL931X_SWRED_THR_MIN_M, min_pages);
	u32 never = FIELD_PREP(RTL931X_SWRED_THR_MAX_M, RTL931X_SWRED_THR_MAX_PAGES) |
		    FIELD_PREP(RTL931X_SWRED_THR_MIN_M, RTL931X_SWRED_THR_MAX_PAGES);
	u32 rate = probability | (probability << 8) | (probability << 16);

	if (min_pages > max_pages || max_pages > RTL931X_SWRED_THR_MAX_PAGES)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	for (int q = 0; q < 8; q++) {
		u32 thr = (queue >= 0 && q != queue) ? never : v;

		sw_w32((queue >= 0 && q != queue) ? 0 : rate,
		       RTL931X_SWRED_Q_DROP_RATE(q));
		for (int dp = 0; dp < RTL931X_SWRED_DROP_PRECEDENCES; dp++)
			sw_w32(thr, RTL931X_SWRED_Q_THR(q, dp));
	}
	sw_w32_mask(RTL931X_FC_EGR_DROP_ALGO_SWRED, RTL931X_FC_EGR_DROP_ALGO_SWRED,
		    RTL931X_FC_PORT_EGR_DROP_CTRL(port));
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

void rtl931x_qos_swred_disable(struct rtl838x_switch_priv *priv, int port)
{
	mutex_lock(&priv->reg_mutex);
	sw_w32_mask(RTL931X_FC_EGR_DROP_ALGO_SWRED, 0,
		    RTL931X_FC_PORT_EGR_DROP_CTRL(port));
	mutex_unlock(&priv->reg_mutex);
}

void rtl931x_qos_swred_get(struct rtl838x_switch_priv *priv, int port,
			   struct rtl838x_qos_swred_state *state)
{
	mutex_lock(&priv->reg_mutex);
	state->enabled = !!(sw_r32(RTL931X_FC_PORT_EGR_DROP_CTRL(port)) &
				   RTL931X_FC_EGR_DROP_ALGO_SWRED);
	for (int queue = 0; queue < MAX_PRIOS; queue++) {
		u32 rate = sw_r32(RTL931X_SWRED_Q_DROP_RATE(queue));

		for (int dp = 0; dp < RTL838X_SWRED_DROP_PRECEDENCES; dp++) {
			u32 v = sw_r32(RTL931X_SWRED_Q_THR(queue, dp));

			state->min_pages[queue][dp] = FIELD_GET(RTL931X_SWRED_THR_MIN_M, v);
			state->max_pages[queue][dp] = FIELD_GET(RTL931X_SWRED_THR_MAX_M, v);
			state->probability[queue][dp] = (rate >> (dp * 8)) & 0xff;
		}
	}
	mutex_unlock(&priv->reg_mutex);
}

static void rtldsa_931x_qos_set_scheduling_queue_weights(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;

	dsa_switch_for_each_user_port(dp, priv->ds)
		for (int q = 0; q < MAX_PRIOS; q++)
			rtl931x_qos_queue_sched_set(dp->index, q,
						    rtldsa_default_queue_weights[q],
						    false);
}

static void rtldsa_931x_qos_init(struct rtl838x_switch_priv *priv)
{
	struct dsa_port *dp;
	u32 v;

	/* Assign all the ports to the Group-0 */
	dsa_switch_for_each_user_port(dp, priv->ds)
		rtldsa_931x_qos_set_group_selector(dp->index, 0);

	rtldsa_931x_qos_prio2queue_matrix(rtldsa_max_available_queue);

	/* configure priority weights */
	v = 0;
	v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_PORT_MASK, 3);
	v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_DSCP_MASK, 5);
	v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_ITAG_MASK, 6);
	v |= FIELD_PREP(RTL93XX_PRI_SEL_TBL_CTRL_OTAG_MASK, 7);

	sw_w32(v, RTL931X_PRI_SEL_TBL_CTRL(0) + 4);
	sw_w32(0, RTL931X_PRI_SEL_TBL_CTRL(0));

	/* The DSCP defaults are programmed from rtl93xx_setup(): the DSA
	 * core seeds the dcbnl app table from hardware when the user ports
	 * are created, before this init runs.
	 */
	rtldsa_931x_qos_set_scheduling_queue_weights(priv);
}

#ifdef CONFIG_NET_DSA_RTL83XX_RTL930X_L3_OFFLOAD

/* RTL931x (Mango) Layer-3 offload.
 *
 * All table layouts and register values derive from the Mango SDK:
 * table geometry from rtk_mango_table_list.c, field offsets from
 * rtk_mango_tableField_list.c, register field positions from
 * swcore_rtl9310.h, init sequence from dal_mango_l3.c.
 *
 * All L3 tables are accessed via table access set 2 (RTL9310_TBL_2):
 *   type 2: L3_ROUTER_MAC           1024 entries x 5 words (ternary)
 *   type 3: L3_HOST_ROUTE_IPUC      12288 entries x 4 words
 *   type 4: L3_PREFIX_ROUTE_IPUC    12288 entries x 6 words (ternary)
 *   type 6: L3_NEXTHOP              8192 entries x 1 word
 *   type 7: L3_IGR_INTF             1024 entries x 2 words
 *   type 8: L3_EGR_INTF             1024 entries x 4 words
 * The host and prefix tables share one physical table with per-format
 * views (types 3-5); the SDK addresses entries logically (0..12287) and
 * the hardware row holds 8 slots of which 6 are usable, so the logical
 * index converts to a physical address with ((idx / 6) * 8) + (idx % 6)
 * (SDK DAL_MANGO_L3_ENTRY_IDX_TO_ADDR).
 * Table data words are big-endian: data word 0 (the lowest address) holds
 * the highest-numbered entry bits; the SDK documents bit positions within
 * the entry.
 */

/* Reads a router-MAC entry (an L3 termination endpoint: packets whose
 * DMAC matches are considered for routing) from the L3_ROUTER_MAC table.
 */
static void rtl931x_get_l3_router_mac(u32 idx, struct rtl93xx_rt_mac *m)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 2);
	u32 v, w;

	/* L3_ROUTER_MAC entry, 160 bits (MANGO_L3_ROUTER_MACt fields):
	 * word 0: VALID 31, PORT_TYPE 30, PORT_ID 29:23, INTF_ID 22:13,
	 *         MAC[47:35] 12:0
	 * word 1: MAC[34:3] 31:0
	 * word 2: MAC[2:0] 31:29, LU_PHASE 28, L3_INTF 27,
	 *         BMSK_PORT_TYPE 22, BMSK_PORT_ID 21:15,
	 *         BMSK_INTF_ID 14:5, BMSK_MAC[47:43] 4:0
	 * word 3: BMSK_MAC[42:11] 31:0
	 * word 4: BMSK_MAC[10:0] 31:21, BMSK_LU_PHASE 20, BMSK_L3_INTF 19,
	 *         ACT 14:12
	 */
	rtl_table_read(r, idx);
	v = sw_r32(rtl_table_data(r, 0));
	w = sw_r32(rtl_table_data(r, 2));
	m->valid = !!(v & BIT(31));
	m->p_type = !!(v & BIT(30));
	m->p_id = (v >> 23) & 0x7f;
	/* The Mango entry has no VID field: it qualifies the matched MAC
	 * by the ingress L3 interface ID, which takes the shared struct's
	 * vid slot (BMSK_INTF_ID takes vid_mask).
	 */
	m->vid = (v >> 13) & 0x3ff;
	m->vid_mask = (w >> 5) & 0x3ff;
	m->p_id_mask = (w >> 15) & 0x7f;
	m->mac = (((u64)v & 0x1fff) << 35) |
		 (((u64)sw_r32(rtl_table_data(r, 1))) << 3) |
		 ((w >> 29) & 0x7);
	m->mac_mask = (((u64)w & 0x1f) << 43) |
		      (((u64)sw_r32(rtl_table_data(r, 3))) << 11) |
		      ((sw_r32(rtl_table_data(r, 4)) >> 21) & 0x7ff);
	m->action = (sw_r32(rtl_table_data(r, 4)) >> 12) & 0x7;
	rtl_table_release(r);
}

/* Writes a router-MAC entry into the L3_ROUTER_MAC table. The shared L3
 * code programs an exact-MAC, any-VLAN, any-port entry with action
 * FORWARD; on Mango "any VLAN" is expressed as BMSK_INTF_ID = 0 (the
 * interface mask does not care), matching the RTL930x vid_mask = 0
 * posture.
 */
static void rtl931x_set_l3_router_mac(u32 idx, struct rtl93xx_rt_mac *m)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 2);
	u32 v;

	v = m->valid ? BIT(31) : 0;
	v |= m->p_type ? BIT(30) : 0;
	v |= (m->p_id & 0x7f) << 23;
	v |= (m->vid & 0x3ff) << 13;		/* INTF_ID, see get */
	v |= (u32)(m->mac >> 35) & 0x1fff;
	sw_w32(v, rtl_table_data(r, 0));

	sw_w32((u32)(m->mac >> 3), rtl_table_data(r, 1));

	v = ((u32)m->mac & 0x7) << 29;		/* LU_PHASE/L3_INTF stay 0 */
	v |= (m->p_id_mask & 0x7f) << 15;	/* BMSK_PORT_TYPE stays 0 */
	v |= (m->vid_mask & 0x3ff) << 5;	/* BMSK_INTF_ID */
	v |= (u32)(m->mac_mask >> 43) & 0x1f;
	sw_w32(v, rtl_table_data(r, 2));

	sw_w32((u32)(m->mac_mask >> 11), rtl_table_data(r, 3));

	v = ((u32)m->mac_mask & 0x7ff) << 21;
	v |= (m->action & 0x7) << 12;
	sw_w32(v, rtl_table_data(r, 4));

	rtl_table_write(r, idx);
	rtl_table_release(r);
}

/* Sets up an L3 interface. Mango splits the RTL930x single egress
 * interface table into an ingress table (VRF, per-family route enables,
 * uRPF) and an egress table (egress VID, inline 48-bit SMAC, MTU
 * indices) sharing one interface index, and selects the ingress
 * interface per packet from the VLAN entry's L3_INTF_ID field
 * (SDK _dal_mango_l3_vlanIntf_insert). The source MAC is not part of
 * struct rtl838x_l3_intf: the shared allocator patches it in afterwards
 * through rtl931x_set_l3_egress_mac().
 */
static void rtl931x_set_l3_egress_intf(int idx, struct rtl838x_l3_intf *intf)
{
	struct rtl838x_vlan_info info;
	struct table_reg *r;
	u32 v;

	/* L3_IGR_INTF entry, 64 bits (MANGO_L3_IGR_INTFt fields):
	 * word 0: VRF_ID 31:24, IPUC_ROUTE_EN 23, IP6UC_ROUTE_EN 22,
	 *         IPMC_ROUTE_EN 21, IP6MC_ROUTE_EN 20, MC lookup-miss and
	 *         scope actions below
	 * word 1: uRPF controls and MC key select
	 * IPv4 and IPv6 unicast routing are enabled; multicast and uRPF
	 * stay off.
	 */
	r = rtl_table_get(RTL9310_TBL_2, 7);
	sw_w32(BIT(23) | BIT(22), rtl_table_data(r, 0));
	sw_w32(0, rtl_table_data(r, 1));
	rtl_table_write(r, idx);
	rtl_table_release(r);

	/* L3_EGR_INTF entry, 128 bits (MANGO_L3_EGR_INTFt fields):
	 * word 0: DST_VID 31:20, SMAC_ADDR[47:28] 19:0
	 * word 1: SMAC_ADDR[27:0] 31:4, IP_MTU_IDX 3:0
	 * word 1 (high): IP6_MTU_IDX 31:28, IPMC_TTL_SCOPE 27:20,
	 *         IP6MC_HL_SCOPE 19:12, IP_ICMP_REDIRECT_ACT 11:9,
	 *         IP6_ICMP_REDIRECT_ACT 8:6, IP_PBR_ICMP_REDIRECT_ACT 5:3,
	 *         IP6_PBR_ICMP_REDIRECT_ACT 2:0
	 * words 2/3: tunnel interface fields, unused
	 * The redirect actions take the shared struct's values verbatim:
	 * 2 = FORWARD on both families (SDK _actEgrIntfIpIcmpRedirect),
	 * so one-armed hairpin traffic (ingress == egress interface, the
	 * ICMP-redirect case) is forwarded in hardware. The TTL/HL scope
	 * fields are multicast scopes on Mango; the unicast TTL check is
	 * driven by the route entry's TTL_DEC/TTL_CHK and the global
	 * TTL_FAIL_ACT instead.
	 */
	v = (intf->ip6_mtu_id & 0xf) << 28;
	v |= (intf->ttl_scope & 0xff) << 20;
	v |= (intf->hl_scope & 0xff) << 12;
	v |= (intf->ip4_icmp_redirect & 0x7) << 9;
	v |= (intf->ip6_icmp_redirect & 0x7) << 6;
	v |= (intf->ip4_pbr_icmp_redirect & 0x7) << 3;
	v |= (intf->ip6_pbr_icmp_redirect & 0x7);
	v |= (intf->ip4_mtu_id & 0xf);

	r = rtl_table_get(RTL9310_TBL_2, 8);
	sw_w32((intf->vid & 0xfff) << 20, rtl_table_data(r, 0));
	sw_w32(v, rtl_table_data(r, 1));
	sw_w32(0, rtl_table_data(r, 2));
	sw_w32(0, rtl_table_data(r, 3));
	rtl_table_write(r, idx);
	rtl_table_release(r);

	/* Bind the VLAN to this interface (SDK _dal_mango_vlan_l3IntfIdx_set):
	 * the VLAN entry's L3_INTF_ID selects the ingress L3 interface for
	 * packets in that VLAN. Read-modify-write keeps membership, FID,
	 * profile and group mask intact.
	 */
	rtl931x_vlan_tables_read(intf->vid, &info);
	info.if_id = idx;
	rtl931x_vlan_set_tagged(intf->vid, &info);
}

/* Get the source MAC of an L3 egress interface. Mango has no SMAC-index
 * table like the RTL930x L3_EGR_INTF_MAC: the SMAC is inline in the
 * L3_EGR_INTF entry, so shared SMAC slot i maps to egress interface i.
 * Indices below L3_EGRESS_DMACS name destination-MAC slots, which do not
 * exist on Mango: a nexthop's DMAC is resolved through its L2 FDB entry
 * (NEXTHOP.DMAC_IDX) instead.
 */
static u64 rtl931x_get_l3_egress_mac(u32 idx)
{
	struct table_reg *r;
	u64 mac;

	if (idx < L3_EGRESS_DMACS)
		return 0;

	r = rtl_table_get(RTL9310_TBL_2, 8);
	rtl_table_read(r, idx - L3_EGRESS_DMACS);
	mac = (((u64)sw_r32(rtl_table_data(r, 0)) & 0xfffff) << 28) |
	      ((u64)sw_r32(rtl_table_data(r, 1)) >> 4);
	rtl_table_release(r);

	return mac;
}

/* Set the source MAC of an L3 egress interface; see
 * rtl931x_get_l3_egress_mac() for the table model. Read-modify-write:
 * the word-1 low nibble holds IP_MTU_IDX, word-0 the DST_VID.
 */
static void rtl931x_set_l3_egress_mac(u32 idx, u64 mac)
{
	struct table_reg *r;
	u32 v;

	if (idx < L3_EGRESS_DMACS)
		return;

	r = rtl_table_get(RTL9310_TBL_2, 8);
	rtl_table_read(r, idx - L3_EGRESS_DMACS);
	v = sw_r32(rtl_table_data(r, 0)) & 0xfff00000;
	v |= (u32)(mac >> 28) & 0xfffff;
	sw_w32(v, rtl_table_data(r, 0));
	v = sw_r32(rtl_table_data(r, 1)) & 0xf;
	v |= (u32)mac << 4;
	sw_w32(v, rtl_table_data(r, 1));
	rtl_table_write(r, idx - L3_EGRESS_DMACS);
	rtl_table_release(r);
}

/* The SDK addresses L3 entries logically; each hardware row has six
 * usable addresses followed by two holes.
 */
static inline int rtl931x_l3_idx_to_addr(int idx)
{
	return ((idx / 6) * 8) + (idx % 6);
}

static inline int rtl931x_l3_addr_to_idx(int addr)
{
	return ((addr / 8) * 6) + (addr % 8);
}

static u32 rtl931x_ip6_word(const struct in6_addr *ip6, int offset)
{
	const u8 *d = ip6->s6_addr + offset;

	return ((u32)d[0] << 24) | ((u32)d[1] << 16) |
	       ((u32)d[2] << 8) | d[3];
}

static void rtl931x_ip6_word_set(struct in6_addr *ip6, int offset, u32 v)
{
	u8 *d = ip6->s6_addr + offset;

	d[0] = v >> 24;
	d[1] = v >> 16;
	d[2] = v >> 8;
	d[3] = v;
}

static u64 rtl931x_ip6_chunk48(const struct in6_addr *ip6, int offset)
{
	const u8 *d = ip6->s6_addr + offset;
	u64 v = 0;

	for (int i = 0; i < 6; i++)
		v = (v << 8) | d[i];

	return v;
}

static void rtl931x_ip6_chunk48_set(struct in6_addr *ip6, int offset, u64 v)
{
	u8 *d = ip6->s6_addr + offset;

	for (int i = 5; i >= 0; i--) {
		d[i] = v;
		v >>= 8;
	}
}

/* IPv4 host-route hash, translated from the Mango SDK
 * (_dal_mango_l3_hostHash0_ret/_hostHash1_ret). The 10-bit hash folds
 * the VRF ID into the key; for a unicast host the SDK hashes
 * {vrf, sip = 0, dip, vid = 0}, so only the VRF and DIP rows
 * contribute. Algorithm 0 XORs the VRF row with the four DIP groups;
 * algorithm 1 sums the four DIP groups with end-around carry into
 * 10 bits and XORs the VRF row in.
 */
static u32 rtl931x_l3_hash4(u32 vrf, u32 ip, int algorithm)
{
	u32 h;

	if (!algorithm) {
		h = ((vrf & 0x1f) << 5) | ((vrf >> 5) & 0x7);
		h ^= (ip >> 30) & 0x3;
		h ^= (ip >> 20) & 0x3ff;
		h ^= (ip >> 10) & 0x3ff;
		h ^= ip & 0x3ff;
	} else {
		h = (ip >> 30) & 0x3;
		for (int g = 20; ; g -= 10) {
			h += (ip >> g) & 0x3ff;
			h = (h & 0x3ff) + (h >> 10);
			if (!g)
				break;
		}
		h ^= ((vrf & 0xf) << 6) | ((vrf >> 4) & 0xf);
	}

	return h;
}

/* IPv6 host-route hash with VRF, SIP and VID fixed to zero, matching
 * the unicast key posture used by rtl931x_l3_hash4().
 */
static u32 rtl931x_l3_hash6(const struct in6_addr *ip6, int algorithm)
{
	const u8 *d = ip6->s6_addr;
	u32 h;

	if (!algorithm) {
		h = d[0];
		h ^= ((u32)d[1] << 2) | (d[2] >> 6);
		h ^= ((u32)(d[2] & 0x3f) << 4) | (d[3] >> 4);
		h ^= ((u32)(d[3] & 0x0f) << 6) | (d[4] >> 2);
		h ^= ((u32)(d[4] & 0x03) << 8) | d[5];
		h ^= ((u32)d[6] << 2) | (d[7] >> 6);
		h ^= ((u32)(d[7] & 0x3f) << 4) | (d[8] >> 4);
		h ^= ((u32)(d[8] & 0x0f) << 6) | (d[9] >> 2);
		h ^= ((u32)(d[9] & 0x03) << 8) | d[10];
		h ^= ((u32)d[11] << 2) | (d[12] >> 6);
		h ^= ((u32)(d[12] & 0x3f) << 4) | (d[13] >> 4);
		h ^= ((u32)(d[13] & 0x0f) << 6) | (d[14] >> 2);
		h ^= ((u32)(d[14] & 0x03) << 8) | d[15];
		return h;
	}

	h = d[12] >> 6;
	h += ((u32)(d[12] & 0x3f) << 4) | (d[13] >> 4);
	h = (h & 0x3ff) + (h >> 10);
	h += ((u32)(d[13] & 0x0f) << 6) | (d[14] >> 2);
	h = (h & 0x3ff) + (h >> 10);
	h += ((u32)(d[14] & 0x03) << 8) | d[15];
	h = (h & 0x3ff) + (h >> 10);

	h ^= ((u32)d[0] << 2) | (d[1] >> 6);
	h ^= ((u32)(d[1] & 0x3f) << 4) | (d[2] >> 4);
	h ^= ((u32)(d[2] & 0x0f) << 6) | (d[3] >> 2);
	h ^= ((u32)(d[3] & 0x03) << 8) | d[4];
	h ^= ((u32)d[5] << 2) | (d[6] >> 6);
	h ^= ((u32)(d[6] & 0x3f) << 4) | (d[7] >> 4);
	h ^= ((u32)(d[7] & 0x0f) << 6) | (d[8] >> 2);
	h ^= ((u32)(d[8] & 0x03) << 8) | d[9];
	h ^= ((u32)d[10] << 2) | (d[11] >> 6);
	h ^= (u32)(d[11] & 0x3f) << 4;

	return h;
}

/* Read a host route entry from the L3 host table using its logical
 * index. IPv6 unicast entries occupy three consecutive logical slots.
 */
static void rtl931x_host_route_read(int idx, struct rtl83xx_route *rt)
{
	u32 data[4], v, w;
	u64 chunk;
	/* Access the host table (L3_HOST_ROUTE_IPUC view) via access set 2 */
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 3);

	rtl_table_read(r, rtl931x_l3_idx_to_addr(idx));
	/* L3_HOST_ROUTE_IPUC entry, 128 bits (MANGO_L3_HOST_ROUTE_IPUCt):
	 * word 0: VALID 31, FMT 30, ENTRY_TYPE 29:28, VRF_ID 27:20,
	 *         IP[31:12] 19:0
	 * word 1: IP[11:0] 31:20, DST_NULL_INTF 10, ACT 9:7, ECMP_EN 6,
	 *         NH_ECMP_IDX[12:7] 5:0
	 * word 2: NH_ECMP_IDX[6:0] 31:25, TTL_DEC 24, TTL_CHK 23,
	 *         QOS_EN 22, QOS_PRI 21:19
	 * word 3: HIT 15
	 */
	for (int i = 0; i < 4; i++)
		data[i] = sw_r32(rtl_table_data(r, i));
	v = data[0];
	rt->attr.valid = !!(v & BIT(31));
	if (!rt->attr.valid)
		goto out;
	rt->attr.type = (v >> 28) & 0x3;
	w = data[1];
	switch (rt->attr.type) {
	case 0: /* IPv4 unicast */
		rt->dst_ip = ((v & 0xfffff) << 12) | (w >> 20);
		break;
	case 2: /* IPv6 unicast */
		/* Other type-2 slots are continuations, not IPv6 entry bases. */
		if (idx % 6 != 0 && idx % 6 != 3) {
			pr_warn_ratelimited("%s: IPv6 route at unaligned slot %d is not decodable\n",
					    __func__, idx);
			goto out;
		}
		rtl931x_ip6_word_set(&rt->dst_ip6, 0,
				      ((v & 0xfffff) << 12) | (w >> 20));
		for (int k = 1; k < 3; k++) {
			rtl_table_read(r, rtl931x_l3_idx_to_addr(idx + k));
			v = sw_r32(rtl_table_data(r, 0));
			w = sw_r32(rtl_table_data(r, 1));
			chunk = ((u64)(v & 0x0fffffff) << 20) | (w >> 12);
			rtl931x_ip6_chunk48_set(&rt->dst_ip6, 4 + (k - 1) * 6,
						     chunk);
		}
		break;
	case 1: /* IPv4 multicast */
	case 3: /* IPv6 multicast */
		pr_warn("%s: route type %d not supported\n", __func__, rt->attr.type);
		goto out;
	}

	w = data[1];
	rt->attr.dst_null = !!(w & BIT(10));
	rt->attr.action = (w >> 7) & 0x7;
	rt->nh.id = ((w & 0x3f) << 7) | (data[2] >> 25);
	v = data[2];
	rt->attr.ttl_dec = !!(v & BIT(24));
	rt->attr.ttl_check = !!(v & BIT(23));
	rt->attr.qos_as = !!(v & BIT(22));
	rt->attr.qos_prio = (v >> 19) & 0x7;
	rt->attr.hit = !!(data[3] & BIT(15));

out:
	rtl_table_release(r);
}

/* Read the activity bit of a host route and clear it. Hardware only ever
 * sets it, so clearing is a read-modify-write through the table data
 * registers - that preserves the entry fields this driver does not decode.
 */
static bool rtl931x_host_route_hit_get_clear(int idx)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 3);
	bool hit = false;
	u32 v;

	if (!r)
		return false;

	if (rtl_table_read(r, rtl931x_l3_idx_to_addr(idx)))
		goto out;

	v = sw_r32(rtl_table_data(r, 0));
	if (!(v & BIT(31)))	/* entry went away under us */
		goto out;

	v = sw_r32(rtl_table_data(r, 3));
	hit = !!(v & BIT(15));
	if (hit) {
		sw_w32(v & ~BIT(15), rtl_table_data(r, 3));
		rtl_table_write(r, rtl931x_l3_idx_to_addr(idx));
	}

out:
	rtl_table_release(r);

	return hit;
}

/* Write a host route entry using its logical index. Invalidation clears
 * every slot in the entry so no valid continuation can survive.
 */
static void rtl931x_host_route_write(int idx, struct rtl83xx_route *rt)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 3);
	u64 chunk;
	u32 ip;
	int width;

	if (rt->attr.type == 1 || rt->attr.type == 3) {
		pr_warn("%s: multicast route type %d not supported\n",
			__func__, rt->attr.type);
		rtl_table_release(r);
		return;
	}
	if (rt->attr.valid && rt->attr.type != 0 && rt->attr.type != 2) {
		pr_warn("%s: route type %d not supported\n", __func__, rt->attr.type);
		rtl_table_release(r);
		return;
	}

	width = rt->attr.type == 2 ? 3 : 1;

	if (!rt->attr.valid) {
		for (int k = 0; k < width; k++) {
			for (int i = 0; i < 4; i++)
				sw_w32(0, rtl_table_data(r, i));
			rtl_table_write(r, rtl931x_l3_idx_to_addr(idx + k));
		}
		goto out;
	}

	if (rt->attr.type == 0) {
		sw_w32(BIT(31) | ((rt->dst_ip >> 12) & 0xfffff),
		       rtl_table_data(r, 0));
		sw_w32(((rt->dst_ip & 0xfff) << 20) |
		       (rt->attr.dst_null ? BIT(10) : 0) |
		       ((rt->attr.action & 0x7) << 7) |
		       ((rt->nh.id >> 7) & 0x3f),
		       rtl_table_data(r, 1));
		sw_w32(((rt->nh.id & 0x7f) << 25) |
		       (rt->attr.ttl_dec ? BIT(24) : 0) |
		       (rt->attr.ttl_check ? BIT(23) : 0) |
		       (rt->attr.qos_as ? BIT(22) : 0) |
		       ((rt->attr.qos_prio & 0x7) << 19),
		       rtl_table_data(r, 2));
		sw_w32(rt->attr.hit ? BIT(15) : 0, rtl_table_data(r, 3));
		rtl_table_write(r, rtl931x_l3_idx_to_addr(idx));
		goto out;
	}

	ip = rtl931x_ip6_word(&rt->dst_ip6, 0);
	sw_w32(BIT(31) | (0x2 << 28) | ((ip >> 12) & 0xfffff),
	       rtl_table_data(r, 0));
	sw_w32(((ip & 0xfff) << 20) |
	       (rt->attr.dst_null ? BIT(10) : 0) |
	       ((rt->attr.action & 0x7) << 7) |
	       ((rt->nh.id >> 7) & 0x3f),
	       rtl_table_data(r, 1));
	sw_w32(((rt->nh.id & 0x7f) << 25) |
	       (rt->attr.ttl_dec ? BIT(24) : 0) |
	       (rt->attr.ttl_check ? BIT(23) : 0) |
	       (rt->attr.qos_as ? BIT(22) : 0) |
	       ((rt->attr.qos_prio & 0x7) << 19),
	       rtl_table_data(r, 2));
	sw_w32(rt->attr.hit ? BIT(15) : 0, rtl_table_data(r, 3));
	rtl_table_write(r, rtl931x_l3_idx_to_addr(idx));

	for (int k = 1; k < 3; k++) {
		chunk = rtl931x_ip6_chunk48(&rt->dst_ip6, 4 + (k - 1) * 6);
		sw_w32(BIT(31) | (0x2 << 28) |
		       ((u32)(chunk >> 20) & 0x0fffffff), rtl_table_data(r, 0));
		sw_w32((u32)(chunk & 0xfffff) << 12, rtl_table_data(r, 1));
		sw_w32(0, rtl_table_data(r, 2));
		sw_w32(0, rtl_table_data(r, 3));
		rtl_table_write(r, rtl931x_l3_idx_to_addr(idx + k));
	}

out:
	rtl_table_release(r);
}

static bool rtl931x_host_route_valid(int idx)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 3);
	bool valid;

	rtl_table_read(r, rtl931x_l3_idx_to_addr(idx));
	valid = !!(sw_r32(rtl_table_data(r, 0)) & BIT(31));
	rtl_table_release(r);

	return valid;
}

/* Find the logical host-table slot for a unicast route. With must_exist
 * the slot already holding this destination is located (for updates and
 * invalidation); otherwise the first free slot in the two hash rows is
 * returned. The scanned slot's own VALID bit decides, never the
 * candidate route's.
 */
static int rtl931x_find_l3_slot(struct rtl83xx_route *rt, bool must_exist)
{
	struct rtl83xx_route route_entry;
	int slot_width, algorithm, addr, idx;
	u32 hash;

	if (rt->attr.type != 0 && rt->attr.type != 2)
		return -1;
	slot_width = rt->attr.type == 2 ? 3 : 1;

	for (int t = 0; t < 2; t++) {
		algorithm = (sw_r32(RTL931X_L3_HOST_TBL_CTRL) >> (2 + t)) & 0x1;
		hash = rt->attr.type == 2 ?
		       rtl931x_l3_hash6(&rt->dst_ip6, algorithm) :
		       rtl931x_l3_hash4(0, rt->dst_ip, algorithm);

		for (int s = 0; s < 6; s += slot_width) {
			addr = (t << 13) | ((hash & 0x3ff) << 3) | s;
			idx = rtl931x_l3_addr_to_idx(addr);

			if (!must_exist) {
				bool free = true;

				for (int k = 0; k < slot_width; k++) {
					if (rtl931x_host_route_valid(idx + k)) {
						free = false;
						break;
					}
				}
				if (free)
					return idx;
				continue;
			}

			memset(&route_entry, 0, sizeof(route_entry));
			rtl931x_host_route_read(idx, &route_entry);
			if (route_entry.attr.valid &&
			    route_entry.attr.type == rt->attr.type &&
			    (rt->attr.type == 2 ?
			     ipv6_addr_equal(&route_entry.dst_ip6, &rt->dst_ip6) :
			     route_entry.dst_ip == rt->dst_ip))
				return idx;
		}
	}

	return -1;
}

/* Unicast prefix routes (L3_PREFIX_ROUTE_IPUC view, access set 2 type 4,
 * ternary, 6 words per entry). The prefix TCAM returns the LOWEST
 * matching address, so longest-prefix-first ordering is a software
 * discipline: entries are kept sorted by descending prefix length and
 * the hardware move engine shuffles the boundaries on every insert or
 * delete. The algorithm and the bookkeeping are translated from the
 * Mango SDK (__dal_mango_l3_routeEntry_alloc/_free, dal_mango_l3.c),
 * not from Longan: Longan moves whole blocks per engine command and
 * keeps different histogram arrays, Mango chains single-entry moves
 * because a multi-entry move would straddle the 2 dead addresses of
 * each 8-address TCAM block.
 *
 * Region layout (SDK dal_mango_l3.c, L3_ROUTE_TBL_* macros):
 * - IPv4 grows UP from index 0, longest prefix at the lowest index,
 *   the default route at the highest used v4 index.
 * - IPv6 grows DOWN from the top in 3-entry strides, longest prefix at
 *   the lowest index. The v6 histogram counts in the OPPOSITE direction
 *   to the v4 one:
 *   ip_pflen[i] counts entries with prefix_len <= i, the SDK's v6
 *   array entries_before_exclude_pfLen[i] counts prefix_len > i.
 * - The top six logical entries are reserved for the IPv4 and IPv6
 *   catch-alls and the two alignment holes between them.
 *
 * The route pool is shared with OpenFlow (same physical TCAM, FMT != 0
 * entries via the FT_L3_TCAM_0 view, SDK RTK_DEFAULT_L3_OPENFLOW_CUTLINE
 * in rtk/default.h). The shipped default cutline assigns the whole pool
 * to L3 routing; nothing here programs OpenFlow entries, and route
 * entries set BMSK_FMT so the two uses can never match each other.
 *
 * The shared L3 code addresses prefix routes by their software route id
 * (0..MAX_ROUTES-1), not by a table position: route_read/route_write
 * receive the id, and route_lookup_hw must return something the other
 * two accept, so it returns the id as well. The id-to-position mapping
 * below decouples the shared first-free-id allocation from the sorted
 * hardware order.
 */
#define RTL931X_L3_ROUTE_TBL_SIZE		12288
/* dal_mango_l3_init: the RTL9311E variant clamps the table (SDK
 * DAL_MANGO_L3_ROUTE_TBL_SIZE_FOR_RTL9311E). Only the accounting is
 * clamped here; the variant is otherwise untested.
 */
#define RTL931X_L3_ROUTE_TBL_SIZE_9311E		768

struct rtl931x_l3_prefix_tbl {
	int size;		/* dynamic route pool entries (9311E clamps) */
	int ip_cnt;		/* v4 entries in use, region [0, ip_cnt) */
	u16 ip_pflen[32];	/* ip_pflen[i] = v4 entries with prefix_len <= i */
	int ip6_cnt;		/* v6 entries in use, region grows down from size */
	u16 ip6_pflen[128];	/* ip6_pflen[i] = v6 entries with prefix_len > i */
	s16 id2pos[MAX_ROUTES];	/* shared route id -> table position, -1 = none */
	s16 pos2id[RTL931X_L3_ROUTE_TBL_SIZE];	/* position -> id, -1 = none */
};

static struct rtl931x_l3_prefix_tbl rtl931x_ptbl;

/* Move len prefix-table entries from src to dst with the hardware move
 * engine (SDK __dal_mango_l3_routeEntry_move). MANGO_L3_ENTRY_MV_CTRLr
 * @0xF260: TO 29:16, FROM 15:2 (both PACKED TCAM addresses, not logical
 * indices), CMD 1 (1 = move, 0 = clear), EXEC 0; MANGO_L3_ENTRY_MV_PARAMr
 * @0xF264: LEN 13:0 as a plain entry count (swcore_rtl9310.h). For an
 * upward move (src < dst) FROM/TO name the END of each block so the
 * engine copies the tail first and the overlap cannot clobber. The SDK
 * polls EXEC 512 times before and after; a pre-move timeout is a hard
 * failure there, and so is a post-move timeout here: the table contents
 * are unknown afterwards and only the log records it.
 */
static int rtl931x_l3_route_move(int dst, int src, int len)
{
	u32 from, to;
	int i;

	if (src > dst) {
		from = rtl931x_l3_idx_to_addr(src);
		to = rtl931x_l3_idx_to_addr(dst);
	} else {
		from = rtl931x_l3_idx_to_addr(src + len - 1);
		to = rtl931x_l3_idx_to_addr(dst + len - 1);
	}

	for (i = 0; i < 512; i++) {
		if (!(sw_r32(RTL931X_L3_ENTRY_MV_CTRL) & BIT(0)))
			break;
	}
	if (i == 512) {
		pr_err("%s: move engine busy, move %d->%d len %d refused\n",
		       __func__, src, dst, len);
		return -EBUSY;
	}

	sw_w32_mask(0x3fff, len, RTL931X_L3_ENTRY_MV_PARAM);
	sw_w32((to << 16) | (from << 2) | BIT(1) | BIT(0),
	       RTL931X_L3_ENTRY_MV_CTRL);

	for (i = 0; i < 512; i++) {
		if (!(sw_r32(RTL931X_L3_ENTRY_MV_CTRL) & BIT(0)))
			return 0;
	}
	pr_err("%s: move %d->%d len %d timed out, prefix table state unknown\n",
	       __func__, src, dst, len);
	return -ETIMEDOUT;
}

/* Clear len prefix-table entries starting at base (SDK
 * __dal_mango_l3_routeEntry_clear): same engine, CMD = 0, only FROM.
 */
static int rtl931x_l3_route_clear(int base, int len)
{
	int i;

	for (i = 0; i < 512; i++) {
		if (!(sw_r32(RTL931X_L3_ENTRY_MV_CTRL) & BIT(0)))
			break;
	}
	if (i == 512) {
		pr_err("%s: move engine busy, clear %d len %d refused\n",
		       __func__, base, len);
		return -EBUSY;
	}

	sw_w32_mask(0x3fff, len, RTL931X_L3_ENTRY_MV_PARAM);
	sw_w32((rtl931x_l3_idx_to_addr(base) << 2) | BIT(0),
	       RTL931X_L3_ENTRY_MV_CTRL);

	for (i = 0; i < 512; i++) {
		if (!(sw_r32(RTL931X_L3_ENTRY_MV_CTRL) & BIT(0)))
			return 0;
	}
	pr_err("%s: clear %d len %d timed out, prefix table state unknown\n",
	       __func__, base, len);
	return -ETIMEDOUT;
}

/* Read and decode the prefix-route entry at table position pos. An
 * invalid entry sets only attr.valid and leaves the rest of the route
 * untouched: the shared code reuses the caller's dst_ip/prefix_len when
 * reprogramming (same contract as rtl930x_route_read()).
 */
static void rtl931x_prefix_entry_read(int pos, struct rtl83xx_route *rt)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 4);
	struct in6_addr ip6_m;
	bool host_route, default_route;
	u32 data[6], v, w;
	u64 chunk;

	rtl_table_read(r, rtl931x_l3_idx_to_addr(pos));

	/* L3_PREFIX_ROUTE_IPUC entry, 192 bits (MANGO_L3_PREFIX_ROUTE_IPUCt,
	 * rtk_mango_tableField_list.c):
	 * word 0: VALID 31, FMT 30, ENTRY_TYPE 29:28, VRF_ID 27:20,
	 *         IP[31:12] 19:0
	 * word 1: IP[11:0] 31:20, BMSK_FMT 10, BMSK_ENTRY_TYPE 9:8,
	 *         BMSK_VRF_ID 7:0
	 * word 2: BMSK_IP 31:0
	 * word 3: HOST_ROUTE 22, DFLT_ROUTE 21, DST_NULL_INTF 20,
	 *         ACT 19:17, ECMP_EN 16, NH_ECMP_IDX 15:3, TTL_DEC 2,
	 *         TTL_CHK 1, QOS_EN 0
	 * word 4: QOS_PRI 31:29
	 * word 5: HIT 27
	 */
	for (int i = 0; i < 6; i++)
		data[i] = sw_r32(rtl_table_data(r, i));
	v = data[0];
	rt->attr.valid = !!(v & BIT(31));
	if (!rt->attr.valid)
		goto out;
	rt->attr.type = (v >> 28) & 0x3;
	w = data[1];
	v = data[3];
	host_route = !!(v & BIT(22));
	default_route = !!(v & BIT(21));

	switch (rt->attr.type) {
	case 0: /* IPv4 unicast */
		rt->dst_ip = ((data[0] & 0xfffff) << 12) | (w >> 20);
		rt->prefix_len = host_route ? 32 : -1;
		if (rt->prefix_len < 0 && default_route)
			rt->prefix_len = 0;
		if (rt->prefix_len < 0)
			rt->prefix_len = inet_mask_len(data[2]);
		break;
	case 2: /* IPv6 unicast */
		/* Other type-2 slots are continuations, not IPv6 entry bases. */
		if (pos % 6 != 0 && pos % 6 != 3) {
			pr_warn_ratelimited("%s: IPv6 route at unaligned slot %d is not decodable\n",
					    __func__, pos);
			goto out;
		}
		rtl931x_ip6_word_set(&rt->dst_ip6, 0,
				      ((data[0] & 0xfffff) << 12) | (w >> 20));
		rtl931x_ip6_word_set(&ip6_m, 0, data[2]);
		for (int k = 1; k < 3; k++) {
			rtl_table_read(r, rtl931x_l3_idx_to_addr(pos + k));
			v = sw_r32(rtl_table_data(r, 0));
			w = sw_r32(rtl_table_data(r, 1));
			chunk = ((u64)(v & 0x0fffffff) << 20) | (w >> 12);
			rtl931x_ip6_chunk48_set(&rt->dst_ip6, 4 + (k - 1) * 6,
						     chunk);
			chunk = ((u64)(w & 0xff) << 40) |
				((u64)sw_r32(rtl_table_data(r, 2)) << 8) |
				(sw_r32(rtl_table_data(r, 3)) >> 24);
			rtl931x_ip6_chunk48_set(&ip6_m, 4 + (k - 1) * 6, chunk);
		}
		rt->prefix_len = host_route ? 128 : -1;
		if (rt->prefix_len < 0 && default_route)
			rt->prefix_len = 0;
		if (rt->prefix_len < 0)
			rt->prefix_len = rtldsa_ip6_mask_len(&ip6_m);
		break;
	case 1: /* IPv4 multicast */
	case 3: /* IPv6 multicast */
		pr_warn("%s: route type %d not supported\n", __func__, rt->attr.type);
		goto out;
	}

	v = data[3];
	rt->attr.dst_null = !!(v & BIT(20));
	rt->attr.action = (v >> 17) & 0x7;
	rt->nh.id = (v >> 3) & 0x1fff;
	rt->attr.ttl_dec = !!(v & BIT(2));
	rt->attr.ttl_check = !!(v & BIT(1));
	rt->attr.qos_as = !!(v & BIT(0));
	rt->attr.qos_prio = (data[4] >> 29) & 0x7;
	rt->attr.hit = !!(data[5] & BIT(27));

out:
	rtl_table_release(r);
}

/* Encode and write a valid prefix-route entry at table position pos;
 * see rtl931x_prefix_entry_read() for the layout. The key fields follow
 * the SDK (l3_util_rtkRoute2routeEntry): FMT = 0 with BMSK_FMT = 1 (an
 * OpenFlow entry never matches as a route), ENTRY_TYPE and VRF_ID fully
 * cared, BMSK_IP the prefix mask.
 */
static void rtl931x_prefix_entry_write4(int pos, struct rtl83xx_route *rt)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 4);
	u32 v;

	sw_w32(BIT(31) | ((rt->dst_ip >> 12) & 0xfffff), rtl_table_data(r, 0));
	sw_w32(((rt->dst_ip & 0xfff) << 20) | BIT(10) | (0x3 << 8) | 0xff,
	       rtl_table_data(r, 1));
	sw_w32(inet_make_mask(rt->prefix_len), rtl_table_data(r, 2));

	v = rt->prefix_len >= 32 ? BIT(22) : 0;		/* HOST_ROUTE */
	v |= rt->prefix_len == 0 ? BIT(21) : 0;		/* DFLT_ROUTE */
	v |= rt->attr.dst_null ? BIT(20) : 0;
	v |= (rt->attr.action & 0x7) << 17;
	v |= (rt->nh.id & 0x1fff) << 3;			/* ECMP_EN stays 0 */
	v |= rt->attr.ttl_dec ? BIT(2) : 0;
	v |= rt->attr.ttl_check ? BIT(1) : 0;
	v |= rt->attr.qos_as ? BIT(0) : 0;
	sw_w32(v, rtl_table_data(r, 3));

	sw_w32((rt->attr.qos_prio & 0x7) << 29, rtl_table_data(r, 4));
	sw_w32(rt->attr.hit ? BIT(27) : 0, rtl_table_data(r, 5));

	rtl_table_write(r, rtl931x_l3_idx_to_addr(pos));
	rtl_table_release(r);
}

static void rtl931x_prefix_entry_write6(int pos, struct rtl83xx_route *rt)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 4);
	struct in6_addr ip6_m;
	u64 chunk, mask_chunk;
	u32 ip, mask, v;

	rtldsa_net6_mask(rt->prefix_len, &ip6_m);
	ip = rtl931x_ip6_word(&rt->dst_ip6, 0);
	mask = rtl931x_ip6_word(&ip6_m, 0);

	sw_w32(BIT(31) | (0x2 << 28) | ((ip >> 12) & 0xfffff),
	       rtl_table_data(r, 0));
	sw_w32(((ip & 0xfff) << 20) | BIT(10) | (0x3 << 8) | 0xff,
	       rtl_table_data(r, 1));
	sw_w32(mask, rtl_table_data(r, 2));

	v = rt->prefix_len >= 128 ? BIT(22) : 0;		/* HOST_ROUTE */
	v |= rt->prefix_len == 0 ? BIT(21) : 0;		/* DFLT_ROUTE */
	v |= rt->attr.dst_null ? BIT(20) : 0;
	v |= (rt->attr.action & 0x7) << 17;
	v |= (rt->nh.id & 0x1fff) << 3;			/* ECMP_EN stays 0 */
	v |= rt->attr.ttl_dec ? BIT(2) : 0;
	v |= rt->attr.ttl_check ? BIT(1) : 0;
	v |= rt->attr.qos_as ? BIT(0) : 0;
	sw_w32(v, rtl_table_data(r, 3));
	sw_w32((rt->attr.qos_prio & 0x7) << 29, rtl_table_data(r, 4));
	sw_w32(rt->attr.hit ? BIT(27) : 0, rtl_table_data(r, 5));
	rtl_table_write(r, rtl931x_l3_idx_to_addr(pos));

	for (int k = 1; k < 3; k++) {
		chunk = rtl931x_ip6_chunk48(&rt->dst_ip6, 4 + (k - 1) * 6);
		mask_chunk = rtl931x_ip6_chunk48(&ip6_m, 4 + (k - 1) * 6);
		sw_w32(BIT(31) | (0x2 << 28) |
		       ((u32)(chunk >> 20) & 0x0fffffff), rtl_table_data(r, 0));
		sw_w32(((u32)chunk & 0xfffff) << 12 | BIT(10) | (0x3 << 8) |
		       ((u32)(mask_chunk >> 40) & 0xff), rtl_table_data(r, 1));
		sw_w32((u32)(mask_chunk >> 8), rtl_table_data(r, 2));
		sw_w32((u32)mask_chunk << 24, rtl_table_data(r, 3));
		sw_w32(0, rtl_table_data(r, 4));
		sw_w32(0, rtl_table_data(r, 5));
		rtl_table_write(r, rtl931x_l3_idx_to_addr(pos + k));
	}

	rtl_table_release(r);
}

static void rtl931x_prefix_entry_write(int pos, struct rtl83xx_route *rt)
{
	if (rt->attr.type == 0)
		rtl931x_prefix_entry_write4(pos, rt);
	else if (rt->attr.type == 2)
		rtl931x_prefix_entry_write6(pos, rt);
	else
		pr_warn("%s: route type %d not supported\n", __func__, rt->attr.type);
}

/* Insert a route into the sorted v4 region
 * (SDK __dal_mango_l3_routeEntry_alloc, IPv4 half): the entries shorter
 * than the new prefix are shifted one position up, one boundary entry
 * per move-engine command, opening a slot behind the last entry of
 * equal-or-longer prefix. /0 appends at the bottom directly. On any
 * failure the route is simply not programmed and keeps working in
 * software via the catch-all trap.
 */
static void rtl931x_prefix_route_insert(int id, struct rtl83xx_route *rt)
{
	struct rtl931x_l3_prefix_tbl *t = &rtl931x_ptbl;
	int dst, src, pos;

	if (t->ip_cnt + 1 > t->size - 3 * t->ip6_cnt) {
		pr_err("%s: prefix table full, route to %pI4/%d stays in software\n",
		       __func__, &rt->dst_ip, rt->prefix_len);
		return;
	}

	dst = t->ip_cnt;
	if (rt->prefix_len > 0) {
		for (int pfl = 0; pfl < rt->prefix_len; pfl++) {
			int mid;

			src = t->ip_cnt - t->ip_pflen[pfl];
			if (src == dst)
				continue;
			if (rtl931x_l3_route_move(dst, src, 1))
				return;
			mid = t->pos2id[src];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
			dst = src;
		}
	}
	pos = dst;

	rtl931x_prefix_entry_write(pos, rt);

	for (int i = rt->prefix_len; i < 32; i++)
		t->ip_pflen[i]++;
	t->ip_cnt++;
	t->pos2id[pos] = id;
	t->id2pos[id] = pos;
}

/* Insert a route into the sorted v6 region. Triples grow downward from
 * the top of the dynamic pool while the longest prefixes remain at the
 * lowest indices.
 */
static void rtl931x_prefix_route_insert6(int id, struct rtl83xx_route *rt)
{
	struct rtl931x_l3_prefix_tbl *t = &rtl931x_ptbl;
	int dst, src, pos;

	if (3 * (t->ip6_cnt + 1) > t->size - t->ip_cnt) {
		pr_err("%s: prefix table full, route to %pI6c/%d stays in software\n",
		       __func__, &rt->dst_ip6, rt->prefix_len);
		return;
	}

	dst = t->size - 3 * (t->ip6_cnt + 1);
	/* A triple base must map to slot 0 or 3 of a packed row. */
	if (WARN_ON_ONCE(dst % 6 != 0 && dst % 6 != 3))
		return;
	src = dst;
	if (rt->prefix_len < 128) {
		for (int pfl = 128; pfl > rt->prefix_len; pfl--) {
			int mid;

			src = t->size -
			      3 * (t->ip6_cnt - t->ip6_pflen[pfl - 1] + 1);
			if (src == dst)
				continue;
			if (rtl931x_l3_route_move(dst, src, 3))
				return;
			mid = t->pos2id[src];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
			dst = src;
		}
	}
	pos = src;

	rtl931x_prefix_entry_write(pos, rt);

	for (int i = rt->prefix_len; i > 0; i--)
		t->ip6_pflen[i - 1]++;
	t->ip6_cnt++;
	t->pos2id[pos] = id;
	t->id2pos[id] = pos;
}

/* Remove the route id from the sorted v4 region (SDK
 * __dal_mango_l3_routeEntry_free, IPv4 half): the boundary entries are
 * moved down one position each to close the gap, the vacated bottom
 * entry is invalidated with the move engine in clear mode, and the
 * bookkeeping is decremented afterwards (the moves above rely on the
 * pre-delete histogram). The prefix length is read back from the
 * hardware entry, as the SDK does, so a caller-side mistake cannot skew
 * the histogram.
 */
static void rtl931x_prefix_route_remove(int id)
{
	struct rtl931x_l3_prefix_tbl *t = &rtl931x_ptbl;
	struct rtl83xx_route cur;
	int plen, bottom, dst, src;
	int pos = t->id2pos[id];

	if (pos < 0)
		return;	/* never programmed (e.g. gateway never resolved) */

	memset(&cur, 0, sizeof(cur));
	rtl931x_prefix_entry_read(pos, &cur);
	if (!cur.attr.valid) {
		pr_warn("%s: id %d maps to invalid entry %d, bookkeeping lost\n",
			__func__, id, pos);
		t->pos2id[pos] = -1;
		t->id2pos[id] = -1;
		return;
	}
	plen = cur.prefix_len;
	bottom = t->ip_cnt - 1;

	if (pos < bottom) {
		dst = pos;
		for (int pfl = plen; pfl > 0; pfl--) {
			int mid;

			src = t->ip_cnt - t->ip_pflen[pfl - 1] - 1;
			if (src == dst)
				continue;
			if (rtl931x_l3_route_move(dst, src, 1))
				return;
			mid = t->pos2id[src];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
			dst = src;
		}
		if (dst != bottom) {
			int mid;

			if (rtl931x_l3_route_move(dst, bottom, 1))
				return;
			mid = t->pos2id[bottom];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
		}
	}

	/* Invalidate the vacated bottom entry. If the clear fails the
	 * entry survives as a stale duplicate of the moved-down one (the
	 * lower copy wins every lookup), so the bookkeeping is completed
	 * regardless: a live id must never keep pointing at another
	 * route's entry.
	 */
	rtl931x_l3_route_clear(bottom, 1);

	for (int i = plen; i < 32; i++)
		t->ip_pflen[i]--;
	t->ip_cnt--;
	t->pos2id[bottom] = -1;
	t->id2pos[id] = -1;
}

/* Remove a route from the sorted v6 region. The move engine handles a
 * triple as one aligned command; only each triple's base is mapped.
 */
static void rtl931x_prefix_route_remove6(int id)
{
	struct rtl931x_l3_prefix_tbl *t = &rtl931x_ptbl;
	struct rtl83xx_route cur;
	int plen, top, dst, src;
	int pos = t->id2pos[id];

	if (pos < 0)
		return;	/* never programmed (e.g. gateway never resolved) */

	memset(&cur, 0, sizeof(cur));
	rtl931x_prefix_entry_read(pos, &cur);
	if (!cur.attr.valid) {
		pr_warn("%s: id %d maps to invalid entry %d, bookkeeping lost\n",
			__func__, id, pos);
		t->pos2id[pos] = -1;
		t->id2pos[id] = -1;
		return;
	}
	plen = cur.prefix_len;
	top = t->size - 3 * t->ip6_cnt;

	if (pos > top) {
		dst = pos;
		for (int pfl = plen; pfl < 128; pfl++) {
			int mid;

			src = t->size - 3 * (t->ip6_cnt - t->ip6_pflen[pfl]);
			if (src == dst)
				continue;
			if (rtl931x_l3_route_move(dst, src, 3))
				return;
			mid = t->pos2id[src];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
			dst = src;
		}
		if (dst != top) {
			int mid;

			if (rtl931x_l3_route_move(dst, top, 3))
				return;
			mid = t->pos2id[top];
			t->pos2id[dst] = mid;
			if (mid >= 0)
				t->id2pos[mid] = dst;
		}
	}

	/* A failed clear leaves only a stale higher-index duplicate, so
	 * complete the bookkeeping as the v4 removal path does.
	 */
	rtl931x_l3_route_clear(top, 3);

	for (int i = plen; i > 0; i--)
		t->ip6_pflen[i - 1]--;
	t->ip6_cnt--;
	t->pos2id[top] = -1;
	t->id2pos[id] = -1;
}

/* Read a prefix route by its software route id; see the region comment
 * above for the id/position split.
 */
static void rtl931x_route_read(int id, struct rtl83xx_route *rt)
{
	int pos;

	if (rt->attr.type == 1 || rt->attr.type == 3) {
		pr_warn("%s: multicast route type %d not supported\n",
			__func__, rt->attr.type);
		rt->attr.valid = false;
		return;
	}

	if (id < 0 || id >= MAX_ROUTES) {
		pr_warn_ratelimited("%s: route id %d out of range\n", __func__, id);
		rt->attr.valid = false;
		return;
	}

	pos = rtl931x_ptbl.id2pos[id];
	if (pos < 0) {
		rt->attr.valid = false;
		return;
	}

	rtl931x_prefix_entry_read(pos, rt);
}

/* Write a prefix route by its software route id. A valid route updates
 * its entry in place when already programmed (the shared code rewrites
 * the action/nexthop once the gateway neighbour resolves) and inserts
 * into the sorted region otherwise; an invalid one is removed.
 */
static void rtl931x_route_write(int id, struct rtl83xx_route *rt)
{
	int pos;

	if (rt->attr.type == 1 || rt->attr.type == 3) {
		pr_warn_ratelimited("%s: multicast route type %d not supported, stays in software\n",
				    __func__, rt->attr.type);
		return;
	}
	if (rt->attr.type != 0 && rt->attr.type != 2) {
		pr_warn_ratelimited("%s: route type %d not supported, stays in software\n",
				    __func__, rt->attr.type);
		return;
	}

	if (id < 0 || id >= MAX_ROUTES) {
		pr_warn_ratelimited("%s: route id %d out of range\n", __func__, id);
		return;
	}

	if (!rt->attr.valid) {
		if (rt->attr.type == 2)
			rtl931x_prefix_route_remove6(id);
		else
			rtl931x_prefix_route_remove(id);
		return;
	}

	if (rt->prefix_len < 0 ||
	    rt->prefix_len > (rt->attr.type == 2 ? 128 : 32)) {
		pr_warn("%s: prefix_len %d out of range\n", __func__, rt->prefix_len);
		return;
	}

	pos = rtl931x_ptbl.id2pos[id];
	if (pos >= 0) {
		rtl931x_prefix_entry_write(pos, rt);
		return;
	}

	if (rt->attr.type == 2)
		rtl931x_prefix_route_insert6(id, rt);
	else
		rtl931x_prefix_route_insert(id, rt);
}

/* Hardware longest-prefix-match lookup of a prefix route (SDK
 * __dal_mango_l3_routeEntry_hwLookup, MANGO_L3_ROUTE_HW_LU): the key is
 * {VRF 0, unicast ENTRY_TYPE, DIP = masked destination}, all other key
 * fields 0. MANGO_L3_HW_LU_KEY_CTRLr @0xF29C, the 128-bit
 * MANGO_L3_HW_LU_KEY_DIP_CTRLr block @0xF2B0 (high word first, IPv4 in
 * the low word at +0xc), MANGO_L3_HW_LU_CTRLr @0xF2C0:
 * EXEC_TCAM 15, RESULT_TCAM 14, ENTRY_IDX_TCAM 13:0 - the result is a
 * packed TCAM address. Being an LPM, a route that was never programmed
 * resolves to its covering entry (e.g. the catch-all); the catch-all is
 * not driver-owned, so it maps to -1 and can never be invalidated
 * through this path. Returns the software route id of the hit.
 */
static int rtl931x_route_lookup_hw(struct rtl83xx_route *rt)
{
	struct in6_addr ip6_m;
	u32 v;
	int i, pos;

	if (rt->attr.type != 0 && rt->attr.type != 2)
		return -1;

	/* Key fields VID_INTF_ID 11:0, MC_KEY_SEL 12, VRF 20:13, IPMC_TYPE
	 * 21 and ROUND 24 go to 0; ENTRY_TYPE 23:22 selects IPv4 or IPv6
	 * unicast. TEST_MODE (25) and above keep their values.
	 */
	sw_w32_mask(0x1ffffff, (rt->attr.type & 0x3) << 22,
		    RTL931X_L3_HW_LU_KEY_CTRL);
	if (rt->attr.type == 2) {
		rtldsa_net6_mask(rt->prefix_len, &ip6_m);
		for (int w = 0; w < 4; w++)
			sw_w32(rtl931x_ip6_word(&rt->dst_ip6, w * 4) &
			       rtl931x_ip6_word(&ip6_m, w * 4),
			       RTL931X_L3_HW_LU_KEY_DIP_CTRL + w * 4);
	} else {
		sw_w32(0, RTL931X_L3_HW_LU_KEY_DIP_CTRL);
		sw_w32(0, RTL931X_L3_HW_LU_KEY_DIP_CTRL + 4);
		sw_w32(0, RTL931X_L3_HW_LU_KEY_DIP_CTRL + 8);
		sw_w32(rt->dst_ip & inet_make_mask(rt->prefix_len),
		       RTL931X_L3_HW_LU_KEY_DIP_CTRL + 0xc);
	}

	sw_w32_mask(BIT(15), BIT(15), RTL931X_L3_HW_LU_CTRL);
	for (i = 0; i < 512; i++) {
		udelay(1);
		v = sw_r32(RTL931X_L3_HW_LU_CTRL);
		if (!(v & BIT(15)))
			break;
	}
	if (i == 512) {
		pr_err("%s: lookup timed out\n", __func__);
		return -1;
	}
	if (!(v & BIT(14)))
		return -1;

	pos = rtl931x_l3_addr_to_idx(v & 0x3fff);
	if (pos >= rtl931x_ptbl.size)
		return -1;

	return rtl931x_ptbl.pos2id[pos];
}

/* Get the destination L2 index and the egress interface of a nexthop
 * entry from the L3_NEXTHOP table. The Mango nexthop couples L3 to the
 * L2 FDB: DMAC_IDX is the physical index of the L2 entry holding the
 * destination MAC (the SDK's _dal_mango_l2_nexthop_add returns exactly
 * this (hash row << 2) | bucket format, which the shared
 * rtl83xx_l2_nexthop_add() also produces), the special values are
 * 0xFFFC invalid/tunnel, 0xFFFD trap-to-master, 0xFFFE trap-to-CPU,
 * 0xFFFF drop.
 */
static void rtl931x_get_l3_nexthop(int idx, u16 *dmac_id, u16 *interface)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 6);
	u32 v;

	rtl_table_read(r, idx);
	v = sw_r32(rtl_table_data(r, 0));
	rtl_table_release(r);

	*dmac_id = (v >> 16) & 0xffff;
	*interface = (v >> 6) & 0x3ff;
}

/* Set the destination L2 index and the egress interface of a nexthop
 * entry in the L3_NEXTHOP table; see rtl931x_get_l3_nexthop().
 */
static void rtl931x_set_l3_nexthop(int idx, u16 dmac_id, u16 interface)
{
	struct table_reg *r = rtl_table_get(RTL9310_TBL_2, 6);

	sw_w32(((u32)dmac_id << 16) | ((interface & 0x3ff) << 6),
	       rtl_table_data(r, 0));
	rtl_table_write(r, idx);
	rtl_table_release(r);
}

static int rtl931x_l3_setup(struct rtl838x_switch_priv *priv)
{
	struct table_reg *r;
	int tbl_size, catchall4, catchall6_pos;

	/* Prefix-route bookkeeping: empty sorted regions, no id mappings.
	 * The RTL9311E clamps the table to 768 entries (dal_mango_l3_init,
	 * DAL_MANGO_L3_ROUTE_TBL_SIZE_FOR_RTL9311E); the RTL_ID field of
	 * MODEL_NAME_INFO (@0x4, bits 31:16) identifies the chip
	 * (include/hal/chipdef/chip.h: 0x9311 = 9311E, 0x9313 = 9313).
	 */
	memset(&rtl931x_ptbl, 0, sizeof(rtl931x_ptbl));
	memset(rtl931x_ptbl.id2pos, 0xff, sizeof(rtl931x_ptbl.id2pos));
	memset(rtl931x_ptbl.pos2id, 0xff, sizeof(rtl931x_ptbl.pos2id));
	tbl_size = RTL931X_L3_ROUTE_TBL_SIZE;
	if ((sw_r32(RTL93XX_MODEL_NAME_INFO) >> 16) == 0x9311) {
		tbl_size = RTL931X_L3_ROUTE_TBL_SIZE_9311E;
		pr_info("RTL9311E: prefix route table clamped to %d entries\n",
			tbl_size);
	}
	/* The pool remains a multiple of six, keeping v6 bases at slot 0 or 3. */
	rtl931x_ptbl.size = tbl_size - 6;
	catchall4 = tbl_size - 1;
	catchall6_pos = tbl_size - 6;

	/* Route ids are software handles here: the prefix pool is the table
	 * minus its reserved top, and the host table is the same size as the
	 * route table on both variants.
	 */
	priv->n_route_ids = rtl931x_ptbl.size;
	priv->n_host_route_ids = tbl_size;

	for (int i = 0; i < MAX_INTF_MTUS; i++)
		priv->intf_mtu_count[i] = priv->intf_mtus[i] = 0;

	/* MTU slot 0 is the reserved default-interface MTU (SDK
	 * DAL_MANGO_L3_RESERVED_INTF_MTU_IDX), slot 1 serves the driver's
	 * egress interfaces. MANGO_L3_INTF_IP_MTUr @0xF1E0 and
	 * MANGO_L3_INTF_IP6_MTUr @0xF220, 16 slots, MTU value in bits 13:0.
	 */
	priv->intf_mtus[0] = DEFAULT_MTU;
	priv->intf_mtus[1] = DEFAULT_MTU;
	for (int i = 0; i < 2; i++) {
		sw_w32_mask(0x3fff, DEFAULT_MTU, RTL931X_L3_INTF_IP_MTU(i));
		sw_w32_mask(0x3fff, DEFAULT_MTU, RTL931X_L3_INTF_IP6_MTU(i));
	}

	/* Host table hash algorithms (MANGO_L3_HOST_TBL_CTRLr @0xF004):
	 * table 0 -> algorithm 0 (XOR), table 1 -> algorithm 1 (carry-fold
	 * sum), as in dal_mango_l3_init(). Masked write: the MC algorithm
	 * selects and the lookup-mode bits keep their reset values.
	 */
	sw_w32_mask(0x3 << 2, BIT(3), RTL931X_L3_HOST_TBL_CTRL);

	/* MANGO_L3_IP_ROUTE_CTRLr @0xF000:
	 * - NON_IP_ACT = TRAP2CPU: non-IP traffic to a router MAC (e.g.
	 *   ARP) must reach the CPU so the switch stays reachable.
	 * - NH_AGE_OUT_ACT = TRAP2CPU: a packet whose nexthop L2 entry was
	 *   invalidated traps so the kernel re-resolves the neighbour
	 *   instead of being silently dropped.
	 * - NH_ERR_ACT = TRAP2CPU: same for a nexthop pointing at an
	 *   invalid or reserved entry.
	 */
	sw_w32_mask((0x3 << 10) | (0x7 << 6) | (0x3 << 4),
		    (0x1 << 10) | (0x1 << 6) | (0x1 << 4),
		    RTL931X_L3_IP_ROUTE_CTRL);

	/* MANGO_L3_IPUC_ROUTE_CTRLr @0xF008:
	 * - GLB_EN: enable IPv4 unicast routing.
	 * - TTL_FAIL_ACT / MTU_FAIL_ACT = TRAP2CPU: expired-TTL and
	 *   over-MTU packets must reach the CPU for ICMP time-exceeded /
	 *   fragmentation-needed (traceroute, PMTUD).
	 * - HDR_OPT_ACT = FORWARD (2): route packets carrying IP options.
	 * - DMAC_BC_ACT = TRAP2CPU: IP-broadcast through the router MAC
	 *   (e.g. DHCP) must not be dropped in hardware.
	 * BAD_SIP/BAD_DIP/ZERO_SIP/DMAC_MC acts keep their reset values.
	 */
	sw_w32_mask(BIT(0) | (0x3 << 7) | (0x7 << 11) | (0x3 << 14) | (0x3 << 16),
		    BIT(0) | (0x1 << 7) | (0x2 << 11) | (0x1 << 14) | (0x1 << 16),
		    RTL931X_L3_IPUC_ROUTE_CTRL);

	/* MANGO_L3_IP6UC_ROUTE_CTRLr @0xF00C:
	 * - GLB_EN: enable IPv6 unicast routing.
	 * - MTU_FAIL_ACT = TRAP2CPU: packet-too-big must reach the CPU for PMTUD.
	 * - HL_FAIL_ACT = TRAP2CPU: expired hop limits must reach the CPU.
	 * - HDR_ROUTE_ACT = FORWARD (2): route packets with extension headers.
	 * Hop-by-hop actions and the other exception actions keep reset values.
	 */
	sw_w32_mask(BIT(0) | (0x7 << 15) | (0x3 << 18) | (0x3 << 20),
		    BIT(0) | (0x2 << 15) | (0x1 << 18) | (0x1 << 20),
		    RTL931X_L3_IP6UC_ROUTE_CTRL);

	/* Enable the L3 TCAMs (MANGO_ALE_L3_MISC_CTRLr @0xF2E8): prefix
	 * TCAM blocks 0-5 (L3_TCAM_BLK_EN = 0x3F) and the router-MAC TCAM
	 * (ROUTER_MAC_TCAM_EN, bit 7). Without these nothing routes and no
	 * error is signalled anywhere (dal_mango_l3_init()).
	 */
	sw_w32_mask(0x3f | BIT(7), 0x3f | BIT(7), RTL931X_ALE_L3_MISC_CTRL);

	/* Reserve nexthop 0 (dal_mango_l3_init): DMAC_IDX = 0xFFFC is the
	 * invalid/tunnel DMAC index, egress interface 0 is the reserved
	 * default-bridging interface. MANGO_L3_NEXTHOPt entry: DMAC_IDX
	 * bits 31:16, L3_EGR_INTF_IDX bits 15:6.
	 */
	r = rtl_table_get(RTL9310_TBL_2, 6);
	sw_w32(0xfffc << 16, rtl_table_data(r, 0));
	rtl_table_write(r, 0);
	rtl_table_release(r);

	/* Mango has no unicast route-miss action: the unicast route controls
	 * carry only exception actions, while L3_IGR_INTF exposes lookup-miss
	 * actions only for multicast. The prefix TCAM returns the lowest
	 * matching address, so both catch-alls sit at the top where every
	 * specific route shadows them.
	 *
	 * Catch-all IPv4 prefix entry:
	 * valid, entry type IPv4-UC with the type bits cared
	 * (BMSK_ENTRY_TYPE = 3), all other masks 0, DFLT_ROUTE, action
	 * TRAP2CPU. A router-MAC-matched packet missing the host table
	 * falls through to this entry and reaches the CPU instead of being
	 * silently dropped.
	 */
	r = rtl_table_get(RTL9310_TBL_2, 4);
	sw_w32(BIT(31), rtl_table_data(r, 0));		/* VALID, FMT 0, type IPUC, VRF 0, IP 0 */
	sw_w32(0x3 << 8, rtl_table_data(r, 1));		/* BMSK_ENTRY_TYPE = 3 */
	sw_w32(0, rtl_table_data(r, 2));		/* BMSK_IP = 0 */
	sw_w32(BIT(21) | (ROUTE_ACT_TRAP2CPU << 17),	/* DFLT_ROUTE | ACT */
	       rtl_table_data(r, 3));
	sw_w32(0, rtl_table_data(r, 4));
	sw_w32(0, rtl_table_data(r, 5));
	rtl_table_write(r, rtl931x_l3_idx_to_addr(catchall4));
	rtl_table_release(r);

	/* The IPv6 catch-all occupies an aligned triple below the IPv4
	 * catch-all. The two intervening slots stay empty because a triple
	 * starting there would overlap the IPv4 catch-all.
	 */
	r = rtl_table_get(RTL9310_TBL_2, 4);
	for (int k = 0; k < 3; k++) {
		sw_w32(BIT(31) | (0x2 << 28), rtl_table_data(r, 0));
		sw_w32(0x3 << 8, rtl_table_data(r, 1));
		sw_w32(0, rtl_table_data(r, 2));
		sw_w32(k ? 0 : BIT(21) | (ROUTE_ACT_TRAP2CPU << 17),
		       rtl_table_data(r, 3));
		sw_w32(0, rtl_table_data(r, 4));
		sw_w32(0, rtl_table_data(r, 5));
		rtl_table_write(r, rtl931x_l3_idx_to_addr(catchall6_pos + k));
	}
	rtl_table_release(r);

	return 0;
}

#endif /* CONFIG_NET_DSA_RTL83XX_RTL930X_L3_OFFLOAD */

const struct rtl838x_reg rtl931x_reg = {
	.mask_port_reg_be = rtl839x_mask_port_reg_be,
	.set_port_reg_be = rtl839x_set_port_reg_be,
	.get_port_reg_be = rtl839x_get_port_reg_be,
	.mask_port_reg_le = rtl839x_mask_port_reg_le,
	.set_port_reg_le = rtl839x_set_port_reg_le,
	.get_port_reg_le = rtl839x_get_port_reg_le,
	.stat_port_rst = RTL931X_STAT_PORT_RST,
	.stat_rst = RTL931X_STAT_RST,
	.stat_port_std_mib = 0,  /* Not defined */
	.stat_port_table_read = rtldsa_931x_stat_port_table_read,
	.stat_counters_lock = rtldsa_counters_lock_table,
	.stat_counters_unlock = rtldsa_counters_unlock_table,
	.stat_counter_poll_interval = RTLDSA_COUNTERS_FAST_POLL_INTERVAL,
	.traffic_enable = rtl931x_traffic_enable,
	.traffic_disable = rtl931x_traffic_disable,
	.traffic_set = rtl931x_traffic_set,
	.l2_ctrl_0 = RTL931X_L2_CTRL,
	.l2_ctrl_1 = RTL931X_L2_AGE_CTRL,
	.l2_port_aging_out = RTL931X_L2_PORT_AGE_CTRL,
	.set_ageing_time = rtl931x_set_ageing_time,
	.smi_poll_ctrl = RTL931X_SMI_PORT_POLLING_CTRL,
	.l2_tbl_flush_ctrl = RTL931X_L2_TBL_FLUSH_CTRL,
	.exec_tbl0_cmd = rtl931x_exec_tbl0_cmd,
	.exec_tbl1_cmd = rtl931x_exec_tbl1_cmd,
	.tbl_access_data_0 = rtl931x_tbl_access_data_0,
	.isr_glb_src = RTL931X_ISR_GLB_SRC,
	.isr_port_link_sts_chg = RTL931X_ISR_PORT_LINK_STS_CHG,
	.imr_port_link_sts_chg = RTL931X_IMR_PORT_LINK_STS_CHG,
	/* imr_glb does not exist on RTL931X */
	.vlan_tables_read = rtl931x_vlan_tables_read,
	.vlan_set_tagged = rtl931x_vlan_set_tagged,
	.vlan_set_untagged = rtl931x_vlan_set_untagged,
	.vlan_profile_dump = rtl931x_vlan_profile_dump,
	.vlan_profile_setup = rtl931x_vlan_profile_setup,
	.vlan_fwd_on_inner = rtl931x_vlan_fwd_on_inner,
	.vlan_qinq_setup = rtl931x_vlan_qinq_setup,
	.vlan_port_qinq_set = rtl931x_vlan_port_qinq_set,
	.stp_get = rtl931x_stp_get,
	.stp_set = rtl931x_stp_set,
	.mac_force_mode_ctrl = rtl931x_mac_force_mode_ctrl,
	.mac_port_ctrl = rtl931x_mac_port_ctrl,
	.l2_port_new_salrn = rtl931x_l2_port_new_salrn,
	.l2_port_new_sa_fwd = rtl931x_l2_port_new_sa_fwd,
	.get_mirror_config = rtldsa_931x_get_mirror_config,
	.port_rate_police_add = rtldsa_931x_port_rate_police_add,
	.port_rate_police_del = rtldsa_931x_port_rate_police_del,
	.read_l2_entry_using_hash = rtl931x_read_l2_entry_using_hash,
	.write_l2_entry_using_hash = rtl931x_write_l2_entry_using_hash,
	.read_cam = rtl931x_read_cam,
	.write_cam = rtl931x_write_cam,
	.vlan_port_keep_tag_set = rtl931x_vlan_port_keep_tag_set,
	.vlan_port_pvidmode_set = rtl931x_vlan_port_pvidmode_set,
	.vlan_port_pvid_set = rtl931x_vlan_port_pvid_set,
	.vlan_port_fast_age = rtldsa_931x_vlan_port_fast_age,
	.trk_mbr_ctr = rtldsa_931x_trk_mbr_ctr,
	.trunk_srcmap_set = rtl931x_trunk_srcmap_set,
	.trunk_egr_ports_set = rtl931x_trunk_egr_ports_set,
	.rma_bpdu_fld_pmask = RTL931X_RMA_BPDU_FLD_PMSK,
	.set_vlan_igr_filter = rtl931x_set_igr_filter,
	.set_vlan_egr_filter = rtl931x_set_egr_filter,
	.set_distribution_algorithm = rtl931x_set_distribution_algorithm,
	.l2_hash_key = rtl931x_l2_hash_key,
	.l2_hash_seed = rtldsa_931x_l2_hash_seed,
	.read_mcast_pmask = rtl931x_read_mcast_pmask,
	.write_mcast_pmask = rtl931x_write_mcast_pmask,
	.pie_init = rtl931x_pie_init,
	.pie_rule_write = rtl931x_pie_rule_write,
	.pie_rule_add = rtl931x_pie_rule_add,
	.pie_rule_rm = rtl931x_pie_rule_rm,
	.packet_cntr_read = rtl931x_packet_cntr_read,
	.packet_cntr_clear = rtl931x_packet_cntr_clear,
#ifdef CONFIG_NET_DSA_RTL83XX_RTL930X_L3_OFFLOAD
	.l3_setup = rtl931x_l3_setup,
	.l3_ecmp_offload = false,
	.l3_ip6_prefix_by_id = true,
	.get_l3_router_mac = rtl931x_get_l3_router_mac,
	.set_l3_router_mac = rtl931x_set_l3_router_mac,
	.set_l3_egress_intf = rtl931x_set_l3_egress_intf,
	.get_l3_egress_mac = rtl931x_get_l3_egress_mac,
	.set_l3_egress_mac = rtl931x_set_l3_egress_mac,
	.host_route_write = rtl931x_host_route_write,
	.host_route_hit_get_clear = rtl931x_host_route_hit_get_clear,
	.find_l3_slot = rtl931x_find_l3_slot,
	.route_read = rtl931x_route_read,
	.route_write = rtl931x_route_write,
	.route_lookup_hw = rtl931x_route_lookup_hw,
	.set_l3_nexthop = rtl931x_set_l3_nexthop,
	.get_l3_nexthop = rtl931x_get_l3_nexthop,
#endif
	.l2_learning_setup = rtl931x_l2_learning_setup,
	.led_init = rtldsa_931x_led_init,
	.enable_learning = rtldsa_931x_enable_learning,
	.enable_flood = rtldsa_931x_enable_flood,
	.enable_bcast_flood = rtldsa_931x_enable_bcast_flood,
	.set_receive_management_action = rtldsa_931x_set_receive_management_action,
	.vendor_init = rtl931x_vendor_init,
	.vendor_init_dump = rtl931x_vendor_init_dump,
	.flow_control_init = rtl931x_flow_control_init,
	.flow_control_dump = rtl931x_flow_control_dump,
	.qos_init = rtldsa_931x_qos_init,
};
