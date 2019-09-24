/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-volatile memory driver
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <nvram_config.h>
#include "nvram_storage.h"
#include <flash.h>

#define SYS_LOG_DOMAIN "nvram"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/* factory config region */
const struct nvram_region_config factory_nvram_region_config = {
	.name = "Factory Config",
	.base_addr = CONFIG_NVRAM_FACTORY_REGION_BASE_ADDR,
	.total_size = CONFIG_NVRAM_FACTORY_REGION_SIZE,
	.flag = 0,
};

/* user config region */
const struct nvram_region_config user_nvram_region_config = {
	.name = "User Config",
	.base_addr = CONFIG_NVRAM_WRITE_REGION_BASE_ADDR,
	.total_size = CONFIG_NVRAM_WRITE_REGION_SIZE,
	.flag = 0,
};

u32_t nvram_buffer[NVRAM_BUFFER_SIZE/4];

const struct acts_nvram_config acts_nvram_config_info0 = {
	.factory_config = &factory_nvram_region_config,
	.user_config = &user_nvram_region_config,
	.nvram_buf = (u8_t *)nvram_buffer,
};

extern int nvram_config_init(struct device *dev);

int acts_nvram_config_init(struct device *dev)
{
	int ret;

	ret = nvram_config_init(dev);

#ifdef CONFIG_XSPI_NOR_LOCK
	if (ret == 0) {
		struct device *storage_dev;

		storage_dev = device_get_binding(CONFIG_NVRAM_STORAGE_DEV_NAME);
		if (!storage_dev) {
			SYS_LOG_ERR("cannot found device \'%s\'",
						CONFIG_NVRAM_STORAGE_DEV_NAME);
		} else {
		    /* Software Protection */
			flash_unlock(storage_dev, CONFIG_NVRAM_FACTORY_REGION_BASE_ADDR, CONFIG_NVRAM_FACTORY_REGION_SIZE + CONFIG_NVRAM_WRITE_REGION_SIZE);
		}
	}
#endif
	return ret;
}

DEVICE_INIT(nvram_acts, CONFIG_NVRAM_ACTS_DRV_NAME,
			acts_nvram_config_init,
			NULL, &acts_nvram_config_info0,
			PRE_KERNEL_1, CONFIG_NVRAM_CONFIG_INIT_PRIORITY);
