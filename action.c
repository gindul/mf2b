/* action.c - f2b_action handling routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "action.h"
#include "substr.h"

static struct action_list_s {
	struct action_list_s *next;
	struct f2b_action action;
} *action_list = NULL;

static struct action_list_s *allocate_listitem(void)
{
	struct action_list_s *item;

	item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	return item;
}

struct f2b_action *get_new_action(void)
{
	struct action_list_s *tmp;

	if (!action_list) {
		action_list = allocate_listitem();
		return (action_list ? &action_list->action : NULL);
	}
	tmp = action_list->next;
	action_list->next = allocate_listitem();
	if (!action_list->next) {
		action_list->next = tmp;
		return NULL;
	}
	action_list->next->next = tmp;
	return &action_list->next->action;
}

int get_min_action_timeout(void)
{
	struct action_list_s *item = action_list;
	int tval;

	if (!item)
		return 0;
	tval = item->action.timeout;

	for (; item->next; item = item->next) {
		if (tval > item->next->action.timeout)
			tval = item->next->action.timeout;
	}
	return tval;
}

int action_copy_empty(struct f2b_action *dst, struct f2b_action *src)
{
#define ASSIGN_IF_ZERO(f) if (!dst->f) dst->f = src->f
	ASSIGN_IF_ZERO(re);
	ASSIGN_IF_ZERO(re_dec);
	ASSIGN_IF_ZERO(limit);
	ASSIGN_IF_ZERO(timeout);
	if (!dst->ban.nstr)
		substr_copy(&dst->ban, &src->ban);
	if (!dst->unban.nstr)
		substr_copy(&dst->unban, &src->unban);
#undef ASSIGN_IF_ZERO
	return 0;
}

struct f2b_action *action_iterate(int reset)
{
	static struct action_list_s *item = NULL;

	if (reset) {
		item = NULL;
		return NULL;
	} else if (!item) {
		item = action_list;
	} else {
		item = item->next;
	}
	return item ? &item->action : NULL;
}

