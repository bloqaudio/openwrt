/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL838X_H
#define _RTL838X_H

#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <net/dsa.h>

/* Register definition */
#define RTL838X_MAC_PORT_CTRL(port)		(0xd560 + (((port) << 7)))
#define RTL839X_MAC_PORT_CTRL(port)		(0x8004 + (((port) << 7)))
#define RTL930X_MAC_PORT_CTRL(port)		(0x3260 + (((port) << 6)))
#define RTL931X_MAC_PORT_CTRL			(0x6004)

#define RTL930X_MAC_L2_PORT_CTRL(port)		(0x3268 + (((port) << 6)))
#define RTL931X_MAC_L2_PORT_CTRL		(0x6000)

/* Maximum accepted frame length, one register per port (Longan):
 * bit 28: count VLAN tag bytes towards the limit
 * bits 27:14: limit when linked at 10M/100M
 * bits 13:0:  limit when linked at 1G/2.5G/5G/10G
 * Measured on RTL9303 rev B: resets to 12288 in both speed fields with
 * the global IOL max-len check disabled (MAC_L2_GLOBAL_CTRL0 bit 15 = 0),
 * which is how the stock firmware passes 12 KB frames without ever
 * writing these registers.
 */
#define RTL930X_MAC_L2_PORT_MAX_LEN_CTRL(port)	(0x326C + (((port) << 6)))

/* The CPU port needs a second register for the direction the per-port array does
 * not cover. Unlike the per-port register this holds a single 14-bit length
 * (CPU_PORT_TX_MAX_LEN, bits 13:0); bits 31:14 are reserved, so it must not be
 * written with the two speed-selected halves the per-port register uses.
 */
#define RTL930X_MAC_L2_CPU_MAX_LEN_CTRL		(0xA3A0)
#define RTL930X_CPU_MAX_LEN_M			GENMASK(13, 0)

/* Silicon maximum frame length (Longan capacity: 12 KB) */
#define RTL930X_MAX_FRAME_LEN			12288
#define RTL931X_MAC_L2_PORT_MAX_LEN_CTRL(port)	(0x5554 + (((port) << 2)))
#define RTL931X_MAC_L2_CPU_MAX_LEN_CTRL		(0x1368)
#define RTL931X_MAX_FRAME_LEN			12288

/* System clock select, used to derive the leaky-bucket tick/token values
 * (SDK dal_mango_construct.c): 0 = 650 MHz, 1 = 325 MHz, 2 = 175 MHz
 */
#define RTL931X_MAC_L2_GLOBAL_CTRL2		(0x1358)
#define RTL931X_SYS_CLK_SEL_M			GENMASK(14, 13)

/* Header + up to two VLAN tags + FCS, counted towards the frame limit */
#define RTL83XX_FRAME_OVERHEAD			(ETH_HLEN + 2 * VLAN_HLEN + ETH_FCS_LEN)

#define RTL838X_RST_GLB_CTRL_0			(0x003c)

#define RTL838X_MAC_FORCE_MODE_CTRL		(0xa104)
#define RTL839X_MAC_FORCE_MODE_CTRL		(0x02bc)
#define RTL930X_MAC_FORCE_MODE_CTRL		(0xCA1C)
#define RTL931X_MAC_FORCE_MODE_CTRL		(0x0DCC)

/*
 * Selects where the switch obtains a port's link ability from: 0 polls an
 * external PHY over MDIO, 2 takes it from the SerDes. Two bits per port.
 */
#define RTL931X_SMI_PHY_ABLTY_GET_SEL(port)	(0x0CAC + (((port) >> 4) << 2))
#define RTL931X_SMI_PHY_ABLTY_GET_SEL_SHIFT(port) (((port) & 0xf) << 1)
#define RTL931X_SMI_PHY_ABLTY_GET_SEL_SERDES	2

#define RTL838X_PORT_ISO_CTRL(port)		(0x4100 + ((port) << 2))
#define RTL839X_PORT_ISO_CTRL(port)		(0x1400 + ((port) << 3))

/* Packet statistics */
#define RTL838X_STAT_PORT_STD_MIB		(0x1200)
#define RTL839X_STAT_PORT_STD_MIB		(0xC000)
#define RTL930X_STAT_PORT_MIB_CNTR		(0x0664)
#define RTL930X_STAT_PORT_PRVTE_CNTR		(0x2364)
#define RTL838X_STAT_RST			(0x3100)
#define RTL839X_STAT_RST			(0xF504)
#define RTL930X_STAT_RST			(0x3240)
#define RTL931X_STAT_RST			(0x7ef4)
#define RTL838X_STAT_PORT_RST			(0x3104)
#define RTL839X_STAT_PORT_RST			(0xF508)
#define RTL930X_STAT_PORT_RST			(0x3244)
#define RTL931X_STAT_PORT_RST			(0x7ef8)
#define RTL838X_STAT_CTRL			(0x3108)
#define RTL839X_STAT_CTRL			(0x04cc)
#define RTL930X_STAT_CTRL			(0x3248)
#define RTL931X_STAT_CTRL			(0x5720)

/* Registers of the internal Serdes of the 8390 */
#define RTL8390_SDS0_1_XSG0			(0xA000)
#define RTL8390_SDS0_1_XSG1			(0xA100)
#define RTL839X_SDS12_13_XSG0			(0xB800)
#define RTL839X_SDS12_13_XSG1			(0xB900)
#define RTL839X_SDS12_13_PWR0			(0xb880)
#define RTL839X_SDS12_13_PWR1			(0xb980)

/* VLAN registers */
#define RTL838X_VLAN_CTRL			(0x3A74)
#define RTL838X_VLAN_PROFILE(idx)		(0x3A88 + ((idx) << 2))
#define RTL838X_VLAN_PORT_EGR_FLTR		(0x3A84)
#define RTL838X_VLAN_PORT_PB_VLAN		(0x3C00)
#define RTL838X_VLAN_PORT_IGR_FLTR		(0x3A7C)

#define RTL839X_VLAN_PROFILE(idx)		(0x25C0 + (((idx) << 3)))
#define RTL839X_VLAN_CTRL			(0x26D4)
#define RTL839X_VLAN_PORT_PB_VLAN		(0x26D8)
#define RTL839X_VLAN_PORT_IGR_FLTR		(0x27B4)
#define RTL839X_VLAN_PORT_EGR_FLTR		(0x27C4)

#define RTL930X_VLAN_PROFILE_SET(idx)		(0x9c60 + (((idx) * 20)))
#define RTL930X_VLAN_TAG_TPID_CTRL(idx)		(0xc7ac + ((idx) << 2))
#define RTL930X_VLAN_PORT_ITAG_TPID_CMP_MSK(port) \
	(0x327c + ((port) << 6))
#define RTL930X_VLAN_PORT_OTAG_TPID_CMP_MSK(port) \
	(0x3280 + ((port) << 6))
#define RTL930X_VLAN_PORT_AFT(port)		(0x8260 + ((port) << 2))
#define RTL930X_VLAN_CTRL			(0x82D4)
#define RTL930X_VLAN_PORT_PB_VLAN		(0x82D8)
#define RTL930X_VLAN_PORT_IGR_FLTR		(0x83C0)
#define RTL930X_VLAN_PORT_EGR_FLTR		(0x83C8)
#define RTL930X_VLAN_PORT_EGR_TPID_CTRL(port)	(0xce98 + ((port) << 2))

#define RTL930X_VLAN_PORT_AFT_ACCEPT_ALL	GENMASK(3, 0)
#define RTL930X_VLAN_PORT_EGR_TPID_OTPID_IDX	GENMASK(5, 4)
#define RTL930X_VLAN_PORT_EGR_TPID_OTPID_KEEP	BIT(3)

#define RTL931X_VLAN_PROFILE_SET(idx)		(0x9800 + (((idx) * 28)))
#define RTL931X_VLAN_TAG_TPID_CTRL(idx)		(0x13fc + ((idx) << 2))
#define RTL931X_VLAN_PORT_OTAG_TPID_CMP_MSK(port) \
	(0x600c + ((port) << 7))
#define RTL931X_VLAN_PORT_ITAG_TPID_CMP_MSK(port) \
	(0x6010 + ((port) << 7))
#define RTL931X_VLAN_CTRL			(0x94E4)
#define RTL931X_VLAN_PORT_IGR_CTRL		(0x94E8)
#define RTL931X_VLAN_PORT_IGR_FLTR		(0x96B4)
#define RTL931X_VLAN_PORT_EGR_FLTR		(0x96C4)
#define RTL931X_PKT_ENCAP_MISC_CTRL		(0x4fcc)

#define RTL931X_PKT_ENCAP_MISC_CTRL_EVC_TCAM_EN	BIT(0)

/* Table access registers */
#define RTL838X_TBL_ACCESS_CTRL_0		(0x6914)
#define RTL838X_TBL_ACCESS_DATA_0(idx)		(0x6918 + ((idx) << 2))
#define RTL838X_TBL_ACCESS_CTRL_1		(0xA4C8)
#define RTL838X_TBL_ACCESS_DATA_1(idx)		(0xA4CC + ((idx) << 2))

#define RTL839X_TBL_ACCESS_CTRL_0		(0x1190)
#define RTL839X_TBL_ACCESS_DATA_0(idx)		(0x1194 + ((idx) << 2))
#define RTL839X_TBL_ACCESS_CTRL_1		(0x6b80)
#define RTL839X_TBL_ACCESS_DATA_1(idx)		(0x6b84 + ((idx) << 2))
#define RTL839X_TBL_ACCESS_CTRL_2		(0x611C)
#define RTL839X_TBL_ACCESS_DATA_2(i)		(0x6120 + (((i) << 2)))

#define RTL930X_TBL_ACCESS_CTRL_0		(0xB340)
#define RTL930X_TBL_ACCESS_DATA_0(idx)		(0xB344 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_CTRL_1		(0xB3A0)
#define RTL930X_TBL_ACCESS_DATA_1(idx)		(0xB3A4 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_CTRL_2		(0xCE04)
#define RTL930X_TBL_ACCESS_DATA_2(i)		(0xCE08 + (((i) << 2)))

#define RTL931X_TBL_ACCESS_CTRL_0		(0x8500)
#define RTL931X_TBL_ACCESS_DATA_0(idx)		(0x8508 + ((idx) << 2))
#define RTL931X_TBL_ACCESS_CTRL_1		(0x40C0)
#define RTL931X_TBL_ACCESS_DATA_1(idx)		(0x40C4 + ((idx) << 2))
#define RTL931X_TBL_ACCESS_CTRL_2		(0x8528)
#define RTL931X_TBL_ACCESS_DATA_2(i)		(0x852C + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_3		(0x0200)
#define RTL931X_TBL_ACCESS_DATA_3(i)		(0x0204 + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_4		(0x20DC)
#define RTL931X_TBL_ACCESS_DATA_4(i)		(0x20E0 + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_5		(0x7E1C)
#define RTL931X_TBL_ACCESS_DATA_5(i)		(0x7E20 + (((i) << 2)))

/* MAC handling */
#define RTL838X_MAC_LINK_STS			(0xa188)
#define RTL839X_MAC_LINK_STS			(0x0390)
#define RTL930X_MAC_LINK_STS			(0xCB10)
#define RTL931X_MAC_LINK_STS			(0x0EC0)

/* MAC link state bits */
#define RTL_SPEED_10				0
#define RTL_SPEED_100				1
#define RTL_SPEED_1000				2
#define RTL_SPEED_2500				5
#define RTL_SPEED_5000				6
#define RTL_SPEED_10000				4

#define RTL83XX_FORCE_EN			BIT(0)
#define RTL83XX_FORCE_LINK_EN			BIT(1)

#define RTL838X_NWAY_EN				BIT(2)
#define RTL838X_DUPLEX_MODE			BIT(3)
#define RTL838X_SPEED_SHIFT			(4)
#define RTL838X_SPEED_MASK			(3 << RTL838X_SPEED_SHIFT)
#define RTL838X_TX_PAUSE_EN			BIT(6)
#define RTL838X_RX_PAUSE_EN			BIT(7)
#define RTL838X_MAC_FORCE_FC_EN			BIT(8)

#define RTL839X_DUPLEX_MODE			BIT(2)
#define RTL839X_SPEED_SHIFT			(3)
#define RTL839X_SPEED_MASK			(3 << RTL839X_SPEED_SHIFT)
#define RTL839X_TX_PAUSE_EN			BIT(5)
#define RTL839X_RX_PAUSE_EN			BIT(6)
#define RTL839X_MAC_FORCE_FC_EN			BIT(7)

#define RTL930X_FORCE_EN			BIT(0)
#define RTL930X_FORCE_LINK_EN			BIT(1)
#define RTL930X_DUPLEX_MODE			BIT(2)
#define RTL930X_SPEED_SHIFT			(3)
#define RTL930X_SPEED_MASK			(15 << RTL930X_SPEED_SHIFT)
#define RTL930X_TX_PAUSE_EN			BIT(7)
#define RTL930X_RX_PAUSE_EN			BIT(8)
#define RTL930X_MAC_FORCE_FC_EN			BIT(9)

#define RTL931X_FORCE_EN			BIT(9)
/*
 * MAC_FORCE_MODE_CTRL field layout for RTL931x, per the vendor SDK
 * (swcore_rtl9310.h RTL9310_MAC_FORCE_MODE_CTRL_SMI_*): the speed select is
 * 4 bits at 12, duplex select at 11, link value at 9, and separate
 * force-enable bits for speed/duplex/link at 3/2/0.
 */
#define RTL931X_SPEED_SHIFT			(12)
#define RTL931X_SPEED_MASK			(0xf << RTL931X_SPEED_SHIFT)
#define RTL931X_DUP_SEL				BIT(11)
#define RTL931X_FORCE_LINK			BIT(9)
#define RTL931X_FORCE_SPD_EN			BIT(3)
#define RTL931X_FORCE_DUP_EN			BIT(2)
#define RTL931X_FORCE_LINK_EN			BIT(0)
#define RTL931X_DUPLEX_MODE			BIT(2)
#define RTL931X_MAC_FORCE_FC_EN			BIT(4)
#define RTL931X_TX_PAUSE_EN			BIT(16)
#define RTL931X_RX_PAUSE_EN			BIT(17)

