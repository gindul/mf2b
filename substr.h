/* substr.h - substring handling declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _SUBSTRINGS_H
#define _SUBSTRINGS_H

#include <regex.h>		/* regmatch_t */

struct substr {
	int nstr;		/* number of strings, not including the terminator */
	char **str;		/* NULL-pointer terminated array of strings */
};

void substr_free(struct substr *);
int substr_init(struct substr *);
int substr_append(struct substr *, const char *);
char *substr_replace(const char *, const struct substr *);
int substr_extract(struct substr *, const char *, const regmatch_t *);
int substr_match(const struct substr *, const char *, const regmatch_t *);
int substr_wordsplit(struct substr *, const char *);
int substr_copy(struct substr *, struct substr *);
int substr_copy_replace(struct substr *, struct substr *, struct substr *);
char *substr_printable(struct substr *);

#endif /* _SUBSTRINGS_H */
