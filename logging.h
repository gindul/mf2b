/* logging.h - mf2b logging declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _LOGGING_H
#define _LOGGING_H

enum log_level {
	LL_NONE = 1,
	LL_CRIT = 2,	/* match syslog priorities from here on */
	LL_ERR,
	LL_WARN,
	LL_NOTICE,
	LL_INFO,
	LL_DEBUG,
};

void add_log_level(int);
int set_log_output(const char *, const char *);

void do_log(enum log_level, const char *, ...);

#define crit(...) do_log(LL_CRIT, __VA_ARGS__)
#define err(...) do_log(LL_ERR, __VA_ARGS__)
#define warn(...) do_log(LL_WARN, __VA_ARGS__)
#define info(...) do_log(LL_INFO, __VA_ARGS__)
#define note(...) do_log(LL_NOTICE, __VA_ARGS__)
#define dbg(...) do_log(LL_DEBUG, __VA_ARGS__)

#endif /* _LOGGING_H */