/* EEE */
#define RTL838X_MAC_EEE_ABLTY			(0xa1a8)
#define RTL838X_EEE_PORT_TX_EN			(0x014c)
#define RTL838X_EEE_PORT_RX_EN			(0x0150)
#define RTL838X_EEE_CLK_STOP_CTRL		(0x0148)
#define RTL838X_EEE_TX_TIMER_GIGA_CTRL		(0xaa04)
#define RTL838X_EEE_TX_TIMER_GELITE_CTRL	(0xaa08)

#define RTL839X_EEE_TX_TIMER_GELITE_CTRL	(0x042C)
#define RTL839X_EEE_TX_TIMER_GIGA_CTRL		(0x0430)
#define RTL839X_EEE_TX_TIMER_10G_CTRL		(0x0434)
#define RTL839X_EEE_CTRL(p)			(0x8008 + ((p) << 7))
#define RTL839X_MAC_EEE_ABLTY			(0x03C8)

#define RTL930X_MAC_EEE_ABLTY			(0xCB34)
#define RTL930X_EEE_CTRL(p)			(0x3274 + ((p) << 6))
#define RTL930X_EEEP_PORT_CTRL(p)		(0x3278 + ((p) << 6))

#define RTL931X_MAC_EEE_ABLTY			(0x0f08)

/* L2 functionality */
#define RTL838X_L2_CTRL_0			(0x3200)
#define RTL839X_L2_CTRL_0			(0x3800)
#define RTL930X_L2_CTRL				(0x8FD8)
#define RTL931X_L2_CTRL				(0xC800)
#define RTL838X_L2_CTRL_1			(0x3204)
#define RTL839X_L2_CTRL_1			(0x3804)
#define RTL930X_L2_AGE_CTRL			(0x8FDC)
#define RTL931X_L2_AGE_CTRL			(0xC804)
#define RTL838X_L2_PORT_AGING_OUT		(0x3358)
#define RTL839X_L2_PORT_AGING_OUT		(0x3b74)
#define	RTL930X_L2_PORT_AGE_CTRL		(0x8FE0)
#define	RTL931X_L2_PORT_AGE_CTRL		(0xc808)
#define RTL838X_TBL_ACCESS_L2_CTRL		(0x6900)
#define RTL839X_TBL_ACCESS_L2_CTRL		(0x1180)
#define RTL930X_TBL_ACCESS_L2_CTRL		(0xB320)
#define RTL930X_TBL_ACCESS_L2_METHOD_CTRL	(0xB324)
#define RTL838X_TBL_ACCESS_L2_DATA(idx)		(0x6908 + ((idx) << 2))
#define RTL839X_TBL_ACCESS_L2_DATA(idx)		(0x1184 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_L2_DATA(idx)		(0xab08 + ((idx) << 2))

#define RTL838X_L2_TBL_FLUSH_CTRL		(0x3370)
#define RTL839X_L2_TBL_FLUSH_CTRL		(0x3ba0)
#define RTL930X_L2_TBL_FLUSH_CTRL		(0x9404)
#define RTL931X_L2_TBL_FLUSH_CTRL		(0xCD9C)

#define RTL838X_L2_LRN_CONSTRT			(0x329C)
#define RTL839X_L2_LRN_CONSTRT			(0x3910)
#define RTL930X_L2_LRN_CONSTRT_CTRL		(0x909c)
#define RTL931X_L2_LRN_CONSTRT_CTRL		(0xC964)

#define RTL838X_L2_FLD_PMSK			(0x3288)
#define RTL839X_L2_FLD_PMSK			(0x38EC)
#define RTL930X_L2_BC_FLD_PMSK			(0x9068)
#define RTL931X_L2_BC_FLD_PMSK			(0xC8FC)

#define RTL930X_L2_UNKN_UC_FLD_PMSK		(0x9064)
#define RTL931X_L2_UNKN_UC_FLD_PMSK		(0xC8F4)

#define RTL838X_L2_LRN_CONSTRT_EN		(0x3368)
#define RTL838X_L2_PORT_LRN_CONSTRT		(0x32A0)
#define RTL839X_L2_PORT_LRN_CONSTRT		(0x3914)
#define RTL930X_L2_LRN_PORT_CONSTRT_CTRL	(0x90A4)
#define RTL930X_L2_LRN_TRK_CONSTRT_CTRL		(0x918C)
#define RTL931X_L2_LRN_PORT_CONSTRT_CTRL	(0xC96C)

#define RTL838X_L2_PORT_NEW_SALRN(p)		(0x328c + (((p >> 4) << 2)))
#define RTL839X_L2_PORT_NEW_SALRN(p)		(0x38F0 + (((p >> 4) << 2)))
#define RTL930X_L2_PORT_SALRN(p)		(0x8FEC + (((p >> 4) << 2)))
#define RTL931X_L2_PORT_NEW_SALRN(p)		(0xC820 + (((p >> 4) << 2)))

#define SALRN_PORT_SHIFT(p)			((p % 16) * 2)
#define SALRN_MODE_MASK				0x3
#define SALRN_MODE_HARDWARE			0
#define SALRN_MODE_DISABLED			2

#define RTL838X_L2_PORT_NEW_SA_FWD(p)		(0x3294 + (((p >> 4) << 2)))
#define RTL839X_L2_PORT_NEW_SA_FWD(p)		(0x3900 + (((p >> 4) << 2)))
#define RTL930X_L2_PORT_NEW_SA_FWD(p)		(0x8FF4 + (((p / 10) << 2)))
#define RTL931X_L2_PORT_NEW_SA_FWD(p)		(0xC830 + (((p / 10) << 2)))

#define RTL838X_L2_PORT_MV_ACT(p)		(0x335c + (((p >> 4) << 2)))
#define RTL839X_L2_PORT_MV_ACT(p)		(0x3b80 + (((p >> 4) << 2)))

#define RTL838X_L2_PORT_STATIC_MV_ACT(p)	(0x327c + (((p >> 4) << 2)))
#define RTL839X_L2_PORT_STATIC_MV_ACT(p)	(0x38dc + (((p >> 4) << 2)))

#define MV_ACT_PORT_SHIFT(p)			((p % 16) * 2)
#define MV_ACT_MASK				0x3
#define MV_ACT_FORWARD				0
#define MV_ACT_DROP				1
#define MV_ACT_TRAP2CPU				2
#define MV_ACT_COPY2CPU				3

#define RTL930X_ST_CTRL				(0x8798)
#define RTL931x_ST_CTRL				(0x8000)

#define RTL930X_L2_PORT_SABLK_CTRL		(0x905c)
#define RTL930X_L2_PORT_DABLK_CTRL		(0x9060)

#define RTL838X_L2_PORT_LM_ACT(p)		(0x3208 + ((p) << 2))
#define RTL838X_VLAN_PORT_FWD			(0x3A78)
#define RTL839X_VLAN_PORT_FWD			(0x27AC)
#define RTL930X_VLAN_PORT_FWD			(0x834C)
#define RTL931X_VLAN_PORT_FWD			(0x95CC)
#define RTL838X_VLAN_FID_CTRL			(0x3aa8)

/* Port Mirroring */
#define RTL838X_MIR_CTRL			(0x5D00)
#define RTL838X_MIR_DPM_CTRL			(0x5D20)
#define RTL838X_MIR_SPM_CTRL			(0x5D10)

#define RTL839X_MIR_CTRL			(0x2500)
#define RTL839X_MIR_DPM_CTRL			(0x2530)
#define RTL839X_MIR_SPM_CTRL			(0x2510)

#define RTL930X_MIR_CTRL			(0xA2A0)
#define RTL930X_MIR_DPM_CTRL			(0xA2C0)
#define RTL930X_MIR_SPM_CTRL			(0xA2B0)

/* Port-based packet sampling (sFlow): one 32-bit register per port, the
 * low half holding the ingress rate and the high half the egress rate.
 * A rate of N samples one in N packets, 0 disables sampling; there is no
 * separate enable bit. Sampled packets are copied to the CPU, forwarding
 * of the original is unaffected (SDK dal_longan_mirror_sflowPort{Igr,Egr}
 * SampleRate_set). The global control register selects the sample copy
 * target (local vs master CPU) and which copy to keep when a packet is
 * both ingress- and egress-sampled (SDK dal_longan_mirror_sflowSample
 * {Target,Ctrl}_set).
 */
#define RTL930X_SFLOW_CTRL			(0xBEA0)
#define RTL930X_SFLOW_CTRL_SMPL_SEL		BIT(0)
#define RTL930X_SFLOW_CTRL_CPU_SEL		BIT(1)
#define RTL930X_SFLOW_PORT_RATE_CTRL(p)		(0xBEA4 + (((p) << 2)))
#define RTL930X_SFLOW_IGR_RATE_MASK		GENMASK(15, 0)
#define RTL930X_SFLOW_EGR_RATE_MASK		GENMASK(31, 16)
#define RTL930X_SFLOW_RATE_MAX			(0xffff)

#define RTL931X_MIR_CTRL			(0xAF00)
#define RTL931X_MIR_DPM_CTRL			(0xAF30)
#define RTL931X_MIR_SPM_CTRL			(0xAF10)

/* Port-based packet sampling (sFlow) on RTL931x: identical field layout to
 * RTL930x (one 32-bit register per port, ingress rate in the low half,
 * egress rate in the high half; a rate of N samples one in N packets, 0
 * disables, no separate enable bit), at different addresses, ports 0-55
 * (SDK swcore_rtl9310.h RTL9310_SFLOW_CTRL_ADDR /
 * RTL9310_SFLOW_PORT_RATE_CTRL_ADDR, fields SMPL_SEL/CPU_SEL/IGR_RATE/
 * EGR_RATE at the same bit positions as RTL930X_SFLOW_*;
 * dal_mango_mirror_sflowPort{Igr,Egr}SampleRate_set).
 *
 * Port-based sampling consumes NONE of the 4 mirror sessions on this
 * family: the DAL setters touch only these two registers, sample copies
 * reach the CPU marked with the dedicated SFLOW field of the CPU tag (not
 * MIR_HIT, SDK nic_rtl9310.h), and their CPU queue comes from a flag-based
 * mapping (QM_FLAG2CPUQID_CTRL_2.SFLOW). The TRAP_Q_SFLOW rejection in
 * dal_mango_trap.c means only that sFlow is not a trap *reason* and
 * therefore cannot be remapped to a CPU queue through the reason-based
 * trap API - it is not a mirror session. What does consume a mirror
 * session is the separate mirror-based sFlow feature
 * (MIR_SAMPLE_RATE_CTRL, sampling of already-mirrored traffic), which this
 * driver does not use.
 */
#define RTL931X_SFLOW_CTRL			(0x8400)
#define RTL931X_SFLOW_PORT_RATE_CTRL(p)		(0x8404 + (((p) << 2)))

/* Storm/rate control and scheduling */
#define RTL838X_STORM_CTRL			(0x4700)
#define RTL839X_STORM_CTRL			(0x1800)
#define RTL838X_STORM_CTRL_LB_CTRL(p)		(0x4884 + (((p) << 2)))
#define RTL838X_STORM_CTRL_BURST_PPS_0		(0x4874)
#define RTL838X_STORM_CTRL_BURST_PPS_1		(0x4878)
#define RTL838X_STORM_CTRL_BURST_0		(0x487c)
#define RTL838X_STORM_CTRL_BURST_1		(0x4880)
#define RTL839X_STORM_CTRL_LB_TICK_TKN_CTRL_0	(0x1804)
#define RTL839X_STORM_CTRL_LB_TICK_TKN_CTRL_1	(0x1808)
#define RTL838X_SCHED_CTRL			(0xB980)
#define RTL839X_SCHED_CTRL			(0x60F4)
#define RTL838X_SCHED_LB_TICK_TKN_CTRL_0	(0xAD58)
#define RTL838X_SCHED_LB_TICK_TKN_CTRL_1	(0xAD5C)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL_0	(0x1804)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL_1	(0x1808)
#define RTL839X_STORM_CTRL_SPCL_LB_TICK_TKN_CTRL (0x2000)
#define RTL839X_IGR_BWCTRL_LB_TICK_TKN_CTRL_0	(0x1604)
#define RTL839X_IGR_BWCTRL_LB_TICK_TKN_CTRL_1	(0x1608)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL		(0x60F8)
#define RTL839X_SCHED_LB_TICK_TKN_PPS_CTRL	(0x6200)
#define RTL838X_SCHED_LB_THR			(0xB984)
#define RTL839X_SCHED_LB_THR			(0x60FC)
#define RTL838X_SCHED_P_EGR_RATE_CTRL(p)	(0xC008 + (((p) << 7)))
#define RTL838X_SCHED_Q_EGR_RATE_CTRL(p, q)	(0xC00C + (p << 7) + (((q) << 2)))
#define RTL838X_STORM_CTRL_PORT_BC_EXCEED	(0x470C)
#define RTL838X_STORM_CTRL_PORT_MC_EXCEED	(0x4710)
#define RTL838X_STORM_CTRL_PORT_UC_EXCEED	(0x4714)
#define RTL839X_STORM_CTRL_PORT_BC_EXCEED(p)	(0x180c + (((p >> 5) << 2)))
#define RTL839X_STORM_CTRL_PORT_MC_EXCEED(p)	(0x1814 + (((p >> 5) << 2)))
#define RTL839X_STORM_CTRL_PORT_UC_EXCEED(p)	(0x181c + (((p >> 5) << 2)))
#define RTL838X_STORM_CTRL_PORT_UC(p)		(0x4718 + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_MC(p)		(0x478c + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_BC(p)		(0x4800 + (((p) << 2)))
#define RTL839X_STORM_CTRL_PORT_UC_0(p)		(0x185C + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_UC_1(p)		(0x1860 + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_MC_0(p)		(0x19FC + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_MC_1(p)		(0x1a00 + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_BC_0(p)		(0x1B9C + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_BC_1(p)		(0x1BA0 + (((p) << 3)))
#define RTL930X_STORM_CTRL			(0x8A60)
#define RTL930X_STORM_LB_CTRL			(0x8A64)
#define RTL930X_STORM_LB_PPS_CTRL		(0x8A68)
#define RTL930X_STORM_PORT_CTRL			(0x8A6C)
/* Per-port 64-bit entries: word 0 = rate/enable/type, word 1 = burst */
#define RTL930X_STORM_PORT_UC_CTRL(p)		(0x8A70 + (((p) << 3)))
#define RTL930X_STORM_PORT_MC_CTRL(p)		(0x8B60 + (((p) << 3)))
#define RTL930X_STORM_PORT_BC_CTRL(p)		(0x8C50 + (((p) << 3)))
#define RTL930X_STORM_PORT_UC_LB_RST		(0x8B58)
#define RTL930X_STORM_PORT_MC_LB_RST		(0x8C48)
#define RTL930X_STORM_PORT_BC_LB_RST		(0x8D38)
#define RTL930X_STORM_PORT_UC_EXCEED		(0x8B5C)
#define RTL930X_STORM_PORT_MC_EXCEED		(0x8C4C)
#define RTL930X_STORM_PORT_BC_EXCEED		(0x8D3C)
#define RTL930X_STORM_RATE_M			GENMASK(23, 0)
#define RTL930X_STORM_EN			BIT(24)
#define RTL930X_STORM_TYPE_INCL_KNOWN		BIT(25)
#define RTL930X_STORM_BURST_M			GENMASK(15, 0)
#define RTL930X_STORM_DFLT_BURST_PPS		(255)

