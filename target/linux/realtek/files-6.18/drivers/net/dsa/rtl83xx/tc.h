/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_TC_H
#define _OTTO_TC_H

#include <linux/types.h>

struct dsa_switch;
struct flow_action_entry;
struct flow_cls_offload;
struct net_device;
struct sk_buff;
struct rtl838x_switch_priv;

int rtldsa_tc_init(struct rtl838x_switch_priv *priv);
void rtldsa_tc_cleanup(struct rtl838x_switch_priv *priv);

int rtldsa_cls_flower_add(struct dsa_switch *ds, int port,
			  struct flow_cls_offload *cls, bool ingress);
int rtldsa_cls_flower_del(struct dsa_switch *ds, int port,
			  struct flow_cls_offload *cls, bool ingress);
int rtldsa_cls_flower_stats(struct dsa_switch *ds, int port,
			    struct flow_cls_offload *cls, bool ingress);

int rtldsa_930x_port_rate_police_add(struct dsa_switch *ds, int port,
				     const struct flow_action_entry *act,
				     bool ingress);
int rtldsa_930x_port_rate_police_del(struct dsa_switch *ds, int port,
				     struct flow_cls_offload *cls,
				     bool ingress);
int rtldsa_930x_egress_shaper_set(struct rtl838x_switch_priv *priv, int port, int queue,
				  u64 rate_bytes_ps, u32 burst);
int rtldsa_930x_storm_set(int port, enum rtldsa_storm_type type, u64 rate_pkt_ps, u32 burst_pkt);
int rtldsa_930x_sample_set(int port, u32 rate);
void rtldsa_sample_rx(struct net_device *conduit, int port, struct sk_buff *skb);
int rtldsa_931x_port_rate_police_add(struct dsa_switch *ds, int port,
				     const struct flow_action_entry *act,
				     bool ingress);
int rtldsa_931x_port_rate_police_del(struct dsa_switch *ds, int port,
				     struct flow_cls_offload *cls,
				     bool ingress);

#endif /* _OTTO_TC_H */
