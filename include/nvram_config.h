/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NVRAM config driver interface
 */

#ifndef __INCLUDE_NVRAM_CONFIG_H__
#define __INCLUDE_NVRAM_CONFIG_H__

#include <kernel.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVRAM_WRITE_REGION_ALIGN_SIZE	256
#define NVRAM_BUFFER_SIZE		NVRAM_WRITE_REGION_ALIGN_SIZE

/**
 * @brief driver data
 */
struct acts_nvram_config {
	const struct nvram_region_config *factory_config;
	const struct nvram_region_config *user_config;
	u8_t *nvram_buf;
};

struct nvram_region_config {
	char name[16];
	u32_t flag;

	/* write regions base address */
	u32_t base_addr;
	/* total write regions size, aligned to erase size */
	s32_t total_size;
};

int nvram_config_init(struct device *dev);

int nvram_config_get(const char *name, void *data, int max_len);
int nvram_config_set(const char *name, const void *data, int len);
int nvram_config_clear_all(void);
void nvram_config_dump(void);

int nvram_config_get_factory(const char *name, void *data, int max_len);
int nvram_config_set_factory(const char *name, const void *data, int len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_NVRAM_CONFIG_H__ */