/* RTL931X (Mango) storm control (SDK swcore_rtl9310.h, dal_mango_rate.c).
 * Same leaky-bucket shape as RTL930x at new addresses. The per-port 64-bit
 * entries hold the high word at the lower address: RATE/EN/TYPE at base + 0,
 * BURST at base + 4. The pps/bps mode select, the leaky-bucket resets and
 * the exceed flags are one bit per port, 32 ports per word on this 57-port
 * family. TYPE selects unknown-DA-only (0) vs all traffic of the class (1)
 * and exists only for UC and MC.
 */
#define RTL931X_STORM_CTRL			(0xB000)
#define RTL931X_STORM_LB_CTRL			(0xB004)
#define RTL931X_STORM_LB_PPS_CTRL		(0xB008)
#define RTL931X_STORM_LB_PPS_TICK_M		GENMASK(15, 4)
#define RTL931X_STORM_LB_PPS_TKN_M		GENMASK(3, 0)
#define RTL931X_STORM_PORT_CTRL(p)		(0xB00C + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_UC_CTRL(p)		(0xB014 + (((p) << 3)))
#define RTL931X_STORM_PORT_MC_CTRL(p)		(0xB1EC + (((p) << 3)))
#define RTL931X_STORM_PORT_BC_CTRL(p)		(0xB3C4 + (((p) << 3)))
#define RTL931X_STORM_PORT_UC_LB_RST(p)		(0xB1DC + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_MC_LB_RST(p)		(0xB3B4 + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_BC_LB_RST(p)		(0xB58C + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_UC_EXCEED(p)		(0xB1E4 + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_MC_EXCEED(p)		(0xB3BC + (((p) >> 5) << 2))
#define RTL931X_STORM_PORT_BC_EXCEED(p)		(0xB594 + (((p) >> 5) << 2))
#define RTL931X_STORM_RATE_M			GENMASK(23, 0)
#define RTL931X_STORM_EN			BIT(24)
#define RTL931X_STORM_TYPE_INCL_KNOWN		BIT(25)
#define RTL931X_STORM_BURST_M			GENMASK(15, 0)
/* Measured silicon reset posture: rate wide open (RATE_M), burst 0x8000 */
#define RTL931X_STORM_RESET_BURST		(0x8000)
/* SDK default burst in packet mode (MANGO_STORM_PPS_DFLT_BURST_SIZE) */
#define RTL931X_STORM_DFLT_BURST_PPS		(255)
/* PPS leaky-bucket tick/token per system clock (SDK dal_mango_construct.h);
 * with these values one RATE unit is exactly 1 pps
 */
#define RTL931X_STORM_LB_PPS_TICK_650M		(39)
#define RTL931X_STORM_LB_PPS_TKN_650M		(1)
#define RTL931X_STORM_LB_PPS_TICK_325M		(77)
#define RTL931X_STORM_LB_PPS_TKN_325M		(1)
#define RTL931X_STORM_LB_PPS_TICK_175M		(42)
#define RTL931X_STORM_LB_PPS_TKN_175M		(1)

/* Storm control traffic classes */
enum rtldsa_storm_class {
	RTLDSA_STORM_UC = 0,
	RTLDSA_STORM_MC,
	RTLDSA_STORM_BC,
};
#define RTL839X_TBL_ACCESS_CTRL_2		(0x611C)
#define RTL839X_TBL_ACCESS_DATA_2(i)		(0x6120 + (((i) << 2)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_10G_0(p)	(0x1618 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_10G_1(p)	(0x161C + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_0(p)	(0x1640 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_1(p)	(0x1644 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_CTRL_LB_THR		(0x1614)

/* Link aggregation (Trunking) */
#define TRUNK_DISTRIBUTION_ALGO_SPA_BIT		0x01
#define TRUNK_DISTRIBUTION_ALGO_SMAC_BIT	0x02
#define TRUNK_DISTRIBUTION_ALGO_DMAC_BIT	0x04
#define TRUNK_DISTRIBUTION_ALGO_SIP_BIT		0x08
#define TRUNK_DISTRIBUTION_ALGO_DIP_BIT		0x10
#define TRUNK_DISTRIBUTION_ALGO_SRC_L4PORT_BIT	0x20
#define TRUNK_DISTRIBUTION_ALGO_DST_L4PORT_BIT	0x40
#define TRUNK_DISTRIBUTION_ALGO_MASKALL		0x7F

#define TRUNK_DISTRIBUTION_ALGO_L2_SPA_BIT	0x01
#define TRUNK_DISTRIBUTION_ALGO_L2_SMAC_BIT	0x02
#define TRUNK_DISTRIBUTION_ALGO_L2_DMAC_BIT	0x04
#define TRUNK_DISTRIBUTION_ALGO_L2_VLAN_BIT	0x08
#define TRUNK_DISTRIBUTION_ALGO_L2_MASKALL	0xF

#define TRUNK_DISTRIBUTION_ALGO_L3_SPA_BIT	0x01
#define TRUNK_DISTRIBUTION_ALGO_L3_SMAC_BIT	0x02
#define TRUNK_DISTRIBUTION_ALGO_L3_DMAC_BIT	0x04
#define TRUNK_DISTRIBUTION_ALGO_L3_VLAN_BIT	0x08
#define TRUNK_DISTRIBUTION_ALGO_L3_SIP_BIT	0x10
#define TRUNK_DISTRIBUTION_ALGO_L3_DIP_BIT	0x20
#define TRUNK_DISTRIBUTION_ALGO_L3_SRC_L4PORT_BIT  0x40
#define TRUNK_DISTRIBUTION_ALGO_L3_DST_L4PORT_BIT  0x80
#define TRUNK_DISTRIBUTION_ALGO_L3_PROTO_BIT	0x100
#define TRUNK_DISTRIBUTION_ALGO_L3_FLOW_LABEL_BIT  0x200
#define TRUNK_DISTRIBUTION_ALGO_L3_MASKALL	0x3FF

#define RTL838X_TRK_MBR_CTR			(0x3E00)
#define RTL838X_TRK_HASH_IDX_CTRL		(0x3E20)
#define RTL838X_TRK_HASH_CTRL			(0x3E24)

#define RTL839X_TRK_MBR_CTR			(0x2200)
#define RTL839X_TRK_HASH_IDX_CTRL		(0x2280)
#define RTL839X_TRK_HASH_CTRL			(0x2284)

#define RTL930X_TRK_MBR_CTRL			(0xA41C)
#define RTL930X_TRK_HASH_CTRL			(0x9F80)
#define RTL930X_TRK_LOCAL_TBL_REFRESH		(0x9F90)
#define RTL930X_TRK_ID_CTRL			(0xA3A8)
#define RTL930X_LOCAL_PORT_TRK_MAP		(0xD0C8)
#define RTL930X_TRK_CTRL			(0x9F88)
#define RTL930X_TRK_SHFT_CTRL			(0x9F8C)
#define RTL930X_TRK_LOCAL_TBL			(0x9F94)
#define RTL930X_STK_GLB_CTRL			(0xA498)

/* RTL9310 trunk registers, per swcore_rtl9310.h */
#define RTL931X_TRK_MBR_CTRL			(0xB8D0)	/* TRK_MBR_CTRL, indexed by local slot, 2 words each */
#define RTL931X_TRK_HASH_CTRL			(0xBA70)	/* TRK_HASH_CTRL, 2 hash mask sets */
#define RTL931X_TRK_CTRL			(0xBA78)	/* TRK_CTRL */
#define RTL931X_TRK_SHFT_CTRL			(0xBA7C)	/* TRK_SHFT_CTRL */
#define RTL931X_TRK_LOCAL_TBL_REFRESH		(0xBA80)	/* TRK_LOCAL_TBL_REFRESH */
#define RTL931X_TRK_ID_CTRL			(0xB800)	/* TRK_ID_CTRL, indexed by local slot 0-51 */
#define RTL931X_LOCAL_PORT_TRK_MAP		(0x4CAC)	/* LOCAL_PORT_TRK_MAP, per port */
#define RTL931X_L2_LRN_TRK_CONSTRT_CTRL		(0xCB34)	/* L2_LRN_TRK_CONSTRT_CTRL, per trunk gid */
#define RTL931X_STK_GBL_CTRL			(0x1448)	/* STK_GBL_CTRL, MY_DEV_ID at bits 4-7 */

/* Attack prevention */
#define RTL838X_ATK_PRVNT_PORT_EN		(0x5B00)
#define RTL838X_ATK_PRVNT_CTRL			(0x5B04)
#define RTL838X_ATK_PRVNT_ACT			(0x5B08)
#define RTL838X_ATK_PRVNT_STS			(0x5B1C)

/* 802.1X */
#define RTL838X_RMA_BPDU_FLD_PMSK		(0x4348)
#define RTL930X_RMA_BPDU_FLD_PMSK		(0x9F18)
#define RTL931X_RMA_BPDU_FLD_PMSK		(0x8950)
#define RTL839X_RMA_BPDU_FLD_PMSK		(0x125C)

#define RTL838X_SPCL_TRAP_CTRL			(0x6980)
#define RTL838X_SPCL_TRAP_EAPOL_CTRL		(0x6988)
#define RTL838X_SPCL_TRAP_ARP_CTRL		(0x698C)
#define RTL838X_SPCL_TRAP_IGMP_CTRL		(0x6984)
#define RTL838X_SPCL_TRAP_IPV6_CTRL		(0x6994)
#define RTL838X_SPCL_TRAP_SWITCH_MAC_CTRL	(0x6998)

#define RTL839X_SPCL_TRAP_CTRL			(0x1054)
#define RTL839X_SPCL_TRAP_EAPOL_CTRL		(0x105C)
#define RTL839X_SPCL_TRAP_ARP_CTRL		(0x1060)
#define RTL839X_SPCL_TRAP_IGMP_CTRL		(0x1058)
#define RTL839X_SPCL_TRAP_IPV6_CTRL		(0x1064)
#define RTL839X_SPCL_TRAP_SWITCH_MAC_CTRL	(0x1068)
#define RTL839X_SPCL_TRAP_SWITCH_IPV4_ADDR_CTRL	(0x106C)
#define RTL839X_SPCL_TRAP_CRC_CTRL		(0x1070)

/* The egress policer shares RTL930X_EGBW_PORT_CTRL() with the root TBF. */
#define RTL930X_BANDWIDTH_CTRL_INGRESS(port)	(0x8068 + (port * 4))
#define RTL930X_BANDWIDTH_CTRL_MAX_BURST	(64 * 1000)
#define RTL930X_BANDWIDTH_CTRL_INGRESS_BURST_HIGH_ON(port) \
						(0x80DC + (port * 8))
#define RTL930X_BANDWIDTH_CTRL_INGRESS_BURST_HIGH_OFF(port) \
						(0x80E0 + (port * 8))
#define RTL930X_BANDWIDTH_CTRL_INGRESS_BURST_MAX \
						GENMASK(30, 0)

/* The egress policer shares RTL931X_EGBW_PORT_CTRL() with the root TBF. */
#define RTL931X_BANDWIDTH_CTRL_INGRESS(port)	(0xe008 + (port * 8))

#define RTL93XX_BANDWIDTH_CTRL_RATE_MAX		GENMASK(19, 0)
#define RTL93XX_BANDWIDTH_CTRL_ENABLE		BIT(20)
#define RTL931X_BANDWIDTH_CTRL_MAX_BURST	GENMASK(15, 0)

#define RTL930X_INGRESS_FC_CTRL(port)		(0x81CC + ((port / 29) * 4))
#define RTL930X_INGRESS_FC_CTRL_EN(port)	BIT(port % 29)

/* special port action controls */
/* values:
 *      0 = FORWARD (default)
 *      1 = DROP
 *      2 = TRAP2CPU
 *      3 = FLOOD IN ALL PORT
 *
 *      Register encoding.
 *      offset = CTRL + (port >> 4) << 2
 *      value/mask = 3 << ((port & 0xF) << 1)
 */

typedef enum {
	BPDU = 0,
	PTP,
	PTP_UDP,
	PTP_ETH2,
	LLDP,
	EAPOL,
	GRATARP,
} rma_ctrl_t;

typedef enum {
	FORWARD = 0,
	DROP,
	TRAP2CPU,
	FLOODALL,
	TRAP2MASTERCPU,
	COPY2CPU,
} action_type_t;

#define RTL838X_RMA_BPDU_CTRL			(0x4330)
#define RTL839X_RMA_BPDU_CTRL			(0x122C)
#define RTL930X_RMA_BPDU_CTRL			(0x9E7C)
#define RTL931X_RMA_BPDU_CTRL			(0x881C)

