/* helper.c - helper routines
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#include <stdio.h>
#include <stdlib.h>

/** safe_realloc: wrapper around realloc, free buffer on error
 * @param buf: buffer to reallocate
 * @param buflen: new length of @buf
 * @return: location of reallocated @buf or NULL on error */
void *safe_realloc(void *buf, size_t buflen)
{
	void *tmp = realloc(buf, buflen);
	if (buflen && !tmp) {
		free(buf);
		return NULL;
	}
	return tmp;
}
