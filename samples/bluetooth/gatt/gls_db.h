/** @file
 *  @brief GLS Database sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GLS_DB_H
#define _GLS_DB_H
#include "gls.h"

#define GLS_DB_MAX_RECORDS      20

void gls_db_init(void);
u16_t gls_db_num_records_get(void);
int gls_db_record_get(u8_t record_num, struct gls_record *record);
int gls_db_record_add(struct gls_record *record);
int gls_db_record_delete(u8_t record_num);
void gls_db_record_delete_all(void);
int gls_db_record_delete_first(void);
int gls_db_record_delete_last(void);

#endif    /* _GLS_DB_H */

