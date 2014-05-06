/**
 * Linux interface for Tempesta FW (FireWall and/or FrameWork).
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
#ifndef __TEMPESTA_FW_H__
#define __TEMPESTA_FW_H__

#include <net/sock.h>

typedef struct {
	int (*sock_tcp_rcv)(struct sock *sk, struct sk_buff *skb);
} TempestaOps;

void tempesta_register_ops(TempestaOps *tops);
void tempesta_unregister_ops(TempestaOps *tops);

#endif /* __TEMPESTA_FW_H__ */