#define RTL838X_RMA_PTP_CTRL			(0x4338)
#define RTL839X_RMA_PTP_CTRL			(0x123C)
#define RTL930X_RMA_PTP_CTRL			(0x9E88)
#define RTL931X_RMA_PTP_CTRL			(0x8834)

#define RTL838X_RMA_LLDP_CTRL			(0x4340)
#define RTL839X_RMA_LLDP_CTRL			(0x124C)
#define RTL930X_RMA_LLDP_CTRL			(0x9EFC)
#define RTL931X_RMA_LLDP_CTRL			(0x8918)

#define RTL930X_RMA_EAPOL_CTRL			(0x9F08)
#define RTL930X_SPCL_TRAP_PORT_CTRL		(0xA1A0)
#define RTL931X_RMA_EAPOL_CTRL			(0x8930)
#define RTL931X_TRAP_ARP_GRAT_PORT_ACT		(0x8C04)

/* QoS */
#define RTL838X_QM_INTPRI2QID_CTRL		(0x5F00)
#define RTL839X_QM_INTPRI2QID_CTRL(q)		(0x1110 + (q << 2))
#define RTL839X_QM_PORT_QNUM(p)			(0x1130 + (((p / 10) << 2)))
#define RTL838X_PRI_SEL_PORT_PRI(p)		(0x5FB8 + (((p / 10) << 2)))
#define RTL839X_PRI_SEL_PORT_PRI(p)		(0x10A8 + (((p / 10) << 2)))
#define RTL930X_PRI_SEL_PORT_PRI(p)		(0x9AE8 + (((p / 10) << 2)))
#define RTL838X_QM_PKT2CPU_INTPRI_MAP		(0x5F10)
#define RTL839X_QM_PKT2CPU_INTPRI_MAP		(0x1154)
#define RTL838X_PRI_SEL_CTRL			(0x10E0)
#define RTL839X_PRI_SEL_CTRL			(0x10E0)
#define RTL838X_PRI_SEL_TBL_CTRL(i)		(0x5FD8 + (((i) << 2)))
#define RTL839X_PRI_SEL_TBL_CTRL(i)		(0x10D0 + (((i) << 2)))
#define RTL838X_QM_PKT2CPU_INTPRI_0		(0x5F04)
#define RTL838X_QM_PKT2CPU_INTPRI_1		(0x5F08)
#define RTL838X_QM_PKT2CPU_INTPRI_2		(0x5F0C)
#define RTL839X_OAM_CTRL			(0x2100)
#define RTL839X_OAM_PORT_ACT_CTRL(p)		(0x2104 + (((p) << 2)))
#define RTL839X_RMK_PORT_DEI_TAG_CTRL(p)	(0x6A9C + (((p >> 5) << 2)))
#define RTL839X_PRI_SEL_IPRI_REMAP		(0x1080)
#define RTL838X_PRI_SEL_IPRI_REMAP		(0x5F8C)
#define RTL839X_PRI_SEL_DEI2DP_REMAP		(0x10EC)
#define RTL839X_PRI_SEL_DSCP2DP_REMAP_ADDR(i)	(0x10F0 + (((i >> 4) << 2)))
#define RTL839X_RMK_DEI_CTRL			(0x6AA4)
#define RTL839X_WRED_PORT_THR_CTRL(i)		(0x6084 + ((i) << 2))
#define RTL839X_WRED_QUEUE_THR_CTRL(q, i)	(0x6090 + ((q) * 12) + ((i) << 2))
#define RTL838X_PRI_DSCP_INVLD_CTRL0		(0x5FE8)
#define RTL838X_RMK_IPRI_CTRL			(0xA460)
#define RTL838X_RMK_OPRI_CTRL			(0xA464)
#define RTL838X_SCHED_P_TYPE_CTRL(p)		(0xC04C + (((p) << 7)))
#define RTL838X_SCHED_LB_CTRL(p)		(0xC004 + (((p) << 7)))
#define RTL838X_FC_P_EGR_DROP_CTRL(p)		(0x6B1C + (((p) << 2)))

#define RTL930X_REMAP_DSCP(p)			(0x9B04 + (((p) / 10) * 4))
#define RTL931X_REMAP_DSCP(p)			(0x9034 + (((p) / 10) * 4))
#define RTL93XX_REMAP_DSCP_INTPRI_DSCP_OFFSET(p) \
						(((p) % 10) * 3)
#define RTL93XX_REMAP_DSCP_INTPRI_DSCP_MASK(index) \
						(0x7 << RTL93XX_REMAP_DSCP_INTPRI_DSCP_OFFSET(index))

#define RTL930X_PORT_TBL_IDX_CTRL(port)		(0x9B20 + (((port) / 16) * 4))
#define RTL931X_PORT_TBL_IDX_CTRL(port)		(0x9064 + (((port) / 16) * 4))
#define RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port) \
						(((port) & 0xF) << 1)
#define RTL93XX_PORT_TBL_IDX_CTRL_IDX_MASK(port) \
						(0x3 << RTL93XX_PORT_TBL_IDX_CTRL_IDX_OFFSET(port))

#define RTL93XX_PRI_SEL_GROUP_0			(0)
#define RTL93XX_PRI_SEL_GROUP_1			(1)

#define RTL930X_PRI_SEL_TBL_CTRL(group)		(0x9B28 + ((group) * 4))
#define RTL931X_PRI_SEL_TBL_CTRL(group)		(0x9074 + ((group) * 8))
#define RTL931X_PRI_SEL_TBL_CTRL_1BR_MASK	GENMASK(15, 12)
#define RTL931X_PRI_SEL_TBL_CTRL_MPLS_MASK	GENMASK(11, 8)
#define RTL931X_PRI_SEL_TBL_CTRL_11E_MASK	GENMASK(7, 4)
#define RTL931X_PRI_SEL_TBL_CTRL_TUNNEL_MASK	GENMASK(3, 0)

#define RTL93XX_PRI_SEL_TBL_CTRL_ROUT_MASK	GENMASK(31, 28)
#define RTL93XX_PRI_SEL_TBL_CTRL_PROT_VLAN_MASK	GENMASK(27, 24)
#define RTL93XX_PRI_SEL_TBL_CTRL_MAC_VLAN_MASK	GENMASK(23, 20)
#define RTL93XX_PRI_SEL_TBL_CTRL_OTAG_MASK	GENMASK(19, 16)
#define RTL93XX_PRI_SEL_TBL_CTRL_ITAG_MASK	GENMASK(15, 12)
#define RTL93XX_PRI_SEL_TBL_CTRL_DSCP_MASK	GENMASK(11, 8)
#define RTL93XX_PRI_SEL_TBL_CTRL_VACL_MASK	GENMASK(7, 4)
#define RTL93XX_PRI_SEL_TBL_CTRL_PORT_MASK	GENMASK(3, 0)

/* port: 0-23, index: 0-7 */
#define RTL930X_SCHED_PORT_Q_CTRL_SET0(port, index) \
						(0x3D48 + ((port) * 384) + ((index) * 4))
/* port: 24-27, index: 0-11 */
#define RTL930X_SCHED_PORT_Q_CTRL_SET1(port, index) \
						((0xE860 + ((port) - 24) * 48) + ((index) * 4))
/* One bit per port: 0 = WFQ (byte-count), 1 = WRR (packet-count) */
#define RTL930X_SCHED_PORT_ALGO_CTRL		(0x7A9C)
/* Per-queue fields of RTL930X_SCHED_PORT_Q_CTRL_SET{0,1} */
#define RTL930X_SCHED_Q_WEIGHT_M		GENMASK(6, 0)
#define RTL930X_SCHED_Q_STRICT_EN		BIT(7)
#define RTL930X_SCHED_Q_WEIGHT_MAX		127
/* Egress bandwidth leaky buckets: one 64-bit entry per port queue, and one
 * per port. Low word: BURST bits [15:0] (bytes); high word: RATE bits
 * [19:0] (1 LSB = 16 Kbps) and EN bit 20
 * (SDK dal_longan_rate_portEgrQueueBwCtrl{Enable,Rate,BurstSize}_set and
 * dal_longan_rate_portEgrBwCtrl{Enable,Rate,BurstSize}_set).
 * port: 0-23, queue: 0-7
 */
#define RTL930X_EGBW_PORT_Q_MAX_LB_CTRL_SET0(port, q) \
							(0x3C60 + ((port) * 384) + ((q) * 8))
/* port: 24-27, queue: 0-11 */
#define RTL930X_EGBW_PORT_Q_MAX_LB_CTRL_SET1(port, q) \
							(0xE300 + (((port) - 24) * 96) + ((q) * 8))
/* port: 0-28. This one bucket is shared by root TBF and egress policer. */
#define RTL930X_EGBW_PORT_CTRL(port)		(0x7660 + ((port) * 16))
/* Cold-reset value of the burst word. The vendor SDK's
 * RTK_DEFAULT_EGR_BANDWIDTH_PORT_BURST is 0x4000, but a cold Longan reset
 * reads 0x8000 here. The burst cap gates egress even with EN clear, so the
 * driver restores the silicon reset posture rather than the SDK policy.
 */
#define RTL930X_EGBW_LB_RESET_BURST		(0x8000)
#define RTL930X_EGBW_LB_CTRL			(0x78EC)
#define RTL930X_EGBW_LB_TKN_M			GENMASK(31, 16)
#define RTL930X_EGBW_Q_RATE_M			GENMASK(19, 0)
#define RTL930X_EGBW_Q_EN			BIT(20)
#define RTL930X_EGBW_Q_BURST_M			GENMASK(15, 0)
/* The bucket stops limiting as the burst nears the field maximum: 64000 shapes
 * exactly, 64512 leaks, and at 65535 the shaper passes line rate. Clamp to the
 * largest burst that shapes rather than to the field width.
 */
#define RTL930X_EGBW_Q_BURST_MAX		(64 * 1000)
/* SWRED (simple WRED): per-port congestion avoidance algorithm select,
 * one bit per port (0 = tail drop, 1 = SWRED). Thresholds and drop
 * probability are per queue and drop precedence in units of 256-byte
 * pages, global to the switch
 * (SDK dal_longan_qos_portCongAvoidAlgo_set and
 * dal_longan_qos_congAvoidGlobalQueueConfig_set).
 */
#define RTL930X_SWRED_PORT_CTRL			(0x7A04)
#define RTL930X_SWRED_QUEUE_DROP_CTRL(q, dp)	(0x7A08 + ((q) * 12) + ((dp) * 4))
#define RTL930X_SWRED_PROB_M			GENMASK(31, 24)
#define RTL930X_SWRED_THR_MAX_M			GENMASK(23, 12)
#define RTL930X_SWRED_THR_MIN_M			GENMASK(11, 0)
#define RTL930X_SWRED_PAGE_BYTES		256
#define RTL930X_SWRED_THR_MAX_PAGES		4095
/* REF_RXCNGST points the egress-drop stage at the source port's congestion
 * state instead of the egress queue, and the SWRED thresholds are only
 * consulted with it clear. It resets set, so enabling SWRED must clear it
 * and disabling must put it back. The register itself is defined with the
 * flow-control block in rtl930x.c.
 */
#define RTL930X_FC_EGR_DROP_REF_RXCNGST		BIT(1)
#define RTL930X_SWRED_DROP_PRECEDENCES		3
/* Egress remarking: per-port enables and global source selects
 * (SDK dal_longan_qos_port{1p,Out1p,Dscp,Dei}RemarkEnable_set and
 * dal_longan_qos_{1p,Dscp,Dei}RemarkSrcSel_set). With all per-port
 * enables clear the original header fields are kept on egress.
 */
#define RTL930X_RMK_CTRL			(0xD144)
#define RTL930X_RMK_IPRI_RMK_SRC_M		GENMASK(9, 8)
#define RTL930X_RMK_OPRI_RMK_SRC_M		GENMASK(7, 6)
#define RTL930X_RMK_DEI_RMK_SRC			BIT(5)
#define RTL930X_RMK_DSCP_RMK_SRC_M		GENMASK(4, 2)
#define RTL930X_RMK_PORT_CTRL(p)		(0xD148 + ((p) * 4))
#define RTL930X_RMK_PORT_IPRI_RMK_EN		BIT(0)
#define RTL930X_RMK_PORT_OPRI_RMK_EN		BIT(1)
#define RTL930X_RMK_PORT_DSCP_RMK_EN		BIT(2)
#define RTL930X_RMK_PORT_DEI_RMK_EN		BIT(3)
#define RTL930X_RMK_PORT_DEI_RMK_TAG_SEL	BIT(4)
/* Global remark mapping tables, indexed by the selected source value */
#define RTL930X_RMK_INTPRI2IPRI_CTRL		(0xD1BC)
#define RTL930X_RMK_INTPRI2DEI_CTRL		(0xD20C)
#define RTL930X_RMK_DP2DEI_CTRL			(0xD210)
#define RTL930X_RMK_INTPRI2DSCP_CTRL(p)		(0xD214 + (((p) / 5) * 4))
/* port: 0-51, index: 0-7 */
#define RTL931X_SCHED_PORT_Q_CTRL_SET0(port, index) \
						(0x2888 + ((port) << 5) + ((index) * 4))
/* port: 52-55, index: 0-11 */
#define RTL931X_SCHED_PORT_Q_CTRL_SET1(port, index) \
						((0x2F08 + ((port) - 52) * 48) + ((index) * 4))
/* One bit per port, 32 ports per word: 0 = WFQ (byte-count),
 * 1 = WRR (packet-count)
 * (SDK swcore_rtl9310.h RTL9310_SCHED_PORT_ALGO_CTRL_ADDR,
 * dal_mango_qos_schedulingAlgorithm_set).
 */
#define RTL931X_SCHED_PORT_ALGO_CTRL(port)	(0x3048 + (((port) >> 5) << 2))
/* Per-queue fields of RTL931X_SCHED_PORT_Q_CTRL_SET{0,1}: STRICT_EN is
 * bit 8 on this family (bit 7 on RTL930x).
 */
#define RTL931X_SCHED_Q_WEIGHT_M		GENMASK(6, 0)
#define RTL931X_SCHED_Q_STRICT_EN		BIT(8)
#define RTL931X_SCHED_Q_WEIGHT_MAX		127

