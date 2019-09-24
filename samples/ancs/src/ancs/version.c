/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>

extern int snprintf_rom(char *, size_t, const char *, ...);

#define YEAR ((__DATE__[9] - '0') * 10 + (__DATE__[10] - '0'))

#define MONTH (__DATE__[2] == 'n' ? 0 \
		: __DATE__[2] == 'b' ? 1 \
		: __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 2 : 3) \
		: __DATE__[2] == 'y' ? 4 \
		: __DATE__[2] == 'n' ? 5 \
		: __DATE__[2] == 'l' ? 6 \
		: __DATE__[2] == 'g' ? 7 \
		: __DATE__[2] == 'p' ? 8 \
		: __DATE__[2] == 't' ? 9 \
		: __DATE__[2] == 'v' ? 10 : 11)

#define DAY ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 \
		+ (__DATE__[5] - '0'))

#define HOUR  ((__TIME__[0] - '0') * 10 + (__TIME__[1] - '0'))

char sys_version[] = "2.0.00.18101609";

char *sys_version_get(void)
{
	snprintf_rom(sys_version, sizeof(sys_version), "2.0.00.%02d%02d%02d%02d", YEAR, MONTH + 1, DAY, HOUR);
	return sys_version;
}
