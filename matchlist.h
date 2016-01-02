/* matchlist.h - circular list of matches declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _MATCHLIST_H
#define _MATCHLIST_H

struct matchlist {
	int nelem;		/* number of items list can hold */
	int full;		/* matchlist is full */
	int first, last;	/* idx of first and last item */
	int *elem;		/* the actual items */
};

int matchlist_init(struct matchlist *, int);
void matchlist_clear(struct matchlist *);

int matchlist_empty(struct matchlist);
int matchlist_full(struct matchlist);
int matchlist_size(struct matchlist);
int matchlist_get(struct matchlist, int);
void matchlist_add(struct matchlist *, int);
void matchlist_drop_first(struct matchlist *);

#define UNIT_TESTING

#ifdef UNIT_TESTING
	void matchlist_print(struct matchlist list);
#endif

#endif /* _MATCHLIST_H */
