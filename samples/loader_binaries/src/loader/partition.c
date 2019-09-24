/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "boot.h"

/* build for keil Load.
 * If probatch by uart, partition_table require built by tools
 */
__attribute__((section(".partition"))) const struct partition_table part_table_in_nor = {
	.magic = PARTITION_TABLE_MAGIC,
	.part_cnt = 5,
	.part_entry_size = sizeof(struct partition_entry),
	.parts[0] = {
		.name = "fw0_boot",
		.type = BOOT_TYPE,
		.offset = LOADER_A_NOR_ADDR,
	},
	.parts[1] = {
		.name = "fw1_boot",
		.type = BOOT_TYPE,
		.offset = LOADER_B_NOR_ADDR,
		.seq = -1,
	},
	.parts[2] = {
		.name = "fw0_app",
		.type = SYSTEM_TYPE,
		.offset = APP_A_NOR_ADDR,
	},
	.parts[4] = {
		.name = "dtm",
		.type = DTM_TYPE,
		.offset = DTM_NOR_ADDR,
	},

};
