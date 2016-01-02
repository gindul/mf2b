/* action.c - f2b_action unit test
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <assert.h>
#include <stdio.h>

#define UNIT_TESTING
#include "../../action.c"
#include "../../wordsplit.c"

int main(void)
{
	struct f2b_action *a, *b, *c;

	a = get_new_action();
	b = get_new_action();
	c = get_new_action();
	assert(a != b);
	assert(b != c);
	assert(a != c);

	a->timeout = 1;
	b->timeout = 2;
	c->timeout = 3;
	assert(get_min_action_timeout() == 1);

	a->limit = 3;
	assert(!action_copy_empty(b, a));
	assert(b->limit == 3);

	assert(!action_iterate(1));
	/* new actions inserted at second position, not the end
	 * therefore order here is a bit unintuitive */
	assert(action_iterate(0) == a);
	assert(action_iterate(0) == c);
	assert(action_iterate(0) == b);
	assert(!action_iterate(0));
	return 0;
}
