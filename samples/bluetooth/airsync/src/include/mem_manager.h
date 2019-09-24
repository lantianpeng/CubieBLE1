/** @file
 *  @brief Wecaht Service sample
 */

/*
 * Copyright (c) 2017-2018 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * Change log:
 *	2017/5/5: Created by lipeng.
 */
#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

void mem_init(void);
void *mem_malloc(size_t size);
void mem_free(void *buf);

#ifdef __cplusplus
}
#endif

#endif	/* _MEM_MANAGER_H_ */
