/* substr.c - substring handling routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "logging.h"
#include "substr.h"

void substr_free(struct substr *ss)
{
	while (ss->nstr)
		free(ss->str[--ss->nstr]);
	if (ss->str) {
		free(ss->str);
		ss->str = NULL;
	}
}

/** substr_init: initialise a struct substr object
 * - must be used on either a new variable,
 *   or one that substr_free() has been called upon before
 *   otherwise memory leakage will occur
 * @param ss: pointer to struct substr to initialise
 * @return: 0 on success, 1 if memory allocation failed */
int substr_init(struct substr *ss)
{
	ss->nstr = 0;
	if (!(ss->str = calloc(1, sizeof(char *))))
		return 1;
	return 0;
}

/** substr_append: append given char pointer to struct substr
 * @param ss: struct substr to append word to,
 *            must have been initialised before
 * @param word: char pointer to append to @ss
 * @return: 0 on success, 1 if memory allocation fails */
int substr_append(struct substr *ss, const char *word)
{
	char **tmp = ss->str;

	ss->str = realloc(ss->str, (++ss->nstr + 1) * sizeof(char *));
	if (!ss->str) {
		ss->nstr--;
		ss->str = tmp;
		return 1;
	}
	ss->str[ss->nstr] = NULL; /* terminating NULL-pointer */
	ss->str[ss->nstr - 1] = word ? strdup(word) : NULL;
	return 0;
}

static char *idx2sub(const struct substr *ss, const int idx)
{
	if (idx < 0 || idx > ss->nstr)
		return NULL;
	return ss->str[idx];
}

static int astrcat(char **buf, int *buflen, const char *addbuf)
{
	if (!addbuf)
		return 1;

	if (!(*buflen)) {
		*buflen = strlen(addbuf) + 1;
		*buf = calloc(*buflen, sizeof(char *));
		if (!*buf)
			goto out_no_mem;
	}
	if (strlen(*buf) + strlen(addbuf) + 1 > *buflen) {
		*buflen = strlen(*buf) + strlen(addbuf) + 1;
		*buf = safe_realloc(*buf, *buflen);
		if (!*buf)
			goto out_no_mem;
	}
	strcat(*buf, addbuf);
	return 0;
out_no_mem:
	err("%s: out of core", __func__);
	return 1;
}

char *substr_replace(const char *fmt, const struct substr *ss)
{
	static char *buf = NULL;
	static int buflen = 0;
	const char *fmtp;
	char *p, *p2, c, *endptr;
	int idx;

	if (!buflen) {
		buflen = 256;
		buf = malloc(buflen);
	}
	buf[0] = '\0';

	fmtp = fmt;

find_dollar:
	p = strchrnul(fmtp, '$');
	c = *p;
	*p = '\0';
	astrcat(&buf, &buflen, fmtp);
	*p = c;
	if (*p != '$')
		return buf;
	if (p[1] == '$') {		/* '$$' -> '$' */
		fmtp = p + 1;		/* XXX: does this really work? */
		goto find_dollar;
	}
	if (p[1] == '{') {		/* '${...}' */
		p2 = strchr(p + 2, '}');
		if (!p2)
			goto err_out;
		*p2 = '\0';
		idx = strtol(p + 2, &endptr, 0);
		if (*endptr)
			goto err_out;
		*p2 = '}';
		fmtp = p2 + 1;
	} else {		/* $... */
		p2 = p + 1;
		while (isdigit(*p2))
			p2++;
		c = *p2;
		*p2 = '\0';
		idx = strtol(p + 1, &endptr, 0);
		if (*endptr)
			goto err_out;
		*p2 = c;
		fmtp = p2;
	}

	if ((idx - 1) > ss->nstr)
		goto err_out;
	astrcat(&buf, &buflen, idx2sub(ss, idx - 1));
	goto find_dollar;
err_out:
	free(buf);
	dbg("%s: substring replace failed: fmt = '%s'",
			__func__, fmt);
	for (idx = 0; idx < ss->nstr; idx++)
		dbg("\tsubs[%d] = '%s'", idx + 1, idx2sub(ss, idx));
	return NULL;
}

/** substr_extract: extract parts of @str defined by @match
 * @param ss: Pointer to struct substr to extract into. Must be either new
 *            or substr_free() has been called upon, otherwise memory leakage
 *            will occur.
 * @param str: String to extract substrings from.
 * @param match: regmatch_t array specifying the substring locations,
 *               terminated by an entry with offsets of -1.
 * @return: 0 on success, 1 if memory allocation fails */
int substr_extract(struct substr *ss, const char *str, const regmatch_t *match)
{
	int i;

	for (i = 1; i <= MAX_SUBS && match[i].rm_so != -1; i++)
		;
	ss->nstr = i - 1;

	/* allocate one more so there is a terminating null pointer at the end */
	if (!(ss->str = calloc(ss->nstr + 1, sizeof(char *)))) {
		ss->nstr = 0;
		return 1;
	}
	for (i = 1; i <= MAX_SUBS && match[i].rm_so != -1; i++)
		ss->str[i - 1] = strndup(str + match[i].rm_so,
		                               match[i].rm_eo - match[i].rm_so);
	return 0;
}

