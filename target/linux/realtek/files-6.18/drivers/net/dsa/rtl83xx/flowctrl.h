/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_FLOWCTRL_H
#define _OTTO_FLOWCTRL_H

struct rtl838x_switch_priv;
struct tc_red_qopt_offload_params;

void rtldsa_930x_flowctrl_init(struct rtl838x_switch_priv *priv);
int rtldsa_930x_red_set(struct rtl838x_switch_priv *priv, int port, int queue,
			const struct tc_red_qopt_offload_params *p);

#endif /* _OTTO_FLOWCTRL_H */
