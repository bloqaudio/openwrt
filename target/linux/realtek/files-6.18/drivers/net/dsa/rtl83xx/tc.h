/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_TC_H
#define _OTTO_TC_H

#include <linux/types.h>

struct dsa_switch;
struct flow_action_entry;
struct flow_cls_offload;
struct net_device;
struct sk_buff;
struct otto_dsa_priv;

int otto_tc_init(struct otto_dsa_priv *priv);
void otto_tc_cleanup(struct otto_dsa_priv *priv);

int otto_tc_cls_flower_add(struct dsa_switch *ds, int port,
			   struct flow_cls_offload *cls, bool ingress);
int otto_tc_cls_flower_del(struct dsa_switch *ds, int port,
			   struct flow_cls_offload *cls, bool ingress);
int otto_tc_cls_flower_stats(struct dsa_switch *ds, int port,
			     struct flow_cls_offload *cls, bool ingress);

int otto_tc_930x_port_rate_police_add(struct dsa_switch *ds, int port,
				      const struct flow_action_entry *act,
				      bool ingress);
int otto_tc_930x_port_rate_police_del(struct dsa_switch *ds, int port,
				      struct flow_cls_offload *cls,
				      bool ingress);
int otto_tc_930x_egress_shaper_set(struct otto_dsa_priv *priv, int port, int queue,
				   u64 rate_bytes_ps, u32 burst);
int otto_tc_930x_storm_set(int port, enum rtldsa_storm_type type, u64 rate_pkt_ps, u32 burst_pkt);
int otto_tc_930x_sample_set(int port, u32 rate);
void otto_tc_sample_rx(struct net_device *conduit, int port, struct sk_buff *skb);
int otto_tc_931x_egress_shaper_set(struct otto_dsa_priv *priv, int port, int queue,
				   u64 rate_bytes_ps, u32 burst);
int otto_tc_931x_port_rate_police_add(struct dsa_switch *ds, int port,
				      const struct flow_action_entry *act,
				      bool ingress);
int otto_tc_931x_port_rate_police_del(struct dsa_switch *ds, int port,
				      struct flow_cls_offload *cls,
				      bool ingress);

#endif /* _OTTO_TC_H */
