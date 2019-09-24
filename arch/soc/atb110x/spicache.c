/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPICACHE profile interface for Actions GL5601 platform
 */

#if defined(CONFIG_SPICACHE_PROFILE)

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <misc/printk.h>
#include "errno.h"
#include "spicache.h"

#define SPICACHE_CTL				0x40014000
#define SPICACHE_PROFILE_ADDR_START		(SPICACHE_CTL + 0x20)
#define SPICACHE_PROFILE_ADDR_END		(SPICACHE_CTL + 0x24)
#define SPICACHE_RANGE_ADDR_MISS_COUNT		(SPICACHE_CTL + 0x28)
#define SPICACHE_RANGE_ADDR_HIT_COUNT		(SPICACHE_CTL + 0x2c)
#define SPICACHE_TOTAL_MISS_COUNT		(SPICACHE_CTL + 0x30)
#define SPICACHE_TOTAL_HIT_COUNT		(SPICACHE_CTL + 0x34)

void spicache_profile_dump(struct spicache_profile *profile)
{
	u32_t interval_ms, total;

	if (!profile)
		return;

	interval_ms = SYS_CLOCK_HW_CYCLES_TO_NS64((int32_t)(profile->end_time) -
								(int32_t)(profile->start_time)) / 1000000;

	printk("spicache profile: addr range 0x%08x ~ 0x%08x, profile time %d ms\n",
		profile->start_addr, profile->end_addr, interval_ms);

	total = (profile->hit_cnt + profile->miss_cnt);
	if (total != 0)
		printk("       hit: %12d           miss: %12d    hit ratio: %d %%\n",
			profile->hit_cnt, profile->miss_cnt,
			(profile->hit_cnt * 100) / total);

	else
		printk("cpu not run into the specific address range!\n");

	total = (profile->total_hit_cnt + profile->total_miss_cnt);
	if (total != 0)
		printk("totoal hit: %12d    totoal miss: %12d    hit ratio: %d %%\n\n",
			profile->total_hit_cnt, profile->total_miss_cnt,
			(profile->total_hit_cnt * 100) / total);
}

static void spicache_update_profile_data(struct spicache_profile *profile)
{
	profile->hit_cnt = sys_read32(SPICACHE_RANGE_ADDR_HIT_COUNT);
	profile->miss_cnt = sys_read32(SPICACHE_RANGE_ADDR_MISS_COUNT);
	profile->total_hit_cnt = sys_read32(SPICACHE_TOTAL_HIT_COUNT);
	profile->total_miss_cnt = sys_read32(SPICACHE_TOTAL_MISS_COUNT);
}

int spicache_profile_get_data(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	spicache_update_profile_data(profile);

	return 0;
}

int spicache_profile_stop(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	profile->end_time = k_cycle_get_32();
	spicache_update_profile_data(profile);

	sys_write32(sys_read32(SPICACHE_CTL) & ~(1 << 4), SPICACHE_CTL);

	return 0;
}

int spicache_profile_start(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	sys_write32(profile->start_addr, SPICACHE_PROFILE_ADDR_START);
	sys_write32(profile->end_addr, SPICACHE_PROFILE_ADDR_END);

	profile->start_time = k_cycle_get_32();

	sys_write32(sys_read32(SPICACHE_CTL) | (1 << 4), SPICACHE_CTL);

	return 0;
}
#endif