int substr_match(const struct substr *ss, const char *str, const regmatch_t *match, int cmd_index)
{
	int i;
	for (i = 0; i < ss->nstr ; i++)
	{
		dbg("%s:[%i] = %s", __func__, i, ss->str[i]);
	}
	dbg("%s: str = %s", __func__, str);
	/* first, make sure number of substrings match */
	for (i = 1; i <= MAX_SUBS + 1 && match[i].rm_so != -1; i++)
	{
		dbg("%s: i=%i rm_so=%i rm_eo = %i nstr=%i", __func__, i, match[i].rm_so, match[i].rm_eo, ss->nstr);
	}
	//dbg("%s: i=%i rm_so=%i rm_eo = %i nstr=%i", __func__, i, match[i].rm_so, match[i].rm_eo, ss->nstr);
	if (ss->nstr != i - 1)
		return 0;

	/* compare each substring */
	dbg("%s: cmd_index=%i", __func__, cmd_index);
	if(cmd_index <= 0)
	{
		for (i = 1; i <= ss->nstr; i++) {
			int slen = match[i].rm_eo - match[i].rm_so;
			if (strlen(ss->str[i - 1]) != slen)
				return 0;
			if (strncmp(ss->str[i - 1], str + match[i].rm_so, slen))
				return 0;
		}
	}
	else
	{
		if(cmd_index > ss->nstr)
		{
			dbg("%s: Error: cmd_index(%i) > ss->nstr(%i)", __func__, cmd_index, ss->nstr);
			return 0;
		}

		int slen = match[cmd_index].rm_eo - match[cmd_index].rm_so;
		if (strlen(ss->str[cmd_index - 1]) != slen)
			return 0;
		if (strncmp(ss->str[cmd_index - 1], str + match[cmd_index].rm_so, slen))
		{
			dbg("%s: ss=%s > str=%s", ss->str[cmd_index - 1], str + match[cmd_index].rm_so);
			return 0;
		}
	}
	return 1;
}

/** line_to_words: split up given line into words
 * - respect double quotes
 * - eliminate escaping of double quotes
 * @param line: string to extract words from
 * @param words: pointer to string array being filled
 * @param nwords: final length of array @words points at */
int substr_wordsplit(struct substr *ss, const char *line)
{
	char *_line;
	int in_word = 0, in_quotes = 0;
	char *wstart, *p;

	if (!line)
		return 1;
	if (substr_init(ss) || !(_line = strdup(line)))
		goto err_no_mem;
	for (wstart = p = _line; *p; p++) {
		if (*p == '\\' &&
		    (*(p + 1) == '"' ||
		     *(p + 1) == '\\')) {
			/* eliminate escaped quotes / double escapes */
			memmove(p, p + 1, strlen(p + 1) + 1);
			continue;
		}
		if (!in_word) {
			/* find start of word */
			if (isspace(*p))
				continue;
			if (*p == '"') {
				in_quotes = 1;
				p++;
			}
			wstart = p;
			in_word = 1;
		} else {
			/* find end of word */
			if (in_quotes && *p != '"')
				continue;
			if (!in_quotes && !isspace(*p))
				continue;
			in_word = 0;
			in_quotes = 0;
			*p = '\0';
			dbg("%s: appending word '%s'", __func__, wstart);
			if (substr_append(ss, wstart))
				goto err_no_mem_free;
			wstart = p + 1;
		}
	}
	if (wstart != p) {
		dbg("%s: appending word '%s'", __func__, wstart);
		if (substr_append(ss, wstart))
			goto err_no_mem_free;
	}
	free(_line);
	return 0;
err_no_mem_free:
	free(_line);
err_no_mem:
	err("%s: out of core", __func__);
	return 1;
}

int substr_copy(struct substr *ss_dst, struct substr *ss_src)
{
	int i;

	if (substr_init(ss_dst))
		return 1;

	for (i = 0; i < ss_src->nstr; i++)
		substr_append(ss_dst, ss_src->str[i]);
	return 0;
}

int substr_copy_replace(struct substr *ss_dst,
                        struct substr *ss_src,
                        struct substr *repl)
{
	int i;

	if (substr_init(ss_dst))
		return 1;

	for (i = 0; i < ss_src->nstr; i++) {
		if (substr_append(ss_dst,
		                  substr_replace(ss_src->str[i], repl))) {
			substr_free(ss_dst);
			return 1;
		}
	}
	return 0;
}

char *substr_printable(struct substr *ss)
{
	char *buf = NULL;
	int buflen = 0;

	astrcat(&buf, &buflen, "{ ");
	if (ss->nstr) {
		astrcat(&buf, &buflen, "\"");
		astrcat(&buf, &buflen, ss->str[0]);
		astrcat(&buf, &buflen, "\"");
		for (int i = 1; i < ss->nstr; i++) {
			astrcat(&buf, &buflen, ", \"");
			astrcat(&buf, &buflen, ss->str[i]);
			astrcat(&buf, &buflen, "\"");
		}
	}
	astrcat(&buf, &buflen, " }");
	return buf;
}
