/**
 *		Tempesta FW
 *
 * Copyright (C) 2012-2014 NatSys Lab. (info@natsys-lab.com).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <linux/security.h>
#include <linux/spinlock.h>

#include <linux/tempesta_fw.h>

static TempestaOps __rcu *tempesta_ops;
static DEFINE_SPINLOCK(tops_lock);

void
tempesta_register_ops(TempestaOps *tops)
{
	spin_lock(&tops_lock);

	BUG_ON(tempesta_ops);

	rcu_assign_pointer(tempesta_ops, tops);

	spin_unlock(&tops_lock);
}
EXPORT_SYMBOL(tempesta_register_ops);

void
tempesta_unregister_ops(TempestaOps *tops)
{
	spin_lock(&tops_lock);

	BUG_ON(tempesta_ops != tops);

	rcu_assign_pointer(tempesta_ops, NULL);

	spin_unlock(&tops_lock);

	/*
	 * tempesta_ops is called in softirq only, so if there are some users
	 * of the structures then they are active on their CPUs.
	 * After the below we can be sure that nobody refers @tops and we can
	 * go forward and destroy it.
	 */
	synchronize_rcu();
}
EXPORT_SYMBOL(tempesta_unregister_ops);

static int
tempesta_sock_tcp_rcv(struct sock *sk, struct sk_buff *skb)
{
	int r = 0;
	TempestaOps *tops;

	rcu_read_lock();
	tops = rcu_dereference(tempesta_ops);
	if (likely(tops))
		r = tops->sock_tcp_rcv(sk, skb);
	rcu_read_unlock();

	return r;
}

static struct security_operations tempesta_sec_ops __read_mostly = {
	.socket_sock_rcv_skb	= tempesta_sock_tcp_rcv,
};

static __init int
tempesta_init(void)
{
	if (register_security(&tempesta_sec_ops))
		panic("Tempesta: kernel security registration failed.\n");

	return 0;
}

security_initcall(tempesta_init);