/* Default (port-based) internal priority: 3 bits per port, 10 ports per
 * word (SDK swcore_rtl9310.h RTL9310_PRI_SEL_REMAP_PORT_ADDR,
 * dal_mango_qos_priRemap_set with PRI_SRC_PB_PRI).
 */
#define RTL931X_PRI_SEL_PORT_PRI(p)		(0x900C + (((p / 10) << 2)))

/* Port egress bandwidth leaky bucket, a 64-bit entry per port. Low word:
 * BURST bits [15:0] (bytes); high word: RATE bits [19:0] (1 LSB = 16 Kbps)
 * and EN bit 20 (SDK swcore_rtl9310.h RTL9310_EGBW_PORT_CTRL_ADDR,
 * dal_mango_rate_portEgrBwCtrl{Enable,Rate,BurstSize}_set). The word order
 * is the reverse of RTL930x, where EN|RATE is the low word.
 */
/* This one bucket is shared by root TBF and egress policer. */
#define RTL931X_EGBW_PORT_CTRL(port)		(0x2164 + ((port) << 3))
#define RTL931X_EGBW_LB_CTRL			(0x2160)
/* TKN is the low half of EGBW_LB_CTRL on this family (the high half on
 * RTL930x).
 */
#define RTL931X_EGBW_LB_TKN_M			GENMASK(15, 0)
/* SDK default burst (RTK_DEFAULT_EGR_BANDWIDTH_PORT_BURST), programmed by
 * the vendor init for all ports and queues; the burst cap gates egress
 * even with EN clear, so "disabled" must restore it rather than write 0.
 */
#define RTL931X_EGBW_LB_RESET_BURST		(0x1194)
#define RTL931X_EGBW_Q_RATE_M			GENMASK(19, 0)
#define RTL931X_EGBW_Q_EN			BIT(20)
#define RTL931X_EGBW_Q_BURST_M			GENMASK(15, 0)
/* Per-queue egress bandwidth table EGR_Q_BW (RTL9310_TBL_4 type 0, one
 * 29-word entry per port 0-55, 12 queues). Entry bit positions of the
 * maximum-bandwidth fields of queue q (SDK rtk_mango_tableField_list.c
 * RTL9310_EGR_Q_BW_FIELDS). The assured-bandwidth fields higher up in the
 * entry are left untouched.
 */
#define RTL931X_EGR_Q_BW_WORDS			29
#define RTL931X_EGR_Q_BW_MAX_BW_EN_LSP(q)	(28 + (q))
#define RTL931X_EGR_Q_BW_MAX_BW_LSP(q)		(40 + (q) * 20)
#define RTL931X_EGR_Q_BW_MAX_BW_LEN		20
#define RTL931X_EGR_Q_BW_MAX_LB_BURST_LSP(q)	(280 + (q) * 16)
#define RTL931X_EGR_Q_BW_MAX_LB_BURST_LEN	16

/* SWRED (simple WRED): the per-port congestion avoidance algorithm select
 * (0 = tail drop, 1 = SWRED) lives in the flow-control block on this
 * family (SDK dal_mango_qos_portCongAvoidAlgo_set,
 * swcore_rtl9310.h RTL9310_FC_PORT_EGR_DROP_CTRL_ADDR). Drop rates are 8
 * bits per drop precedence, packed into one word per queue; thresholds are
 * 13 bits per drop precedence in units of 256-byte pages, one word per
 * queue and precedence, all of it global to the switch
 * (SDK dal_mango_qos_congAvoidGlobalQueueConfig_set,
 * swcore_rtl9310.h RTL9310_SWRED_Q_DROP_RATE_ADDR / RTL9310_SWRED_Q_THR_ADDR).
 */
#define RTL931X_FC_PORT_EGR_DROP_CTRL(p)	(0xA800 + ((p) << 2))
#define RTL931X_FC_EGR_DROP_ALGO_SWRED		BIT(2)
#define RTL931X_FC_EGR_DROP_REF_RXCNGST		BIT(1)
#define RTL931X_SWRED_Q_DROP_RATE(q)		(0x27C4 + ((q) << 2))
#define RTL931X_SWRED_Q_THR(q, dp)		(0x27F4 + ((q) * 12) + ((dp) << 2))
#define RTL931X_SWRED_THR_MAX_M			GENMASK(28, 16)
#define RTL931X_SWRED_THR_MIN_M			GENMASK(12, 0)
#define RTL931X_SWRED_PAGE_BYTES		256
#define RTL931X_SWRED_THR_MAX_PAGES		8191
#define RTL931X_SWRED_DROP_PRECEDENCES		3

#define RTL930X_QM_INTPRI2QID_CTRL		(0xA320)
#define RTL931X_QM_INTPRI2QID_CTRL		(0xA9D0)

/* Debug features */
#define RTL930X_STAT_PRVTE_DROP_COUNTER0	(0xB5B8)

/* Packet Inspection Engine */
#define RTL838X_METER_GLB_CTRL			(0x4B08)
#define RTL839X_METER_GLB_CTRL			(0x1300)
#define RTL930X_METER_GLB_CTRL			(0xa0a0)
#define RTL931X_METER_GLB_CTRL			(0x411C)

#define RTL839X_ACL_CTRL			(0x1288)

#define RTL838X_ACL_BLK_LOOKUP_CTRL		(0x6100)
#define RTL839X_ACL_BLK_LOOKUP_CTRL		(0x1280)
#define RTL930X_PIE_BLK_LOOKUP_CTRL		(0xa5a0)
#define RTL931X_PIE_BLK_LOOKUP_CTRL		(0x4180)

#define RTL838X_ACL_BLK_PWR_CTRL		(0x6104)
#define RTL839X_PS_ACL_PWR_CTRL			(0x049c)

#define RTL838X_ACL_BLK_TMPLTE_CTRL(block)	(0x6108 + ((block) << 2))
#define RTL839X_ACL_BLK_TMPLTE_CTRL(block)	(0x128c + ((block) << 2))
#define RTL930X_PIE_BLK_TMPLTE_CTRL(block)	(0xa624 + ((block) << 2))
#define RTL931X_PIE_BLK_TMPLTE_CTRL(block)	(0x4214 + ((block) << 2))

#define RTL838X_ACL_BLK_GROUP_CTRL		(0x615C)
#define RTL839X_ACL_BLK_GROUP_CTRL		(0x12ec)

#define RTL838X_ACL_CLR_CTRL			(0x6168)
#define RTL839X_ACL_CLR_CTRL			(0x12fc)
#define RTL930X_PIE_CLR_CTRL			(0xa66c)
#define RTL931X_PIE_CLR_CTRL			(0x42D8)

#define RTL838X_DMY_REG27			(0x3378)

#define RTL838X_ACL_PORT_LOOKUP_CTRL(p)		(0x616C + (((p) << 2)))
#define RTL930X_ACL_PORT_LOOKUP_CTRL(p)		(0xA784 + (((p) << 2)))
#define RTL931X_ACL_PORT_LOOKUP_CTRL(p)		(0x44F8 + (((p) << 2)))

#define RTL930X_PIE_BLK_PHASE_CTRL		(0xA5A4)
#define RTL931X_PIE_BLK_PHASE_CTRL		(0x4184)

/* PIE actions */
#define PIE_ACT_COPY_TO_PORT	2
#define PIE_ACT_REDIRECT_TO_PORT 4
#define PIE_ACT_ROUTE_UC	6
#define PIE_ACT_VID_ASSIGN	0

/* L3 actions */
#define L3_FORWARD		0
#define L3_DROP			1
#define L3_TRAP2CPU		2
#define L3_COPY2CPU		3
#define L3_TRAP2MASTERCPU	4
#define L3_COPY2MASTERCPU	5
#define L3_HARDDROP		6

/* Route actions */
#define ROUTE_ACT_FORWARD	0
#define ROUTE_ACT_TRAP2CPU	1
#define ROUTE_ACT_COPY2CPU	2
#define ROUTE_ACT_DROP		3

/* L3 Routing */
#define RTL839X_ROUTING_SA_CTRL			0x6afc
#define RTL930X_L3_HOST_TBL_CTRL		(0xAB48)
#define RTL930X_L3_IPUC_ROUTE_CTRL		(0xAB4C)
#define RTL930X_L3_IP6UC_ROUTE_CTRL		(0xAB50)
#define RTL930X_L3_IPMC_ROUTE_CTRL		(0xAB54)
#define RTL930X_L3_IP6MC_ROUTE_CTRL		(0xAB58)
#define RTL930X_L3_IP_MTU_CTRL(i)		(0xAB5C + ((i >> 1) << 2))
#define RTL930X_L3_IP6_MTU_CTRL(i)		(0xAB6C + ((i >> 1) << 2))
#define RTL930X_L3_HW_LU_KEY_CTRL		(0xAC9C)
#define RTL930X_L3_HW_LU_KEY_IP_CTRL		(0xACA0)
#define RTL930X_L3_HW_LU_CTRL			(0xACC0)
#define RTL930X_L3_IP_ROUTE_CTRL		0xab44

/* RTL931x (Mango) L3 routing - register addresses and field layouts from the
 * Mango SDK (swcore_rtl9310.h, dal_mango_l3.c)
 */
#define RTL931X_L3_IP_ROUTE_CTRL		(0xF000)
#define RTL931X_L3_HOST_TBL_CTRL		(0xF004)
#define RTL931X_L3_IPUC_ROUTE_CTRL		(0xF008)
#define RTL931X_L3_IP6UC_ROUTE_CTRL		(0xF00C)
/* Multicast routing has a global enable of its own, in addition to the
 * per-ingress-interface IPMC_ROUTE_EN in L3_IGR_INTF. Both are required: with
 * GLB_EN (bit 0) clear the route lookup is not performed at all, so an entry
 * stays valid and correctly keyed and its hit bit never sets.
 */
#define RTL931X_L3_IPMC_ROUTE_CTRL		(0xF010)
#define RTL931X_L3_IP6MC_ROUTE_CTRL		(0xF014)
#define RTL931X_L3_INTF_IP_MTU(i)		(0xF1E0 + ((i) << 2))
#define RTL931X_L3_INTF_IP6_MTU(i)		(0xF220 + ((i) << 2))
#define RTL931X_L3_ENTRY_MV_CTRL		(0xF260)
#define RTL931X_L3_ENTRY_MV_PARAM		(0xF264)
#define RTL931X_L3_HW_LU_KEY_CTRL		(0xF29C)
#define RTL931X_L3_HW_LU_KEY_DIP_CTRL		(0xF2B0)
#define RTL931X_L3_HW_LU_CTRL			(0xF2C0)
#define RTL931X_ALE_L3_MISC_CTRL		(0xF2E8)

/* Port LED Control */
#define RTL930X_LED_PORT_NUM_CTRL(p)		(0xCC04 + (((p >> 4) << 2)))
#define RTL930X_LED_SET0_0_CTRL			(0xCC28)
#define RTL930X_LED_PORT_COPR_SET_SEL_CTRL(p)	(0xCC2C + (((p >> 4) << 2)))
#define RTL930X_LED_PORT_FIB_SET_SEL_CTRL(p)	(0xCC34 + (((p >> 4) << 2)))
#define RTL930X_LED_PORT_COPR_MASK_CTRL		(0xCC3C)
#define RTL930X_LED_PORT_FIB_MASK_CTRL		(0xCC40)
#define RTL930X_LED_PORT_COMBO_MASK_CTRL	(0xCC44)

#define RTL931X_LED_PORT_NUM_CTRL(p)		(0x0604 + (((p >> 4) << 2)))
#define RTL931X_LED_SET0_0_CTRL			(0x0630)
#define RTL931X_LED_PORT_COPR_SET_SEL_CTRL(p)	(0x0634 + (((p >> 4) << 2)))
#define RTL931X_LED_PORT_FIB_SET_SEL_CTRL(p)	(0x0644 + (((p >> 4) << 2)))
#define RTL931X_LED_PORT_COPR_MASK_CTRL		(0x0654)
#define RTL931X_LED_PORT_FIB_MASK_CTRL		(0x065c)
#define RTL931X_LED_PORT_COMBO_MASK_CTRL	(0x0664)

#define RTL931X_LED_GLB_ACTIVE_LOW BIT(21)

#define RTL931X_LED_SETX_0_CTRL(x) (RTL931X_LED_SET0_0_CTRL - (x * 8))
#define RTL931X_LED_SETX_1_CTRL(x) (RTL931X_LED_SETX_0_CTRL(x) - 4)

/* get register for given set and led in the set */
#define RTL931X_LED_SETX_LEDY(x, y) (RTL931X_LED_SETX_0_CTRL(x) - 4 * (y / 2))

/* get shift for given led in any set */
#define RTL931X_LED_SET_LEDX_SHIFT(x) (16 * (x % 2))

#define MAX_VLANS 4096
/* Reserved internal VLANs giving standalone (routed) ports an egress VLAN
 * context for L3 offload, one per possible user port, counting down from
 * 4094 (4095 is reserved by 802.1Q). Only used when the SoC offloads host
 * routes; see rtldsa_l3_port_vlan_set().
 */
#define RTLDSA_L3_PORT_VID(port) (4094 - (port))
#define MAX_LAGS 16
#define MAX_PRIOS 8
#define RTL930X_PORT_IGNORE 0x3f
#define MAX_MC_GROUPS 512
#define UNKNOWN_MC_PMASK (MAX_MC_GROUPS - 1)
#define PIE_BLOCK_SIZE 128
#define MAX_PIE_ENTRIES (18 * PIE_BLOCK_SIZE)
#define N_FIXED_FIELDS 12
#define N_FIXED_FIELDS_RTL931X 14
#define MAX_COUNTERS 2048
/* Software id spaces for offloaded routes, sized for the largest family:
 * both RTL931x route tables hold 12288 entries. The usable range is
 * clamped per family at L3 setup through priv->n_route_ids /
 * priv->n_host_route_ids; host-route ids stay offset by MAX_ROUTES so
 * the two spaces cannot collide.
 */
