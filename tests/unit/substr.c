/* substrings.c - substrings unit test
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>

#define UNIT_TESTING
#include "../../logging.c"
#include "../../substr.c"

char *strings[][4] = {
	{ "bla", "blub", "plop", NULL },
	{ "bl a", "blub", "bl\"op", NULL },
};

struct {
	const char *str;
	regmatch_t match[5];
	struct substr subs;
} testobj[] = {{
	.str = "bla blub plop",
	.match = {{
		.rm_so = 0,
		.rm_eo = 13,
	}, {
		.rm_so = 0,
		.rm_eo = 3,
	}, {
		.rm_so = 4,
		.rm_eo = 8,
	}, {
		.rm_so = 9,
		.rm_eo = 13,
	}, {
		.rm_so = -1,
		.rm_eo = -1,
	}},
	.subs = {
		.nstr = 3,
		.str = strings[0],
	},
}, {
	.str = NULL
}};

struct {
	const char *line;
	struct substr subs;
} testobj2[] = {{
	.line = "bla blub plop",
	.subs = {
		.nstr = 3,
		.str = strings[0],
	}
}, {
	.line = "\"bl a\" blub bl\\\"op",
	.subs = {
		.nstr = 3,
		.str = strings[1],
	}
}, { .line = NULL }};

static int substrcmp(struct substr *a, struct substr *b)
{
	int i;

	if (a->nstr != b->nstr) {
		err("%s: string counts do not match (%d != %d)", __func__, a->nstr, b->nstr);
		return 1;
	}
	for (i = 0; i < a->nstr; i++) {
		if (strcmp(a->str[i], b->str[i])) {
			err("%s: strings at index %d do not match ('%s' != '%s')",
					__func__, i, a->str[i], b->str[i]);
			return 1;
		}
	}
	if (a->str[i] != b->str[i]) {
		err("%s: terminator not equal (%lx != %lx)",
				__func__, a->str[i], b->str[i]);
		return 1;
	}
	return 0;
}

int main(void)
{
	int i;
	struct substr subs;

#if 0
	add_log_level(4);
	set_log_output("unit", "stderr");
#endif

	for (i = 0; testobj[i].str; i++) {
		assert(!substr_extract(&subs, testobj[i].str, testobj[i].match));
		assert(!substrcmp(&subs, &testobj[i].subs));
		substr_free(&subs);
	}

	for (i = 0; testobj2[i].line; i++) {
		assert(!substr_wordsplit(&subs, testobj2[i].line));
		assert(!substrcmp(&subs, &testobj2[i].subs));
		substr_free(&subs);
	}

	return 0;
}
