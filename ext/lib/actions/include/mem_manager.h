/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MEM_MANAGER_H__
#define __MEM_MANAGER_H__

#include <kernel.h>

void *mem_malloc(unsigned int num_bytes);

void mem_free(void *ptr);

#endif
