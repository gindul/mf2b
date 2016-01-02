/* mf2b.h - mf2b core declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef __MF2B_H
#define __MF2B_H

#include "action.h"	/* struct f2b_action */

struct f2b_desc {
	char *fname;
	struct f2b_action **act;
	int nact;
	int fd;
	char *backlog;
};

#endif /* __MF2B_H */
