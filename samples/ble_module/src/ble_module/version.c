/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

char sys_version[] = "2.0.00.18070920";

char *sys_version_get(void)
{
	return sys_version;
}
