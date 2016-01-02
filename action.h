/* action.h - f2b_action handling declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _ACTION_H
#define _ACTION_H

#include "matchlist.h"	/* struct matchlist */
#include "substr.h"	/* struct substr */

struct f2b_match {
	struct substr subs;
	char banned;
	struct matchlist mlist;
};

struct f2b_action {
	regex_t *re;
	regex_t *re_dec; //decrement counter
	int limit;
	int timeout; /* seconds */
	struct substr ban, unban;
	int nmatch; /* length of match array */
	struct f2b_match *match;
	int cmp_index; /*compare group index*/
};

struct f2b_action *get_new_action(void);
int get_min_action_timeout(void);
int action_copy_empty(struct f2b_action *, struct f2b_action *);
struct f2b_action *action_iterate(int);

#endif /* _ACTION_H */
