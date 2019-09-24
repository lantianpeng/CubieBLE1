/** @file
 *  @brief CGMS Database sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cgms_db.h"
#include "errno.h"

struct database_entry {
    bool  is_in_use;
    struct cgms_record record;
};

static struct database_entry cgms_database[CGMS_DB_MAX_RECORDS];
static u16_t  cgms_records_num;

void cgms_db_init(void)
{
    int i;

    for (i = 0; i < CGMS_DB_MAX_RECORDS; i++) {
        cgms_database[i].is_in_use = false;
    }

    cgms_records_num = 0;
}

u16_t cgms_db_num_records_get(void)
{	
	return cgms_records_num;
}

int cgms_db_record_get(u8_t rec_idx, struct cgms_record *record)
{
    if (rec_idx >= cgms_records_num) {
        return -EINVAL;
    }

    /* copy record to the specified memory */
    *record = cgms_database[rec_idx].record;

    return 0;
}

int cgms_db_record_add(struct cgms_record *record)
{
    int i;

    if (cgms_records_num == CGMS_DB_MAX_RECORDS) {
        return -ENOMEM;
    }

    /* find next available database entry */ 
    for (i = 0; i < CGMS_DB_MAX_RECORDS; i++) {
        if (!cgms_database[i].is_in_use) {
            cgms_database[i].is_in_use = true;
            cgms_database[i].record  = *record;

            cgms_records_num++;
            return 0;
        }
    }

    return -ENOMEM;
}

int cgms_db_record_delete(u8_t rec_idx)
{
    if (rec_idx >= cgms_records_num) {
        return -EINVAL;
    }

    /* free entry */ 
    cgms_database[rec_idx].is_in_use = false;

    /* decrease number of records */ 
    cgms_records_num--;

    return 0;
}

int cgms_db_record_delete_first(void)
{
	int i;
	for (i = 0; i < CGMS_DB_MAX_RECORDS; i++) {
		if (cgms_database[i].is_in_use) {
			cgms_database[i].is_in_use = false;
			cgms_records_num--;
			return 0;
		} 
	}
    return -1;
}

int cgms_db_record_delete_last(void)
{
	int i;
	for (i = CGMS_DB_MAX_RECORDS -1; i >= 0; i++) {
		if (cgms_database[i].is_in_use) {
			cgms_database[i].is_in_use = false;
			cgms_records_num--;
			return 0;
		} 
	}
    return -1;
}
void cgms_db_record_delete_all(void)
{
	int i;
	for (i = 0; i < CGMS_DB_MAX_RECORDS; i++) {
		cgms_database[i].is_in_use = false;
	}

	cgms_records_num = 0;
}

