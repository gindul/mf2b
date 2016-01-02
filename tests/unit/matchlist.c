/* matchlist.c - circular list of matches unit test
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <assert.h>
#include <stdio.h>

#define UNIT_TESTING
#include "../../matchlist.c"

int main(void)
{
	struct matchlist mlist;

	assert(!matchlist_init(&mlist, 3));
	matchlist_print(mlist);
	assert(matchlist_empty(mlist));
	assert(!matchlist_full(mlist));
	matchlist_add(&mlist, 1);
	matchlist_print(mlist);
	assert(matchlist_size(mlist) == 1);
	assert(!matchlist_full(mlist));

	matchlist_add(&mlist, 2);
	matchlist_print(mlist);
	assert(!matchlist_full(mlist));
	matchlist_add(&mlist, 3);
	matchlist_print(mlist);
	assert(matchlist_size(mlist) == 3);
	assert(matchlist_full(mlist));
	assert(matchlist_get(mlist, 0) == 1);
	assert(matchlist_get(mlist, 1) == 2);
	assert(matchlist_get(mlist, 2) == 3);

	matchlist_drop_first(&mlist);
	assert(matchlist_size(mlist) == 2);
	assert(!matchlist_full(mlist));
	matchlist_drop_first(&mlist);
	assert(matchlist_size(mlist) == 1);
	matchlist_drop_first(&mlist);
	assert(matchlist_size(mlist) == 0);
	assert(matchlist_empty(mlist));

	return 0;
}
