/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/util.h>

#define ACT_DATA_MTU 20

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define SYS_LOG_DOMAIN "act_data"
#include <logging/sys_log.h>

void act_data_init(void);

void enter_into_data_mode(void);

int act_data_send_data(const u8_t *data, int len);
