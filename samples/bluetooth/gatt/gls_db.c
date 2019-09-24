/** @file
 *  @brief GLS Database sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gls_db.h"
#include "errno.h"

struct database_entry {
    bool  is_in_use;
    struct gls_record record;
};

static struct database_entry gls_database[GLS_DB_MAX_RECORDS];
static u16_t  gls_records_num;

void gls_db_init(void)
{
    int i;

    for (i = 0; i < GLS_DB_MAX_RECORDS; i++) {
        gls_database[i].is_in_use = false;
    }

    gls_records_num = 0;
}

u16_t gls_db_num_records_get(void)
{	
	return gls_records_num;
}

int gls_db_record_get(u8_t rec_idx, struct gls_record *record)
{
    if (rec_idx >= gls_records_num) {
        return -EINVAL;
    }

    /* copy record to the specified memory */
    *record = gls_database[rec_idx].record;

    return 0;
}

int gls_db_record_add(struct gls_record *record)
{
    int i;

    if (gls_records_num == GLS_DB_MAX_RECORDS) {
        return -ENOMEM;
    }

    /* find next available database entry */ 
    for (i = 0; i < GLS_DB_MAX_RECORDS; i++) {
        if (!gls_database[i].is_in_use) {
            gls_database[i].is_in_use = true;
            gls_database[i].record  = *record;

            gls_records_num++;
            return 0;
        }
    }

    return -ENOMEM;
}

int gls_db_record_delete(u8_t rec_idx)
{
    if (rec_idx >= gls_records_num) {
        return -EINVAL;
    }

    /* free entry */ 
    gls_database[rec_idx].is_in_use = false;

    /* decrease number of records */ 
    gls_records_num--;

    return 0;
}

int gls_db_record_delete_first(void)
{
	int i;
	for (i = 0; i < GLS_DB_MAX_RECORDS; i++) {
		if (gls_database[i].is_in_use) {
			gls_database[i].is_in_use = false;
			gls_records_num--;
			return 0;
		} 
	}
    return -1;
}

int gls_db_record_delete_last(void)
{
	int i;
	for (i = GLS_DB_MAX_RECORDS -1; i >= 0; i++) {
		if (gls_database[i].is_in_use) {
			gls_database[i].is_in_use = false;
			gls_records_num--;
			return 0;
		} 
	}
    return -1;
}
void gls_db_record_delete_all(void)
{
	int i;
	for (i = 0; i < GLS_DB_MAX_RECORDS; i++) {
		gls_database[i].is_in_use = false;
	}

	gls_records_num = 0;
}


