/* config.c - mf2b config parser routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "action.h"
#include "helper.h"
#include "logging.h"
#include "matchlist.h"
#include "substr.h"
#include "mf2b.h"

/** add_desc: extend f2b_desc array with given files, assigning given f2b_action to them
 * @param desc: pointer to f2b_desc array to extend
 * @param ndesc: final length of f2b_desc array behind @desc
 * @param act: f2b_action to assign to given files
 * @param files: struct substr containing file names, will be cleared on the go
 *               (like if substr_free() had been called upon it */
static int add_desc(struct f2b_desc **desc, int *ndesc,
		struct f2b_action *act, struct substr *files)
{
	int i;

	while (files->nstr--) {
		/* check if there already is a descriptor for the file */
		for (i = 0; i < *ndesc; i++) {
			if (!strcmp((*desc)[i].fname, files->str[files->nstr]))
				break;
		}
		/* allocate new descriptor if not found */
		if (i == *ndesc) {
			*desc = realloc(*desc, ++(*ndesc) * sizeof(**desc));
			memset(&(*desc)[i], 0, sizeof(**desc));
			(*desc)[i].fname = files->str[files->nstr];
		}
		/* add given action to file's descriptor */
		(*desc)[i].act = realloc((*desc)[i].act,
				++(*desc)[i].nact * sizeof(*(*desc)[i].act));
		(*desc)[i].act[(*desc)[i].nact - 1] = act;
	}
	free(files->str);
	return 0;
}

/** parse_config_str: parse a configuration string
 * @param cfgstr: string containing the config content
 * @param desc: pointer to f2b_desc array to create
 * @param ndesc: final length of array behind @desc
 * @param act: pointer to f2b_action array to create
 * @param nact: final length of array behind @act */
static int parse_config_str(char *cfgstr,
		struct f2b_desc **desc, int *ndesc)
{
	struct substr cfg_ss, files_ss;
	char **w, *endptr;
	struct f2b_desc *_desc = NULL;
	struct f2b_action *defact, *curact;
	int _ndesc = 0, rc;
	enum {
		ST_REG = 0,
		ST_FILE,
		ST_ARG_MATCH,
		ST_ARG_LIMIT,
		ST_ARG_TIMEOUT,
		ST_ARG_BAN,
		ST_ARG_UNBAN,
	} state = ST_REG;

	substr_init(&files_ss);

	/* allocate for default action (the first one) */
	defact = get_new_action();

	/* fill in sane defaults */
	curact = defact;
	curact->limit = 3;
	curact->timeout = 60;