#define MAX_ROUTES 12288
#define MAX_HOST_ROUTES 12288
#define MAX_INTF_MTUS 8
#define DEFAULT_MTU 1536
#define MAX_INTERFACES 100
#define MAX_ROUTER_MACS 64
#define L3_EGRESS_DMACS 2048
#define MAX_SMACS 64
/* Hardware output-interface list elements backing IPMR/IP6MR routes, sized
 * for the largest family: RTL931x has 16384, RTL930x 512. Clamped per family
 * at L3 setup through priv->n_mc_oifs. Element 0 doubles as the "no next
 * element" encoding on both families and is never allocated.
 */
#define MAX_MC_OIFS 16384
#define DSCP_MAP_MAX 64

/* This interval needs to be short enough to prevent an undetected counter
 * overflow. The octet counters don't need to be considered for this, because
 * they are 64 bits on all platforms. Based on the possible packets per second
 * at the highest supported speeds, an interval of a minute is probably a safe
 * choice for the other counters.
 */
#define RTLDSA_COUNTERS_POLL_INTERVAL	(60 * HZ)

/* Some SoC families require table access to get the HW counters. A mutex is
 * required for this access - which will potentially cause a sleep in the
 * current context. This is not always possible with .get_stats64 because it
 * is also called in atomic contexts.
 *
 * For these SoCs, the retrieval of the current counters in .get_stats64 is
 * skipped and the counters are simply retrieved a lot more often from the HW.
 */
#define RTLDSA_COUNTERS_FAST_POLL_INTERVAL	(3 * HZ)

enum phy_type {
	PHY_NONE = 0,
	PHY_RTL838X_SDS = 1,
	PHY_RTL8218B_INT = 2,
	PHY_RTL8218B_EXT = 3,
	PHY_RTL8214FC = 4,
	PHY_RTL839X_SDS = 5,
};

enum pbvlan_type {
	PBVLAN_TYPE_INNER = 0,
	PBVLAN_TYPE_OUTER,
};

enum pbvlan_mode {
	PBVLAN_MODE_UNTAG_AND_PRITAG = 0,
	PBVLAN_MODE_UNTAG_ONLY,
	PBVLAN_MODE_ALL_PKT,
};

struct rtldsa_counter {
	u64 val;
	u32 last;
};

struct rtldsa_counter_state {
	/**
	 * @lock: protect updates to members of the structure when the
	 * priv->counters_lock is not used. (see rtl931x_reg->stat_update_counters_atomically)
	 */
	spinlock_t lock;
	ktime_t last_update;

	struct rtldsa_counter symbol_errors;

	struct rtldsa_counter if_in_octets;
	struct rtldsa_counter if_out_octets;
	struct rtldsa_counter if_in_ucast_pkts;
	struct rtldsa_counter if_in_mcast_pkts;
	struct rtldsa_counter if_in_bcast_pkts;
	struct rtldsa_counter if_out_ucast_pkts;
	struct rtldsa_counter if_out_mcast_pkts;
	struct rtldsa_counter if_out_bcast_pkts;
	struct rtldsa_counter if_out_discards;
	struct rtldsa_counter single_collisions;
	struct rtldsa_counter multiple_collisions;
	struct rtldsa_counter deferred_transmissions;
	struct rtldsa_counter late_collisions;
	struct rtldsa_counter excessive_collisions;
	struct rtldsa_counter crc_align_errors;
	struct rtldsa_counter rx_pkts_over_max_octets;

	struct rtldsa_counter unsupported_opcodes;

	struct rtldsa_counter rx_undersize_pkts;
	struct rtldsa_counter rx_oversize_pkts;
	struct rtldsa_counter rx_fragments;
	struct rtldsa_counter rx_jabbers;

	struct rtldsa_counter tx_pkts[ETHTOOL_RMON_HIST_MAX];
	struct rtldsa_counter rx_pkts[ETHTOOL_RMON_HIST_MAX];

	struct rtldsa_counter drop_events;
	struct rtldsa_counter collisions;

	struct rtldsa_counter rx_pause_frames;
	struct rtldsa_counter tx_pause_frames;

	/** @link_stat_lock: Protect link_stat */
	spinlock_t link_stat_lock;

	/** @link_stat: Prepared return data for .get_stats64 which can be accessed without mutex */
	struct rtnl_link_stats64 link_stat;
};

struct psample_group;
struct seq_file;

/* Per-port hardware packet sampling state, indexed by direction
 * (0 = ingress, 1 = egress). Written under reg_mutex from the tc
 * offload path, read locklessly from the conduit RX path under
 * rcu_read_lock(); readers must use READ_ONCE() on group.
 */
struct rtldsa_sample {
	struct psample_group *group;
	u32 rate;
	u32 trunc_size;
	/* Frames the ASIC tagged as sampled in this direction, counted on
	 * arrival and before any delivery decision.
	 */
	u64 seen;
};

#define RTL838X_SWRED_DROP_PRECEDENCES	3

struct rtl838x_qos_swred_state {
	bool enabled;
	u16 min_pages[MAX_PRIOS][RTL838X_SWRED_DROP_PRECEDENCES];
	u16 max_pages[MAX_PRIOS][RTL838X_SWRED_DROP_PRECEDENCES];
	u8 probability[MAX_PRIOS][RTL838X_SWRED_DROP_PRECEDENCES];
};

enum rtl838x_port_shaper_owner {
	RTL838X_PORT_SHAPER_NONE,
	RTL838X_PORT_SHAPER_TBF,
	RTL838X_PORT_SHAPER_POLICER,
};

struct rtl838x_port {
	bool enable:1;
	bool phy_is_integrated:1;
	bool sfp:1;
	bool is10G:1;
	bool is2G5:1;
	bool isolated:1;
	bool qinq:1;
	bool rate_police_ingress:1;
	/* Root TBF and egress police share one hardware bucket. Protected by
	 * reg_mutex with the register programming paths.
	 */
	enum rtl838x_port_shaper_owner egress_shaper_owner;
	u64 pm;
	u16 pvid;
	bool eee_enabled;
	enum phy_type phy;
	int led_set;
	int leds_on_this_port;
	struct rtldsa_counter_state counters;
	struct rtldsa_sample sample[2];
	const struct dsa_port *dp;
};

struct rtl838x_vlan_info {
	u64 untagged_ports;
	u64 member_ports;
	u8 profile_id;
	bool hash_mc_fid;
	bool hash_uc_fid;
	u8 fid; /* AKA MSTI */

	/* The following fields are used only by the RTL931X */
	int if_id;		/* Interface (index in L3_EGR_INTF_IDX) */
	u16 multicast_grp_mask;
	int l2_tunnel_list_id;
};

struct rtldsa_mst {
	/** @msti: MSTI mapped to this slot. 0 == unused */
	u16 msti;

	/** @refcount: number of vlans currently using this msti, undefined when unused */
	struct kref refcount;
};

enum l2_entry_type {
	L2_INVALID = 0,
	L2_UNICAST = 1,
	L2_MULTICAST = 2,
	IP4_MULTICAST = 3,
	IP6_MULTICAST = 4,
};

struct rtl838x_l2_entry {
	u8 mac[6];
	u16 vid;
	u16 rvid;
	u8 port;
	enum l2_entry_type type;
	bool valid:1;
	bool is_static:1;
	bool is_ip_mc:1;
	bool is_ipv6_mc:1;
	bool block_da:1;
	bool block_sa:1;
	bool suspended:1;
	bool next_hop:1;
	bool is_trunk:1;
	bool nh_vlan_target:1;  /* Only RTL83xx: VLAN used for next hop */
	int age;
	u8 trunk;
	u8 stack_dev;
	u16 mc_portmask_index;
	u32 mc_gip;
	u32 mc_sip;
	u16 mc_mac_index;
	u16 nh_route_id;

	/* The following is only valid on RTL931x */
	bool is_open_flow:1;
	bool is_pe_forward:1;
	bool is_local_forward:1;
	bool is_remote_forward:1;
	bool is_l2_tunnel:1;
	bool hash_msb:1;
	int l2_tunnel_id;
	int l2_tunnel_list_id;
};

enum fwd_rule_action {
	FWD_RULE_ACTION_NONE = 0,
	FWD_RULE_ACTION_FWD = 1,
};

enum pie_phase {
	PHASE_VACL = 0,
	PHASE_IACL = 1,
};

enum igr_filter {
	IGR_FORWARD = 0,
	IGR_DROP = 1,
	IGR_TRAP = 2,
};

enum egr_filter {
	EGR_DISABLE = 0,
	EGR_ENABLE = 1,
};

/* Intermediate representation of a  Packet Inspection Engine Rule
 * as suggested by the Kernel's tc flower offload subsystem
 * Field meaning is universal across SoC families, but data content is specific
 * to SoC family (e.g. because of different port ranges)
 */
struct pie_rule {
	int id;
	enum pie_phase phase;	/* Phase in which this template is applied */
	int packet_cntr;	/* ID of a packet counter assigned to this rule */
	int octet_cntr;		/* ID of a byte counter assigned to this rule */
	u32 last_packet_cnt;
	u64 last_octet_cnt;

	/* The following are requirements for the pie template */
	bool is_egress;
	bool is_ipv6;		/* This is a rule with IPv6 fields */

	/* Fixed fields that are always matched against on RTL8380 */
	u8 spmmask_fix;
	u8 spn;			/* Source port number */
	bool stacking_port;	/* Source port is stacking port */
	bool mgnt_vlan;		/* Packet arrived on management VLAN */
	bool dmac_hit_sw;	/* The packet's destination MAC matches one of the device's */
	bool content_too_deep;	/* The content of the packet cannot be parsed: too many layers */
	bool not_first_frag;	/* Not the first IP fragment */
	u8 frame_type_l4;	/* 0: UDP, 1: TCP, 2: ICMP/ICMPv6, 3: IGMP */
	u8 frame_type;		/* 0: ARP, 1: L2 only, 2: IPv4, 3: IPv6 */
	bool otag_fmt;		/* 0: outer tag packet, 1: outer priority tag or untagged */
	bool itag_fmt;		/* 0: inner tag packet, 1: inner priority tag or untagged */
	bool otag_exist;	/* packet with outer tag */
	bool itag_exist;	/* packet with inner tag */
	bool frame_type_l2;	/* 0: Ethernet, 1: LLC_SNAP, 2: LLC_Other, 3: Reserved */
	bool igr_normal_port;	/* Ingress port is not cpu or stacking port */
	u8 tid;			/* The template ID defining the what the templated fields mean */

	/* Masks for the fields that are always matched against on RTL8380 */
	u8 spmmask_fix_m;
	u8 spn_m;
	bool stacking_port_m;
	bool mgnt_vlan_m;
	bool dmac_hit_sw_m;
	bool content_too_deep_m;
	bool not_first_frag_m;
	u8 frame_type_l4_m;
	u8 frame_type_m;
	bool otag_fmt_m;
	bool itag_fmt_m;
	bool otag_exist_m;
	bool itag_exist_m;
	bool frame_type_l2_m;
	bool igr_normal_port_m;
	u8 tid_m;

	/* Logical operations between rules, special rules for rule numbers apply */
	bool valid;
	bool cond_not;		/* Matches when conditions not match */
	bool cond_and1;		/* And this rule 2n with the next rule 2n+1 in same block */
	bool cond_and2;		/* And this rule m in block 2n with rule m in block 2n+1 */
	bool ivalid;

	/* Actions to be performed */
	bool drop;		/* Drop the packet */
	bool fwd_sel;		/* Forward packet: to port, portmask, dest route, next rule, drop */
	bool ovid_sel;		/* So something to outer vlan-id: shift, re-assign */
	bool ivid_sel;		/* Do something to inner vlan-id: shift, re-assign */
	bool flt_sel;		/* Filter the packet when sending to certain ports */
	bool log_sel;		/* Log the packet in one of the LOG-table counters */
	bool rmk_sel;		/* Re-mark the packet, i.e. change the priority-tag */
	bool meter_sel;		/* Meter the packet, i.e. limit rate of this type of packet */
	bool tagst_sel;		/* Change the ergress tag */
	bool mir_sel;		/* Mirror the packet to a Link Aggregation Group */
	bool nopri_sel;		/* Change the normal priority */
	bool cpupri_sel;	/* Change the CPU priority */
	bool otpid_sel;		/* Change Outer Tag Protocol Identifier (802.1q) */
	bool itpid_sel;		/* Change Inner Tag Protocol Identifier (802.1q) */
	bool shaper_sel;	/* Apply traffic shaper */
	bool mpls_sel;		/* MPLS actions */
	bool bypass_sel;	/* Bypass actions */
	bool fwd_sa_lrn;	/* Learn the source address when forwarding */
	bool fwd_mod_to_cpu;	/* Forward the modified VLAN tag format to CPU-port */

	/* Fields used in predefined templates 0-2 on RTL8380 / 90 / 9300 */
	u64 spm;		/* Source Port Matrix */
	u16 otag;		/* Outer VLAN-ID */
	u8 smac[ETH_ALEN];	/* Source MAC address */
	u8 dmac[ETH_ALEN];	/* Destination MAC address */
	u16 ethertype;		/* Ethernet frame type field in ethernet header */
	u16 itag;		/* Inner VLAN-ID */
	u16 field_range_check;
	u32 sip;		/* Source IP */
	struct in6_addr sip6;	/* IPv6 Source IP */
	u32 dip;		/* Destination IP */
	struct in6_addr dip6;	/* IPv6 Destination IP */
	u16 tos_proto;		/* IPv4: TOS + Protocol fields, IPv6: Traffic class + next header */
	u16 sport;		/* TCP/UDP source port */
	u16 dport;		/* TCP/UDP destination port */
	u16 icmp_igmp;
	u16 tcp_info;
	u16 dsap_ssap;		/* Destination / Source Service Access Point bytes (802.3) */

	u64 spm_m;
	u16 otag_m;
	u8 smac_m[ETH_ALEN];
	u8 dmac_m[ETH_ALEN];
	u8 ethertype_m;
	u16 itag_m;
	u16 field_range_check_m;
	u32 sip_m;
	struct in6_addr sip6_m;	/* IPv6 Source IP mask */
	u32 dip_m;
	struct in6_addr dip6_m;	/* IPv6 Destination IP mask */
	u16 tos_proto_m;
	u16 sport_m;
	u16 dport_m;
	u16 icmp_igmp_m;
	u16 tcp_info_m;
	u16 dsap_ssap_m;

