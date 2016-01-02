/* logging.c - log output unit test
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <assert.h>
#include <stdio.h>

#define UNIT_TESTING
#include "../../logging.c"

int main(void)
{
	FILE *fp;
	char buf[64] = {0};

	assert(level == LL_ERR);

	add_log_level(4);
	assert(level == LL_DEBUG);
	add_log_level(1);
	assert(level == LL_DEBUG);
	add_log_level(-5);
	assert(level == LL_CRIT);
	add_log_level(-1);
	assert(level == LL_NONE);
	add_log_level(1);

	assert(!set_log_output("unit_test", "unit_test.log"));
	do_log(LL_CRIT, "log message");
	set_log_output("unit_test", NULL);
	fp = fopen("unit_test.log", "r");
	assert(fp);
	fgets(buf, 64, fp);
	fclose(fp);
	assert(!strcmp(buf, "log message\n"));
	unlink("unit_test.log");
	return 0;
}
