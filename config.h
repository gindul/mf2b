/* config.h - mf2b config parsing declarations
 *
 * Copyright (C) 2014  Phil Sutter <phil@nwl.cc>
 *
 * This file is part of Micro Fail2Ban, licensed under
 * GPLv3 or later. See file LICENSE in this source tree.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#include "mf2b.h"	/* struct f2b_desc */

int parse_config_file(const char *, struct f2b_desc **, int *);

#endif /* _CONFIG_H */
