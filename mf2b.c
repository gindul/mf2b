/* mf2b.c - mf2b core routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 *
 * Content of function check_fd() copied from coreutils/tail.c
 * of busybox and therefore Copyright (C) 2009  Eric Lammerts
 * (at least according to busybox' git log).
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "action.h"
#include "config.h"
#include "helper.h"
#include "logging.h"
#include "matchlist.h"
#include "substr.h"
#include "mf2b.h"

#define DEFAULT_CONFIG	"/etc/mf2b.conf"

void matchlist_maintenance(struct f2b_match *match, time_t timeout)
{
	time_t now = time(NULL);

	while (!matchlist_empty(match->mlist) &&
	       matchlist_get(match->mlist, 0) + timeout < now) {
		dbg("first time is %d", matchlist_get(match->mlist, 0));
		matchlist_drop_first(&match->mlist);
	}
}

static int exec_replace(struct substr *argv, struct substr *ss)
{
	struct substr argv_rep;
	char *cmd;
	int status;
	pid_t pid;

	if (substr_copy_replace(&argv_rep, argv, ss)) {
		err("substr_copy_replace() failed!\n");
		return 1;
	}

	cmd = substr_printable(&argv_rep);
	info("calling: %s", cmd);
	free(cmd);

	if ((pid = fork()) == -1) {
		err("fork() failed: %s", strerror(errno));
		return 1;
	}
	if (pid != 0) { /* parent process */
		while (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR) {
				err("waitpid() failed: %s", strerror(errno));
				return 1;
			}
		}
		if (!WIFEXITED(status)) {
			err("child did not exit normally?!");
			return 1;
		} else if (WEXITSTATUS(status)) {
			err("child exited non-zero (%d)", WEXITSTATUS(status));
			return 1;
		}
	} else { /* child process */
		if (execv(argv_rep.str[0], argv_rep.str) == -1) {
			err("execv() failed: %s", strerror(errno));
			exit(127);
		}
	}
	substr_free(&argv_rep);
	return 0;
}

static int check_unban(struct f2b_action *act)
{
	int i, j;
	struct f2b_match *match;

	for (match = &act->match[i = 0]; i < act->nmatch; match = &act->match[++i]) {
		matchlist_maintenance(match, act->timeout);
		if (matchlist_full(match->mlist))
			continue;
		if (match->banned)
			match->banned = exec_replace(&act->unban, &match->subs);
		if (matchlist_empty(match->mlist)) {
			dbg("droppping empty matchlist");
			substr_free(&match->subs);
			matchlist_clear(&match->mlist);
			/* XXX: fix this? */
#if 0
			if (act->nmatch > (i + 1))
				memmove(match, match + 1, (act->nmatch - i) * sizeof(*match));
#else
			for (j = i + 1; j < act->nmatch; j++)
				act->match[j - 1] = act->match[j];
#endif
			act->nmatch--;
			act->match = realloc(act->match, sizeof(*match) * act->nmatch);
			i--;
		}
	}
	return 0;
}

int match_line(struct f2b_desc *desc, char *str)
{
	int i, j;
	struct f2b_action *act;
	regmatch_t match[MAX_SUBS + 1];	/* allocate for one more, first is always full line */

	dbg("matching line: %s", str);
	for (i = 0; i < desc->nact; i++) {
		act = desc->act[i];
		if (regexec(act->re, str, MAX_SUBS + 1, match, 0))
			continue;
		dbg("%s: got a match for action %d", desc->fname, i);
		for (j = 0; j < act->nmatch; j++) {
			if (substr_match(&act->match[j].subs, str, match))
				break;
		}
		dbg("%s: subs matched, got index %d, nmatch is %d", desc->fname, j, act->nmatch);
		if (j == act->nmatch) {
			dbg("%s: extending match array to length %d", desc->fname, j + 1);
			act->nmatch = j + 1;
			act->match = realloc(act->match, sizeof(*act->match) * act->nmatch);
			substr_extract(&act->match[j].subs, str, match);
			act->match[j].banned = 0;
			matchlist_init(&act->match[j].mlist, act->limit);
		}
		dbg("adding matchlist entry with time %d", (int)time(NULL));
		matchlist_add(&act->match[j].mlist, time(NULL));
		dbg("matchlist has now %d entries", matchlist_size(act->match[j].mlist));
		if (matchlist_full(act->match[j].mlist) && !act->match[j].banned) {
			act->match[j].banned = !exec_replace(&act->ban,
			                                     &act->match[j].subs);
		}
		break;
	}
	return 0;
}

int read_lines(struct f2b_desc *desc, int fd)
{
	static char *buf = NULL;
	static int buflen = 0;
	int bufpos = 0, rlen;
	char *p, *pstart;

	dbg("reading lines from file %s with fd %d at offset %d",
			desc->fname, fd, (int)lseek(fd, 0, SEEK_CUR));
	if (!buflen) {
		buflen = 1024;
		buf = calloc(buflen, sizeof(char));
	}
	if (desc->backlog) {
		bufpos = strlen(desc->backlog);
		if (buflen <= bufpos) {
			while (buflen <= bufpos)
				buflen *= 2;
			buf = safe_realloc(buf, buflen);
			if (!buf) {
				err("%s: out of core", __func__);
				return 1;
			}
		}
		sprintf(buf, "%s", desc->backlog);
		free(desc->backlog);
		desc->backlog = NULL;
	}
	while ((rlen = read(fd, buf + bufpos, buflen - 1 - bufpos)) > 0) {
		buf[bufpos + rlen] = '\0';
		if ((p = strchr(buf, '\n')) == NULL) {
			/* buffer length exceeded */
			buflen *= 2;
			buf = safe_realloc(buf, buflen);
			if (!buf) {
				err("%s: out of core", __func__);
				return 1;
			}
			bufpos += rlen;
			continue;
		}
		pstart = buf;
		do {
			*p = '\0';
			match_line(desc, pstart);
			pstart = p + 1;
		} while ((p = strchr(pstart, '\n')) != NULL);
		memmove(buf, pstart, strlen(pstart) + 1);
		bufpos = strlen(buf);
	}

	if (bufpos)
		asprintf(&desc->backlog, "%s", buf);
	return 0;
}


