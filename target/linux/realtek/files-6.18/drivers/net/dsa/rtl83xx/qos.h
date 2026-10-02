/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_QOS_H
#define _OTTO_QOS_H

#include <linux/types.h>
#include <net/pkt_cls.h>

struct dsa_switch;
struct rtl838x_switch_priv;

u32 rtldsa_838x_get_egress_rate(struct rtl838x_switch_priv *priv, int port);
int rtldsa_838x_set_egress_rate(struct rtl838x_switch_priv *priv, int port, u32 rate);
u32 rtldsa_839x_get_egress_rate(struct rtl838x_switch_priv *priv, int port);
int rtldsa_839x_set_egress_rate(struct rtl838x_switch_priv *priv, int port, u32 rate);

void rtldsa_838x_qos_init(struct rtl838x_switch_priv *priv);
void rtldsa_839x_qos_init(struct rtl838x_switch_priv *priv);
void rtldsa_930x_qos_init(struct rtl838x_switch_priv *priv);
void rtldsa_931x_qos_init(struct rtl838x_switch_priv *priv);
void rtldsa_930x_queue_sched_set(int port, int queue, u32 weight, bool strict);
void rtldsa_931x_queue_sched_set(int port, int queue, u32 weight, bool strict);

void rtldsa_qos_setup(struct dsa_switch *ds);
int rtldsa_port_get_default_prio(struct dsa_switch *ds, int port);
int rtldsa_port_set_default_prio(struct dsa_switch *ds, int port, u8 prio);
int rtldsa_port_get_dscp_prio(struct dsa_switch *ds, int port, u8 dscp);
int rtldsa_port_add_dscp_prio(struct dsa_switch *ds, int port, u8 dscp, u8 prio);
int rtldsa_port_del_dscp_prio(struct dsa_switch *ds, int port, u8 dscp, u8 prio);
int rtldsa_port_get_apptrust(struct dsa_switch *ds, int port, u8 *sel, int *nsel);
int rtldsa_port_set_apptrust(struct dsa_switch *ds, int port, const u8 *sel, int nsel);

int rtldsa_port_setup_tc(struct dsa_switch *ds, int port, enum tc_setup_type type,
			 void *type_data);

#endif /* _OTTO_QOS_H */
