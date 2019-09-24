/** @file
 *  @brief CGMS Database sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CGMS_DB_H
#define _CGMS_DB_H
#include "cgms.h"

#define CGMS_DB_MAX_RECORDS      20

void cgms_db_init(void);
u16_t cgms_db_num_records_get(void);
int cgms_db_record_get(u8_t record_num, struct cgms_record *record);
int cgms_db_record_add(struct cgms_record *record);
int cgms_db_record_delete(u8_t record_num);
void cgms_db_record_delete_all(void);
int cgms_db_record_delete_first(void);
int cgms_db_record_delete_last(void);

#endif    /* _CGMS_DB_H */

