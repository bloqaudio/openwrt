/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_FLOWCTRL_H
#define _OTTO_FLOWCTRL_H

struct otto_dsa_priv;
struct tc_red_qopt_offload_params;

void rtldsa_930x_flowctrl_init(struct otto_dsa_priv *priv);
int rtldsa_930x_red_set(struct otto_dsa_priv *priv, int port, int queue,
			const struct tc_red_qopt_offload_params *p);

#endif /* _OTTO_FLOWCTRL_H */
