/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <string.h>
#include <misc/printk.h>

#include <zephyr.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/storage.h>

#include "keys.h"
#include "rpa.h"
#include <logging/sys_log.h>

#include "nvram_config.h"

u8_t addr_match_bt_keys(const bt_addr_le_t *addr, struct bt_keys *keys)
{
	u8_t ret = 0;

	if (!bt_addr_le_is_rpa(addr)) {
		if (!bt_addr_le_cmp(&keys->addr, addr))
			ret = 1;
	} else {
		if (!bt_addr_cmp(&addr->a, &keys->irk.rpa))
			ret = 1;

		if (bt_rpa_irk_matches(keys->irk.val, &addr->a)) {
			bt_addr_copy(&keys->irk.rpa, &addr->a);
			ret = 1;
		}
	}
	return ret;
}

static ssize_t storage_read(const bt_addr_le_t *addr, u16_t key, void *data,
			    size_t length)
{
	ssize_t ret = 0;
#ifdef CONFIG_NVRAM_CONFIG
	char *name = NULL;

	switch (key) {
	case BT_STORAGE_ID_ADDR:
		name = "BT_ADDR";
	break;
	case BT_STORAGE_LOCAL_IRK:
		name = "LOCAL_IRK";
	break;
	case BT_STORAGE_BT_KEYS:
		name = "BT_KEYS";
	break;
	}

	if (name != NULL) {
		ret = nvram_config_get(name, data, length);
		if (ret >= 0) {
			SYS_LOG_DBG("successful to get cfg %s, ret %d\n", name, ret);
		} else {
			SYS_LOG_DBG("failed to get cfg %s, ret %d\n", name, ret);
			return ret;
		}
	}

	if (addr != NULL) {
		if (!addr_match_bt_keys(addr, (struct bt_keys *)data)) {
			ret = 0;
			SYS_LOG_DBG("##addr_match_bt_keys fail\n");
		} else {
			SYS_LOG_DBG("##addr_match_bt_keys ok\n");
		}
	}
#endif
	return ret;
}

static ssize_t storage_write(const bt_addr_le_t *addr, u16_t key,
			     const void *data, size_t length)
{
	ssize_t ret = 0;
#ifdef CONFIG_NVRAM_CONFIG
	char *name = NULL;

	switch (key) {
	case BT_STORAGE_ID_ADDR:
		name = "BT_ADDR";
	break;
	case BT_STORAGE_LOCAL_IRK:
		name = "LOCAL_IRK";
	case BT_STORAGE_BT_KEYS:
	{
		name = "BT_KEYS";
		struct bt_keys tmp_bt_keys;

		ret = nvram_config_get(name, &tmp_bt_keys, sizeof(tmp_bt_keys));
		if ((ret >= 0) && (!memcmp(&tmp_bt_keys, data, sizeof(tmp_bt_keys)))) {
			SYS_LOG_DBG("same key\n");
			return ret;
		}
	}
	break;
	}

	if (name != NULL)
		ret = nvram_config_set(name, data, length);
#endif
	return ret;
}

static int storage_clear(const bt_addr_le_t *addr)
{
	int err = 0;
#ifdef CONFIG_NVRAM_CONFIG
	char *name = "BT_KEYS";
	struct bt_keys tmp_bt_keys;

	memset(&tmp_bt_keys, 0, sizeof(tmp_bt_keys));
	err = nvram_config_set(name, &tmp_bt_keys, sizeof(tmp_bt_keys));
#endif
	return err;
}

static int storage_init(struct device *unused)
{
	static const struct bt_storage storage = {
		.read  = storage_read,
		.write = storage_write,
		.clear = storage_clear
	};

#ifdef CONFIG_NVRAM_CONFIG
	bt_storage_register(&storage);
#endif
	return 0;
}

SYS_INIT(storage_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