	/* Data associated with actions */
	u8 fwd_act;		/* Type of forwarding action */
				/* 0: permit, 1: drop, 2: copy to port id, 4: copy to portmask */
				/* 4: redirect to portid, 5: redirect to portmask */
				/* 6: route, 7: vlan leaky (only 8380) */
	u16 fwd_data;		/* Additional data for forwarding action, e.g. destination port */
	u8 ovid_act;
	u16 ovid_data;		/* Outer VLAN ID */
	u8 ivid_act;
	u16 ivid_data;		/* Inner VLAN ID */
	u16 flt_data;		/* Filtering data */
	u16 log_data;		/* ID of packet or octet counter in LOG table, on RTL93xx */
				/* unnecessary since PIE-Rule-ID == LOG-counter-ID */
	bool log_octets;
	u8 mpls_act;		/* MPLS action type */
	u16 mpls_lib_idx;	/* MPLS action data */

	u16 rmk_data;		/* Data for remarking */
	u16 meter_data;		/* ID of meter for bandwidth control */
	u16 tagst_data;
	u16 mir_data;
	u16 nopri_data;
	u16 cpupri_data;
	u16 otpid_data;
	u16 itpid_data;
	u16 shaper_data;

	/* Bypass actions, ignored on RTL8380 */
	bool bypass_all;	/* Not clear */
	bool bypass_igr_stp;	/* Bypass Ingress STP state */
	bool bypass_ibc_sc;	/* Bypass Ingress Bandwidth Control and Storm Control */
};

struct rtl838x_l3_intf {
	u16 vid;
	u8 smac_idx;
	u8 ip4_mtu_id;
	u8 ip6_mtu_id;
	u16 ip4_mtu;
	u16 ip6_mtu;
	u8 ttl_scope;
	u8 hl_scope;
	u8 ip4_icmp_redirect;
	u8 ip6_icmp_redirect;
	u8 ip4_pbr_icmp_redirect;
	u8 ip6_pbr_icmp_redirect;
};

/* An entry in the RTL93XX SoC's ROUTER_MAC tables setting up a termination point
 * for the L3 routing system. Packets arriving and matching an entry in this table
 * will be considered for routing.
 * Mask fields state whether the corresponding data fields matter for matching
 */
struct rtl93xx_rt_mac {
	bool valid;	/* Valid or not */
	bool p_type;	/* Individual (0) or trunk (1) port */
	bool p_mask;	/* Whether the port type is used */
	u8 p_id;
	u8 p_id_mask;	/* Mask for the port */
	u8 action;	/* Routing action performed: 0: FORWARD, 1: DROP, 2: TRAP2CPU */
			/*   3: COPY2CPU, 4: TRAP2MASTERCPU, 5: COPY2MASTERCPU, 6: HARDDROP */
	u16 vid;
	u16 vid_mask;
	u64 mac;	/* MAC address used as source MAC in the routed packet */
	u64 mac_mask;
};

struct rtl83xx_nexthop {
	u16 id;		/* ID: L3_NEXT_HOP table-index or route-index set in L2_NEXT_HOP */
	u32 dev_id;
	u16 port;
	u16 vid;	/* VLAN-ID for L2 table entry (saved from L2-UC entry) */
	u16 rvid;	/* Relay VID/FID for the L2 table entry */
	u64 mac;	/* The MAC address of the entry in the L2_NEXT_HOP table */
	u16 mac_id;
	u16 l2_id;	/* Index of this next hop forwarding entry in L2 FIB table */
	u64 gw;		/* The gateway MAC address packets are forwarded to */
	int if_id;	/* Interface (into L3_EGR_INTF_IDX) */
};

struct rtl838x_switch_priv;

struct rtl83xx_flow {
	unsigned long cookie;
	struct rhash_head node;
	struct rcu_head rcu_head;
	struct rtl838x_switch_priv *priv;
	struct pie_rule rule;
	u32 flags;
};

struct rtl93xx_route_attr {
	bool valid;
	bool hit;
	bool ttl_dec;
	bool ttl_check;
	bool dst_null;
	bool qos_as;
	u8 qos_prio;
	u8 type;
	u8 action;
};

/* Result fields of a multicast route entry. The unicast attributes above
 * describe a nexthop; a multicast entry replicates instead, so it carries a
 * reverse-path check and a pointer to an output-interface list.
 */
struct rtl93xx_mc_route_attr {
	u16 rpf_vid;		/* Expected ingress VLAN. Both families compare a
				 * VLAN here, not an L3 interface id: Longan has
				 * only a VID field, and Mango's RPF id follows
				 * its multicast key select, which this driver
				 * leaves in VLAN mode.
				 */
	bool rpf_check;
	u8 rpf_fail_action;	/* ROUTE_ACT_* */
	u8 ttl_min;		/* Ingress admission threshold, 0 = accept any */
	u16 oif_idx;		/* Head element of the output-interface list */
	bool oif_valid;
};

struct rtl83xx_route {
	struct in6_addr gw_ip6;		/* IP of the route's gateway - the hashtable key.
					 * IPv4 gateways are stored v4-mapped so the
					 * connected-route keys 0.0.0.0 and :: never share
					 * a bucket.
					 */
	u32 dst_ip;			/* IPv4 destination net */
	struct in6_addr dst_ip6;	/* IPv6 destination net */
	int prefix_len;			/* Network prefix len of the destination net */
	bool is_host_route;
	bool neigh_route;		/* Synthesized from a neighbour entry, not the FIB:
					 * torn down on neighbour invalidation
					 */
	int ifindex;			/* netdev the gateway neighbour lives on; 0 = unknown */
	u32 src_ip;			/* IPv4 multicast source, 0 for (*,G) */
	struct in6_addr src_ip6;	/* IPv6 multicast source, :: for (*,G) */
	int id;				/* ID number of this route */
	struct rhlist_head linkage;
	u16 switch_mac_id;		/* Index into switch's own MACs, RTL839X only */
	struct rtl83xx_nexthop nh;
	struct pie_rule pr;
	struct rtl93xx_route_attr attr;
	struct rtl93xx_mc_route_attr mc;
};

/* Multicast routing tracks one vif table per address family. The value must
 * match the kernel's MAXVIFS; common.c asserts that against <linux/mroute.h>
 * so this header does not have to pull the multicast routing headers in.
 */
#define RTLDSA_MC_MAX_VIFS 32

/* One element of a hardware output-interface list: an egress L3 interface
 * together with the port set inside it. Hardware chains the elements, so a
 * writer needs its successor as well as its own contents.
 */
struct rtl83xx_mc_oif {
	u16 intf_id;		/* Index into the L3 egress interface table */
	u16 pmask_idx;		/* Multicast portmask group */
	u16 next;		/* Successor element, 0 when this is the last */
	bool last;
	bool ttl_dec;
	bool ttl_check;
};

/* Resources a single output interface of a multicast route holds. Freed in
 * reverse on teardown; the vif index ties it back to the kernel's vif table
 * so a vif going away can find the routes referencing it.
 */
struct rtl83xx_mc_oif_res {
	u16 oif_idx;
	u16 pmask_idx;
	u16 intf_id;
	u16 vif;
};

/* Key of an offloaded multicast route. IPv4 addresses are stored v4-mapped so
 * an IPv4 and an IPv6 group can never share a bucket, matching the convention
 * struct rtl83xx_route uses for gateways.
 */
struct rtl83xx_mc_key {
	struct in6_addr grp;
	struct in6_addr src;	/* :: for (*,G) */
	u32 family;		/* RTNL_FAMILY_IPMR or RTNL_FAMILY_IP6MR. Sized so the
				 * key has no padding: rhashtable compares it with
				 * memcmp over the whole struct.
				 */
};

struct rtl83xx_mc_route {
	struct rhash_head node;
	/* Iterating the hashtable holds rcu_read_lock, which rules out the
	 * mutex-taking table accessors, so bulk work walks this list under RTNL
	 * instead.
	 */
	struct list_head list;
	struct rtl83xx_mc_key key;
	struct mr_mfc *mfc;		/* Reference held for the route's lifetime */
	int id;				/* Host-route id, offset by MAX_ROUTES */
	int slot;			/* Hardware host-table slot, -1 if unplaced */
	struct rtl83xx_route rt;	/* Image of the programmed hardware entry */
	struct rtl83xx_mc_oif_res oifs[RTLDSA_MC_MAX_VIFS];
	int n_oifs;
	/* The L2 multicast entry this route claimed so bridging stops flooding
	 * the group into the ingress port's internal VLAN. l2_vid is zero when
	 * no entry is held, which includes losing a race with the MDB path.
	 */
	u16 l2_vid;
	int l2_port;
	u64 l2_mac;
	bool offloaded;
};

/* A multicast routing vif. Only vifs whose netdev resolves to a switch L3
 * interface can be replicated in hardware; the rest keep dev set but stay
 * invalid so a route using them is trapped instead of silently dropped.
 */
struct rtl83xx_mc_vif {
	struct net_device *dev;
	u16 intf_id;
	u16 vid;		/* VLAN the interface routes in; the reverse-path
				 * check compares against it
				 */
	u16 pmask_idx;
	bool valid;
};

/**
 * struct rtldsa_mirror_config - Mirror configuration for specific group and port
 */
struct rtldsa_mirror_config {
	/** @ctrl: control register for mirroring group */
	int ctrl;

	/** @spm: register for the destination port members */
	int spm;

	/** @dpm: register for the source port members */
	int dpm;

	/** @val: @ctrl register settings to enable mirroring */
	u32 val;
};

struct rtl838x_reg {
	void (*mask_port_reg_be)(u64 clear, u64 set, int reg);
	void (*set_port_reg_be)(u64 set, int reg);
	u64 (*get_port_reg_be)(int reg);
	void (*mask_port_reg_le)(u64 clear, u64 set, int reg);
	void (*set_port_reg_le)(u64 set, int reg);
	u64 (*get_port_reg_le)(int reg);
	int stat_port_rst;
	int stat_rst;
	int stat_port_std_mib;
	int stat_port_prv_mib;
	u64 (*stat_port_table_read)(int port, unsigned int mib_size, unsigned int offset, bool is_pvt);
	void (*stat_counters_lock)(struct rtl838x_switch_priv *priv, int port);
	void (*stat_counters_unlock)(struct rtl838x_switch_priv *priv, int port);

