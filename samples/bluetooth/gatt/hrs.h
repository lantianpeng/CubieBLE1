/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _HRS_H
#define _HRS_H

#ifdef __cplusplus
extern "C" {
#endif

void hrs_init(u8_t blsc);
void hrs_notify(void);

#ifdef __cplusplus
}
#endif

#endif    /* _HRS_H */