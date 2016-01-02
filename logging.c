/* logging.c - mf2b logging routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "logging.h"

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static enum log_level level = LL_ERR;

static FILE *logfile_fp = NULL;
static int do_syslog = 0;

void add_log_level(int offset)
{
	level += offset;
	level = MIN(level, LL_DEBUG);
	level = MAX(level, LL_NONE);
}

int set_log_output(const char *ident, const char *output)
{
	if (logfile_fp) {
		if (logfile_fp != stdout && logfile_fp != stderr)
			fclose(logfile_fp);
		logfile_fp = NULL;
	}
	if (!output)
		return 1;
	if (!strcmp(output, "stdout")) {
		logfile_fp = stdout;
	} else if (!strcmp(output, "stderr")) {
		logfile_fp = stderr;
	} else if (!strcmp(output, "syslog")) {
		do_syslog = 1;
		openlog(ident, 0, LOG_DAEMON);
	} else {
		if ((logfile_fp = fopen(output, "a")) == NULL)
			return 1;
	}
	return 0;
}

void do_log(enum log_level lvl, const char *fmt, ...)
{
	va_list ap;

	if (lvl > level)
		return;
	va_start(ap, fmt);
	if (logfile_fp) {
		vfprintf(logfile_fp, fmt, ap);
		fprintf(logfile_fp, "\n");
	}
	if (do_syslog)
		vsyslog(lvl, fmt, ap);
	va_end(ap);
}
