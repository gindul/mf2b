/* matchlist.c - circular list of matches routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <stdio.h>
#include <stdlib.h>
#include "matchlist.h"
#include "logging.h"

static int next_idx(struct matchlist *list, int idx)
{
	return (idx + 1) % list->nelem;
}

int matchlist_init(struct matchlist *list, int nelem)
{
	if ((list->elem = calloc(nelem, sizeof(int))) == NULL)
		return 1;
	list->nelem = nelem;
	list->full = 0;
	list->first = 0;
	list->last = 0;
	return 0;
}

void matchlist_clear(struct matchlist *list)
{
	if (list->elem) {
		free(list->elem);
		list->elem = NULL;
	}
}

int matchlist_empty(struct matchlist list)
{
	return (list.first == list.last) && !list.full;
}

int matchlist_full(struct matchlist list)
{
	return list.full;
}

int matchlist_size(struct matchlist list)
{
	if (list.full)
		return list.nelem;
	if (list.last < list.first)
		return list.last + list.nelem - list.first;
	return list.last - list.first;
}

int matchlist_get(struct matchlist list, int idx)
{
	idx = (list.first + idx) % list.nelem;
	return list.elem[idx];
}

void matchlist_add(struct matchlist *list, int elem)
{
	if (matchlist_full(*list))
		list->first = next_idx(list, list->first);
	list->elem[list->last] = elem;
	list->last = next_idx(list, list->last);
	if (list->last == list->first)
		list->full = 1;
}

void matchlist_drop_first(struct matchlist *list)
{
	if (!matchlist_empty(*list))
		list->first = next_idx(list, list->first);
	list->full = 0;
}

#ifdef UNIT_TESTING
void matchlist_print(struct matchlist list)
{
	dbg("{ nelem = %d, full = %d, first = %d, last = %d, elem = { ",
			list.nelem, list.full, list.first, list.last);
	while (list.nelem-- > 0)
		dbg("%d, ", *(list.elem++));
	dbg("} }\n");
}
#endif
