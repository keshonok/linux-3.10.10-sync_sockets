/**
 *		Tempesta Memory Reservation
 *
 * Copyright (C) 2015 Tempesta Technologies.
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
#include <linux/gfp.h>
#include <linux/tempesta.h>
#include <linux/topology.h>

#define MAX_PGORDER		25	/* 128GB per one table */
#define MIN_PGORDER		9	/* 2MB - one extent */
#define DEFAULT_PGORDER		17	/* 512MB */
#define PGCHUNKS		(1 << (MAX_PGORDER - MAX_ORDER + 1))

static int pgorder = DEFAULT_PGORDER;
static TempestaMapping map[MAX_NUMNODES];

static int __init
tempesta_setup_pages(char *str)
{
	get_option(&str, &pgorder);
	if (pgorder < MIN_PGORDER)
		pgorder = MIN_PGORDER;

	return 1;
}
__setup("tempesta_pages=", tempesta_setup_pages);

void __init
tempesta_reserve_pages(void)
{
	static struct page *pg_arr[PGCHUNKS];
	struct page *p = NULL;
	int i, node, n, chunk_order;

	chunk_order = min_t(int, MAX_ORDER - 1, pgorder);
	n = 1 << (pgorder - chunk_order);

	/*
	 * Linux buddy allocator returns blocks aligned on the their size
	 * and returned addresses grow from top to bottom.
	 */
	for_each_node_with_cpus(node) {
		for (i = n - 1; i >= 0; --i) {
			p = alloc_pages_node(node, GFP_KERNEL|__GFP_ZERO,
					     chunk_order);
			if (!p) {
				pr_err("Tempesta: cannot allocate page set"
				       " at node %d\n", node);
				goto err;
			}
			if (i < n - 1
			    && page_address(p) + PAGE_SIZE * (1 << chunk_order)
			       != page_address(pg_arr[i + 1]))
			{
				pr_err("Tempesta: sparse page set alloc:"
				       " %p:%p %lx\n", page_address(p),
				     page_address(pg_arr[i + 1]),
				     PAGE_SIZE * (1 << chunk_order));
				goto err;
			}

			pr_info("Tempesta: alloc %lu pages starting at address"
				" %p at node %d\n",
			     1UL << chunk_order, page_address(p), node);

			pg_arr[i] = p;
		}
		BUG_ON(map[node].addr);
		map[node].addr = (unsigned long)page_address(p);
		map[node].pages = 1 << pgorder;
	}

	return;
err:
	for (--node; node >= 0; --node)
		for (i = 0; i < n; ++i)
			if (pg_arr[i])
				__free_pages(pg_arr[i], chunk_order);
}

int
tempesta_get_mapping(int node, TempestaMapping **tm)
{
	if (!map[node].addr)
		return -ENOMEM;

	*tm = &map[node];

	return 0;
}
EXPORT_SYMBOL(tempesta_get_mapping);

