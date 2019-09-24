/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPICACHE profile interface for Actions GL5118 platform
 */

#ifndef __SPICACHE_H__
#define __SPICACHE_H__

struct spicache_profile {
	/* must aligned to 64-byte */
	u32_t start_addr;
	u32_t end_addr;

	/* timestamp, ns */
	u32_t start_time;
	u32_t end_time;

	/* hit/miss in user address range */
	u32_t hit_cnt;
	u32_t miss_cnt;

	/* hit/miss in all address range */
	u32_t total_hit_cnt;
	u32_t total_miss_cnt;
};

int spicache_profile_start(struct spicache_profile *profile);
int spicache_profile_stop(struct spicache_profile *profile);
int spicache_profile_get_data(struct spicache_profile *profile);
void spicache_profile_dump(struct spicache_profile *profile);

#endif	/* __SPICACHE_H__ */