	/**
	 * @stat_update_counters_atomically: When set, the SoC family allows atomically retrieving
	 * of statistic counters using this function.  This function must not require "might_sleep"
	 * code.
	 *
	 * Any SoC family which requires stat_port_table_read must use the table
	 * rtldsa_counters_(un)lock_table helpers. They are using a mutex for locking. The counters
	 * update is therefore not atomic.
	 */
	void (*stat_update_counters_atomically)(struct rtl838x_switch_priv *priv, int port);
	unsigned long stat_counter_poll_interval;
	int (*port_iso_ctrl)(int p);
	void (*traffic_enable)(int source, int dest);
	void (*traffic_disable)(int source, int dest);
	void (*traffic_set)(int source, u64 dest_matrix);
	int l2_ctrl_0;
	int l2_ctrl_1;
	int smi_poll_ctrl;
	u32 l2_port_aging_out;
	int l2_tbl_flush_ctrl;
	void (*exec_tbl0_cmd)(u32 cmd);
	void (*exec_tbl1_cmd)(u32 cmd);
	int (*tbl_access_data_0)(int i);
	int isr_glb_src;
	int isr_port_link_sts_chg;
	int imr_port_link_sts_chg;
	int imr_glb;
	void (*vlan_tables_read)(u32 vlan, struct rtl838x_vlan_info *info);
	void (*vlan_set_tagged)(u32 vlan, struct rtl838x_vlan_info *info);
	void (*vlan_set_untagged)(u32 vlan, u64 portmask);
	void (*vlan_profile_dump)(int index);
	void (*vlan_profile_setup)(int profile);
	void (*vlan_port_pvidmode_set)(int port, enum pbvlan_type type, enum pbvlan_mode mode);
	void (*vlan_port_pvid_set)(int port, enum pbvlan_type type, int pvid);
	void (*vlan_port_keep_tag_set)(int port, bool keep_outer, bool keep_inner);
	void (*vlan_qinq_setup)(struct rtl838x_switch_priv *priv);
	void (*vlan_port_qinq_set)(int port, bool enable);
	int (*vlan_port_fast_age)(struct rtl838x_switch_priv *priv, int port, u16 vid);
	void (*set_vlan_igr_filter)(int port, enum igr_filter state);
	void (*set_vlan_egr_filter)(int port, enum egr_filter state);
	void (*enable_learning)(int port, bool enable);
	void (*enable_flood)(int port, bool enable);
	void (*enable_mcast_flood)(int port, bool enable);
	void (*enable_bcast_flood)(int port, bool enable);
	void (*set_static_move_action)(int port, bool forward);
	void (*stp_get)(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[]);
	void (*stp_set)(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[]);
	int  (*mac_force_mode_ctrl)(int port);
	int  (*mac_port_ctrl)(int port);
	int  (*l2_port_new_salrn)(int port);
	int  (*l2_port_new_sa_fwd)(int port);
	int (*set_ageing_time)(unsigned long msec);
	int (*get_mirror_config)(struct rtldsa_mirror_config *config, int group, int port);
	int (*port_rate_police_add)(struct dsa_switch *ds, int port,
				    const struct flow_action_entry *act, bool ingress);
	int (*port_rate_police_del)(struct dsa_switch *ds, int port, struct flow_cls_offload *cls,
				    bool ingress);
	u64 (*read_l2_entry_using_hash)(u32 hash, u32 position, struct rtl838x_l2_entry *e);
	void (*write_l2_entry_using_hash)(u32 hash, u32 pos, struct rtl838x_l2_entry *e);
	u64 (*read_cam)(int idx, struct rtl838x_l2_entry *e);
	void (*write_cam)(int idx, struct rtl838x_l2_entry *e);
	int (*trk_mbr_ctr)(int group);
	/* Optional extra trunk programming beyond the member mask: the
	 * RTL930x/RTL931x additionally need the per-source-port trunk mapping
	 * and the egress candidate list the TX hash indexes into.
	 */
	void (*trunk_srcmap_set)(int port, bool valid, int group);
	void (*trunk_egr_ports_set)(int group, u64 members);
	int rma_bpdu_fld_pmask;
	int spcl_trap_eapol_ctrl;
	void (*init_eee)(struct rtl838x_switch_priv *priv, bool enable);
	void (*set_mac_eee)(struct rtl838x_switch_priv *priv, int port, bool enable);
	u64 (*l2_hash_seed)(u64 mac, u32 vid);
	u32 (*l2_hash_key)(struct rtl838x_switch_priv *priv, u64 seed);
	u64 (*read_mcast_pmask)(int idx);
	void (*write_mcast_pmask)(int idx, u64 portmask);
	void (*vlan_fwd_on_inner)(int port, bool is_set);
	void (*pie_init)(struct rtl838x_switch_priv *priv);
	int (*pie_rule_read)(struct rtl838x_switch_priv *priv, int idx, struct  pie_rule *pr);
	int (*pie_rule_write)(struct rtl838x_switch_priv *priv, int idx, struct pie_rule *pr);
	int (*pie_rule_add)(struct rtl838x_switch_priv *priv, struct pie_rule *rule);
	void (*pie_rule_rm)(struct rtl838x_switch_priv *priv, struct pie_rule *rule);
	void (*l2_learning_setup)(void);
	u32 (*packet_cntr_read)(int counter);
	void (*packet_cntr_clear)(int counter);
	void (*route_read)(int idx, struct rtl83xx_route *rt);
	void (*route_write)(int idx, struct rtl83xx_route *rt);
	void (*host_route_write)(int idx, struct rtl83xx_route *rt);
	bool (*host_route_hit_get_clear)(int idx);
	int (*l3_setup)(struct rtl838x_switch_priv *priv);
	bool l3_ecmp_offload;
	/* The family driver owns IPv6 prefix-route placement: route_write()
	 * takes the software route id and allocates the table position
	 * itself, instead of the shared code assigning table indices.
	 */
	bool l3_ip6_prefix_by_id;
	void (*set_l3_nexthop)(int idx, u16 dmac_id, u16 interface);
	void (*get_l3_nexthop)(int idx, u16 *dmac_id, u16 *interface);
	u64 (*get_l3_egress_mac)(u32 idx);
	void (*set_l3_egress_mac)(u32 idx, u64 mac);
	int (*find_l3_slot)(struct rtl83xx_route *rt, bool must_exist);
	int (*route_lookup_hw)(struct rtl83xx_route *rt);
	void (*get_l3_router_mac)(u32 idx, struct rtl93xx_rt_mac *m);
	void (*set_l3_router_mac)(u32 idx, struct rtl93xx_rt_mac *m);
	void (*set_l3_egress_intf)(int idx, struct rtl838x_l3_intf *intf);
	/* Multicast route entries share the host-route memory with unicast
	 * ones and are told apart by rtl93xx_route_attr::type, but they are
	 * wider and hash on (source, group) instead of the destination, so
	 * they need their own accessors and slot search.
	 */
	void (*mc_route_read)(int idx, struct rtl83xx_route *rt);
	void (*mc_route_write)(int idx, struct rtl83xx_route *rt);
	int (*mc_find_slot)(struct rtl83xx_route *rt, bool must_exist);
	void (*mc_oif_write)(int idx, const struct rtl83xx_mc_oif *oif);
	void (*mc_oif_read)(int idx, struct rtl83xx_mc_oif *oif);
	/* Print the entry's raw table words. A decode can only show fields
	 * somebody thought to add; the raw words let any bit be checked from a
	 * dump without deriving offsets by hand.
	 */
	void (*mc_route_dump)(int idx, u8 type, struct seq_file *m);
	/* Both families gate multicast routing globally at L3 setup. Mango has a
	 * second, per-ingress-interface gate on top of that, and needs both; a
	 * family with no per-interface gate leaves this unset.
	 */
	void (*l3_mc_intf_enable)(int idx, bool enable);
	bool l3_mc_offload;
	/* Print where this family gates multicast routing, so a dump can show
	 * whether it is armed for an interface rather than only that the driver
	 * asked for it.
	 */
	void (*l3_mc_dump)(struct rtl838x_switch_priv *priv, int intf_id,
			   struct seq_file *m);
	void (*set_distribution_algorithm)(int group, int algoidx, u32 algomask);
	void (*set_receive_management_action)(int port, rma_ctrl_t type, action_type_t action);
	void (*led_init)(struct rtl838x_switch_priv *priv);
	void (*vendor_init)(struct rtl838x_switch_priv *priv);
	void (*vendor_init_dump)(struct rtl838x_switch_priv *priv,
				 struct seq_file *m);
	void (*flow_control_init)(struct rtl838x_switch_priv *priv);
	void (*flow_control_dump)(struct rtl838x_switch_priv *priv, int port,
				  struct seq_file *m);
	void (*qos_init)(struct rtl838x_switch_priv *priv);
};

struct rtl838x_switch_priv {
	/* Switch operation */
	struct dsa_switch *ds;
	struct device *dev;
	u16 id;
	u16 family_id;
	char version;
	struct rtl838x_port ports[57];
	/* Protocol owning each shared VLAN-table VID. CPU-port membership
	 * never claims a VID; values are enum rtldsa_vlan_proto.
	 */
	u8 vlan_proto[MAX_VLANS];
	struct phylink_pcs *pcs[57];
	struct mutex reg_mutex;		/* Mutex for individual register manipulations */
	struct mutex pie_mutex;		/* Mutex for Packet Inspection Engine */
	int link_state_irq;
	int mirror_group_ports[4];
	struct mii_bus *parent_bus;
	const struct rtl838x_reg *r;
	u8 cpu_port;
	/* Port whose egress sampler is armed, or -1. An egress sample's CPU tag
	 * names the port the frame arrived on and leaves PORT_DATA zero, so the
	 * sampled port is only knowable while a single one is armed.
	 */
	int sample_egr_port;
	u8 port_mask;
	u8 port_width;
	u8 port_ignore;
	u64 irq_mask;
	u32 fib_entries;
	int l2_bucket_size;
	u16 n_mst;
	struct dentry *dbgfs_dir;

	/* lags_port_members[] and lag_primary[] are indexed by the one-based
	 * DSA LAG id; slot 0 is unused.
	 */
	/** @lags_port_members: Port (bit) is part of a specific LAG */
	u64 lags_port_members[MAX_LAGS + 1];

	/** @lag_primary: port of a LAG is primary (repesenting) and is added to
	 * the port matrix
	 */
	u32 lag_primary[MAX_LAGS + 1];

	/**
	 * @lag_non_primary: Port (bit) is part of any LAG but not the
	 * first/primary port which needs to be added in the port matrix
	 */
	u64 lag_non_primary;

	/** @lagmembers: Port (bit) is part of any LAG */
	u64 lagmembers;
	struct workqueue_struct *wq;
	struct notifier_block ne_nb;
	struct notifier_block fib_nb;
	bool eee_enabled;
	unsigned long mc_group_bm[MAX_MC_GROUPS >> 5];
	int n_pie_blocks;
	struct rhashtable tc_ht;
	bool tc_ht_initialized;
	unsigned long pie_use_bm[MAX_PIE_ENTRIES >> 5];
	int n_counters;
	unsigned long octet_cntr_use_bm[MAX_COUNTERS >> 5];
	unsigned long packet_cntr_use_bm[MAX_COUNTERS >> 4];
	struct rhltable routes;
	unsigned long route_use_bm[MAX_ROUTES >> 5];
	unsigned long host_route_use_bm[MAX_HOST_ROUTES >> 5];
	int n_route_ids;	/* family clamp on the route id pools */
	int n_host_route_ids;
	int ip6_prefix_hw_cnt;	/* entries programmed in the IPv6 prefix region */
	/* Offloaded multicast routes, keyed on struct rtl83xx_mc_key. The vif
	 * tables are indexed by address family: 0 for IPv4, 1 for IPv6, since
	 * the kernel keeps a separate vif space per family.
	 */
	struct rhashtable mc_routes;
	struct list_head mc_route_list;
	bool mc_routes_initialized;
	struct rtl83xx_mc_vif mc_vifs[2][RTLDSA_MC_MAX_VIFS];
	unsigned long mc_oif_use_bm[MAX_MC_OIFS >> 5];
	int n_mc_oifs;		/* family clamp on the output-interface list pool */
	struct rtl838x_l3_intf *interfaces[MAX_INTERFACES];
	u16 intf_mtus[MAX_INTF_MTUS];
	int intf_mtu_count[MAX_INTF_MTUS];

	/**
	 * @msts: MSTI to HW MST slot allocations. index 0 is for HW slot 1 because CIST is
	 * not stored in @msts
	 */
	struct rtldsa_mst *msts;
	struct delayed_work counters_work;
	struct delayed_work l3_activity_work;

	/**
	 * @counters_lock: Protects the hardware reads happening from MIB
	 * callbacks and the workqueue which reads the data
	 * periodically.
	 */
	struct mutex counters_lock;
};

void rtl838x_dbgfs_init(struct rtl838x_switch_priv *priv);
void rtl930x_dbgfs_init(struct rtl838x_switch_priv *priv);
void rtl930x_storm_control_init(struct rtl838x_switch_priv *priv);
void rtl931x_storm_control_init(struct rtl838x_switch_priv *priv);
int rtl931x_storm_port_rate_set(struct rtl838x_switch_priv *priv, int port,
				enum rtldsa_storm_class class, u32 pps);
u32 rtl931x_storm_port_rate_get(int port, enum rtldsa_storm_class class);
int rtl931x_storm_port_type_set(struct rtl838x_switch_priv *priv, int port,
				enum rtldsa_storm_class class, bool incl_known);
bool rtl931x_storm_port_type_get(int port, enum rtldsa_storm_class class);
void rtl930x_port_max_frame_set(int port, int frame_len);
void rtl931x_port_max_frame_set(int port, int frame_len);
int rtldsa_ip6_mask_len(struct in6_addr *ip6_m);
void rtldsa_net6_mask(int prefix_len, struct in6_addr *ip6_m);
void rtldsa_930x_qos_setup_default_dscp2queue_map(void);
int rtl930x_qos_default_prio_get(int port);
int rtl930x_qos_default_prio_set(struct rtl838x_switch_priv *priv, int port,
				 u8 prio);
int rtl930x_qos_dscp_prio_get(int dscp);
int rtl930x_qos_dscp_prio_set(struct rtl838x_switch_priv *priv, int dscp,
			      u8 prio);
void rtl930x_qos_queue_sched_set(int port, int queue, u8 weight, bool strict);
int rtl930x_qos_sched_algo_get(int port);
void rtl930x_qos_sched_algo_set(int port, bool wrr);
void rtl930x_qos_port_sched_defaults(int port);
void rtl930x_qos_sched_defaults(struct rtl838x_switch_priv *priv);
int rtl83xx_qos_shaper_validate(struct rtl838x_switch_priv *priv, int port,
				int queue, u64 rate_bytes_ps, u32 min_burst,
				u32 max_burst, u32 *rate, u32 *burst);
int rtl930x_qos_queue_shaper_set(struct rtl838x_switch_priv *priv, int port,
				 int queue, u64 rate_bytes_ps, u32 burst);
int rtl930x_qos_port_shaper_set(struct rtl838x_switch_priv *priv, int port,
				u64 rate_bytes_ps, u32 burst);
int rtl930x_qos_swred_set(struct rtl838x_switch_priv *priv, int port, int queue,
			  u32 min_pages, u32 max_pages, u8 probability);
void rtl930x_qos_swred_disable(struct rtl838x_switch_priv *priv, int port);
void rtl930x_qos_swred_get(struct rtl838x_switch_priv *priv, int port,
			   struct rtl838x_qos_swred_state *state);
void rtldsa_931x_qos_setup_default_dscp2queue_map(void);
int rtl931x_qos_default_prio_get(int port);
int rtl931x_qos_default_prio_set(struct rtl838x_switch_priv *priv, int port,
				 u8 prio);
int rtl931x_qos_dscp_prio_get(int dscp);
int rtl931x_qos_dscp_prio_set(struct rtl838x_switch_priv *priv, int dscp,
			      u8 prio);
void rtl931x_qos_queue_sched_set(int port, int queue, u8 weight, bool strict);
int rtl931x_qos_sched_algo_get(int port);
void rtl931x_qos_sched_algo_set(int port, bool wrr);
void rtl931x_qos_port_sched_defaults(int port);
void rtl931x_qos_sched_defaults(struct rtl838x_switch_priv *priv);
int rtl931x_qos_queue_shaper_set(struct rtl838x_switch_priv *priv, int port,
				 int queue, u64 rate_bytes_ps, u32 burst);
int rtl931x_qos_port_shaper_set(struct rtl838x_switch_priv *priv, int port,
				u64 rate_bytes_ps, u32 burst);
int rtl931x_qos_swred_set(struct rtl838x_switch_priv *priv, int port, int queue,
			  u32 min_pages, u32 max_pages, u8 probability);
void rtl931x_qos_swred_disable(struct rtl838x_switch_priv *priv, int port);
void rtl931x_qos_swred_get(struct rtl838x_switch_priv *priv, int port,
			   struct rtl838x_qos_swred_state *state);

void rtldsa_counters_lock_register(struct rtl838x_switch_priv *priv, int port)
	__acquires(&priv->ports[port].counters.lock);
void rtldsa_counters_unlock_register(struct rtl838x_switch_priv *priv, int port)
	__releases(&priv->ports[port].counters.lock);
void rtldsa_counters_lock_table(struct rtl838x_switch_priv *priv, int port)
	__acquires(&priv->counters_lock);
void rtldsa_counters_unlock_table(struct rtl838x_switch_priv *priv, int port)
	__releases(&priv->ports[port].counters.lock);

void rtldsa_update_counters_atomically(struct rtl838x_switch_priv *priv, int port);

extern int rtldsa_max_available_queue[];
extern int rtldsa_default_queue_weights[];

#endif /* _RTL838X_H */
