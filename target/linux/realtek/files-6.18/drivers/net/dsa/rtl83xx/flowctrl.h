/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _OTTO_FLOWCTRL_H
#define _OTTO_FLOWCTRL_H

struct rtl838x_switch_priv;
struct rtldsa_red_cfg;

void rtldsa_930x_flowctrl_init(struct rtl838x_switch_priv *priv);
void rtldsa_930x_red_queue_set(int queue, const struct rtldsa_red_cfg *cfg);
void rtldsa_930x_red_port_set(int port, bool enable);
void rtldsa_931x_red_queue_set(int queue, const struct rtldsa_red_cfg *cfg);
void rtldsa_931x_red_port_set(int port, bool enable);

#endif /* _OTTO_FLOWCTRL_H */
