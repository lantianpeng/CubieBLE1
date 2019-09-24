/** @file
 *  @brief LLS Service sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LLS_H
#define _LLS_H

#ifdef __cplusplus
extern "C" {
#endif

void lls_init(void);
void lls_alert_off(void);
void lls_alert_on(void);

#ifdef __cplusplus
}
#endif

#endif    /* _LLS_H */