/* Make sure the fd is valid and the file hasn't been replaced. */
int check_fd(struct f2b_desc *desc, int fd)
{
	struct stat sbuf, fsbuf;

	if (fd < 0
	    || fstat(fd, &fsbuf) < 0
	    || stat(desc->fname, &sbuf) < 0
	    || fsbuf.st_dev != sbuf.st_dev
	    || fsbuf.st_ino != sbuf.st_ino) {
		if (fd >= 0)
			close(fd);
		fd = open(desc->fname, O_RDONLY);
		if (fd >= 0)
			lseek(fd, 0, SEEK_END);
	}
	return fd;
}

#define INOTIFY_EVENTS \
	(IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE_SELF | IN_MOVE)

int main(int argc, char **argv)
{
	struct f2b_desc *desc;
	struct f2b_action *act;
	int opt, foreground = 0, log_output_set = 0;
	int i, ndesc, checktime = 0, check_interval = 0;
	struct pollfd *pfd;
	struct inotify_event inev;
	char *config_file = NULL, *chroot_path = NULL, *pidfile = NULL;

	set_log_output(argv[0], "stderr");

	while ((opt = getopt(argc, argv, "c:fl:p:qr:Vv")) != -1) {
		switch (opt) {
		case 'c':
			if (config_file)
				free(config_file);
			config_file = strdup(optarg);
			break;
		case 'f':
			foreground = 1;
			break;
		case 'l':
			if (set_log_output(argv[0], optarg)) {
				crit("opening log output '%s' failed", optarg);
				return 1;
			}
			log_output_set = 1;
			break;
		case 'p':
			if (pidfile)
				free(pidfile);
			pidfile = strdup(optarg);
		case 'q':
			add_log_level(-1);
			break;
		case 'r':
			if (chroot_path)
				free(chroot_path);
			chroot_path = strdup(optarg);
			break;
		case 'V':
			crit("%s version " VERSION " Copyright (C) 2014 by Phil Sutter", argv[0]);
			return 0;
		case 'v':
			add_log_level(1);
			break;

		default:
			crit("Usage: %s [-fqVv] [-c config] [-l logfile] [-r chroot-path]\n", argv[0]);
			crit("config defaults to '" DEFAULT_CONFIG "'.");
			crit("logfile may also be stdout, stderr or syslog.");
			crit("Default is syslog or stderr if running in foreground.");
			return 1;
		}
	}
	if (!config_file)
		config_file = strdup(DEFAULT_CONFIG);

	note("using config file %s", config_file);
	if (parse_config_file(config_file, &desc, &ndesc)) {
		crit("config parser failed!");
		return 1;
	}
	free(config_file);

	if (ndesc < 1) {
		crit("no files specified in config?");
		return 1;
	}
	dbg("parsed config, %d descriptors found", ndesc);

	if (chroot_path) {
		if (chroot(chroot_path))
			err("chroot failed: %s", strerror(errno));
	}

	if (!foreground) {
		if (daemon(1, 0))
			err("daemonize failed: %s", strerror(errno));
		if (!log_output_set)
			set_log_output(argv[0], "syslog");
	}

	if (pidfile) {
		FILE *pid_fp = fopen(pidfile, "w");
		if (pid_fp) {
			fprintf(pid_fp, "%d", getpid());
			fclose(pid_fp);
		} else {
			err("opening pidfile '%s' failed: %s",
					pidfile, strerror(errno));
		}
	}

	/* use the smallest timeout as interval for checking */
	check_interval = get_min_action_timeout();
	note("checking matchlists every %d seconds", check_interval);

	pfd = calloc(ndesc, sizeof(*pfd));
	for (i = 0; i < ndesc; i++) {
		desc[i].fd = check_fd(&desc[i], -1);
		pfd[i].events = POLLIN;
		pfd[i].fd = inotify_init();
		inotify_add_watch(pfd[i].fd, desc[i].fname, INOTIFY_EVENTS);
	}
	while (poll(pfd, ndesc, check_interval * 1000) >= 0) {
		for (i = 0; i < ndesc; i++) {
			if (pfd[i].revents) {
				read(pfd[i].fd, &inev, sizeof(inev));
				if (inev.mask & IN_MODIFY)
					read_lines(&desc[i], desc[i].fd);
				if (inev.mask & ~IN_MODIFY)
					desc[i].fd = check_fd(&desc[i], desc[i].fd);
			}
			if (desc[i].fd == -1) {
				desc[i].fd = check_fd(&desc[i], desc[i].fd);
				inotify_add_watch(pfd[i].fd,
						desc[i].fname, INOTIFY_EVENTS);
			}
		}
		/* do maintenance every check_interval seconds */
		if (checktime + check_interval > time(NULL))
			continue;
		checktime = time(NULL);
		/* maintenance tasks */
		action_iterate(1);
		while ((act = action_iterate(0)))
			check_unban(act);
	}
	return 0;
}