	substr_wordsplit(&cfg_ss, cfgstr);
#define W_IS(str) (!strcmp(*w, str))
	for (w = cfg_ss.str; *w; w++) {
		//printf("%s: handling word '%s'\n", __func__, *w);
		switch (state) {
		case ST_ARG_MATCH:
			if (!curact->re) {
				curact->re = malloc(sizeof(*curact->re));
				memset(curact->re, 0, sizeof(*curact->re));
			} else {
				regfree(curact->re);
			}
			regcomp(curact->re, *w, 0); /* XXX: allow passing flags */
			state = ST_REG;
			break;
		case ST_ARG_LIMIT:
			curact->limit = strtol(*w, &endptr, 0);
			if (*endptr) {
				err("parsing limit arg '%s' failed", *w);
				goto out_syntax_error;
			}
			state = ST_REG;
			break;
		case ST_ARG_TIMEOUT:
			curact->timeout = strtol(*w, &endptr, 0);
			if (*endptr) {
				err("parsing timeout arg '%s' failed", *w);
				goto out_syntax_error;
			}
			state = ST_REG;
			break;
		case ST_ARG_BAN:
			substr_wordsplit(&curact->ban, *w);
			state = ST_REG;
			break;
		case ST_ARG_UNBAN:
			substr_wordsplit(&curact->unban, *w);
			state = ST_REG;
			break;
		case ST_FILE:
			if (W_IS("match") ||
			    W_IS("limit") ||
			    W_IS("timeout") ||
			    W_IS("ban") ||
			    W_IS("unban") ||
			    W_IS("}")) {
				err("%s: unexpected word '%s'", __func__, *w);
				goto out_syntax_error;
			}
			/* fall through */
		case ST_REG:
			if (W_IS("match")) {
				state = ST_ARG_MATCH;
			} else if (W_IS("limit")) {
				state = ST_ARG_LIMIT;
			} else if (W_IS("timeout")) {
				state = ST_ARG_TIMEOUT;
			} else if (W_IS("ban")) {
				state = ST_ARG_BAN;
			} else if (W_IS("unban")) {
				state = ST_ARG_UNBAN;
			} else if (W_IS("{")) {
				curact = get_new_action();
				state = ST_REG;
			} else if (W_IS("}")) {
				if (curact == defact) {
					err("%s: closing brace without an opening one?", __func__);
					goto out_syntax_error;
				}
				if (!files_ss.nstr) {
					err("%s: confined action missing files?!", __func__);
					goto out_syntax_error;
				}
				/* fill empty fields with default values */
				action_copy_empty(curact, defact);
				add_desc(&_desc, &_ndesc, curact, &files_ss);
				substr_init(&files_ss);
				curact = defact;
				state = ST_REG;
			} else { /* assume word is a file name */
				substr_append(&files_ss, *w);
				state = ST_FILE;
			}
			break;
		}
#undef W_IS
	}
	switch (state) {
	case ST_FILE:
		if (!files_ss.nstr) {
			err("%s: left in state ST_FILE but no files given?", __func__);
			goto out_syntax_error;
		}
		add_desc(&_desc, &_ndesc, curact, &files_ss);
		substr_init(&files_ss);
		break;
	case ST_REG:
		/* nothing to do */
		break;
	default:
		err("%s: left in state %d at EOF", __func__, state);
		goto out_syntax_error;
	}

	/* assign output variables */
	if (desc)
		*desc = _desc;
	if (ndesc)
		*ndesc = _ndesc;
	rc = 0;
	goto out_cleanup;
out_syntax_error:
	rc = 1;
	if (_ndesc)
		free(_desc);
out_cleanup:
	substr_free(&cfg_ss);
	substr_free(&files_ss);
	return rc;
}

/* parse_config_fp: wrapper around parse_config_str, reading
 *                  data from given file pointer
 * @param fp: file pointer to read config content from */
static int parse_config_fp(FILE *fp,
		struct f2b_desc **desc, int *ndesc)
{
	char *cfgstr;
	int len, rc, pos = 0, lstart = 0;

	len = 1024;
	if (!(cfgstr = calloc(len, sizeof(char)))) {
		err("%s: out of core", __func__);
		return 1;
	}

	while (fgets(cfgstr + pos, len - pos, fp)) {
		pos += strlen(cfgstr + pos);
		if (cfgstr[pos - 1] == '\n') {
			while (cfgstr[lstart] != '#' &&
			       cfgstr[lstart] != '\n')
				lstart++;
			cfgstr[lstart++] = ' ';
			pos = lstart;
			cfgstr[pos] = '\0';
		}
		if (pos < len - 2)
			continue;
		/* buffer space exhausted */
		len += 1024;
		cfgstr = safe_realloc(cfgstr, len * sizeof(char));
		if (!cfgstr) {
			err("%s: out of core", __func__);
			return 1;
		}
	}
	dbg("%s: read config into string: '%s'", __func__, cfgstr);
	rc = parse_config_str(cfgstr, desc, ndesc);
	free(cfgstr);
	return rc;
}

/** parse_config_file: wrapper around parse_config_fp,
 *                     opening given file name
 * @param cfgpath: filename to open for passing on */
int parse_config_file(const char *cfgpath,
		struct f2b_desc **desc, int *ndesc)
{
	int rc;
	FILE *fp;

	if (!(fp = fopen(cfgpath, "r"))) {
		err("opening config file '%s' failed: %s",
				cfgpath, strerror(errno));
		return 1;
	}
	rc = parse_config_fp(fp, desc, ndesc);
	fclose(fp);
	return rc;
}
