/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "errno.h"
#include <string.h>
#include <misc/printk.h>

#include <zephyr.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/storage.h>

#include "keys.h"
#include "rpa.h"
#include "nvram_config.h"

#define SYS_LOG_DOMAIN "bt_host"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#include <logging/sys_log.h>

#if (CONFIG_NVRAM_CONFIG) && (CONFIG_BT_MAX_CONN == 1)
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
			SYS_LOG_DBG("successful to get cfg %s, ret %d", name, ret);
		} else {
			SYS_LOG_DBG("failed to get cfg %s, ret %d", name, ret);
			return ret;
		}
	}

	if (addr != NULL) {
		if (!addr_match_bt_keys(addr, (struct bt_keys *)data)) {
			ret = 0;
			SYS_LOG_DBG("addr_match_bt_keys fail");
		} else {
			SYS_LOG_DBG("addr_match_bt_keys ok");
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
			SYS_LOG_DBG("same key");
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
#else
#define key_pool_count 2
static struct bt_keys storage_key_pool[key_pool_count];
static bt_addr_le_t	  storage_local_bt_addr;
static u8_t			  storage_local_irk[16];

enum storage_access {
	STORAGE_READ,
	STORAGE_WRITE
};
static int removeCnt;
struct bt_keys *find_bt_keys(const bt_addr_le_t *addr, u16_t key,
			enum storage_access access)
{
	int i;
	struct bt_keys *keys = NULL;

	if (!bt_addr_le_is_rpa(addr)) {
		for (i = 0; i < key_pool_count; i++) {
			if (!bt_addr_le_cmp(&storage_key_pool[i].addr, addr)) {
				keys = &storage_key_pool[i];
				return keys;
			}
		}
	} else {
		for (i = 0; i < key_pool_count; i++) {
			if (!(storage_key_pool[i].keys & BT_KEYS_IRK)) {
				continue;
			}

			if (!bt_addr_cmp(&addr->a, &storage_key_pool[i].irk.rpa)) {
				return &storage_key_pool[i];
			}

			if (bt_rpa_irk_matches(storage_key_pool[i].irk.val, &addr->a)) {
				bt_addr_copy(&storage_key_pool[i].irk.rpa, &addr->a);
				return &storage_key_pool[i];
			}
		}

	}

	if ((keys == NULL) && (access == STORAGE_WRITE)) {
		for (i = 0; i < key_pool_count; i++) {
			if (!bt_addr_le_cmp(&storage_key_pool[i].addr, BT_ADDR_LE_ANY)) {
				keys = &storage_key_pool[i];
				bt_addr_le_copy(&keys->addr, addr);
				return keys;
			}
		}
		SYS_LOG_WRN("No Key Space In storage key pool");

		keys = &storage_key_pool[(removeCnt++)%key_pool_count];
		bt_addr_le_copy(&keys->addr, addr);
		return keys;
	}

	return NULL;
}

static ssize_t storage_read(const bt_addr_le_t *addr, u16_t key, void *data,
			    size_t length)
{
	ssize_t ret = 0;
	struct bt_keys *keys = NULL;

	if (addr != NULL) {
		keys = find_bt_keys(addr, key, STORAGE_READ);
	}

	switch (key) {
	case BT_STORAGE_ID_ADDR:
	{
		bt_addr_le_copy(data, &storage_local_bt_addr);
		ret = sizeof(storage_local_bt_addr);
	}
	break;
	case BT_STORAGE_LOCAL_IRK:
	{
		memcpy(data, &storage_local_irk, 16);
		ret = 16;
	}
	case BT_STORAGE_SLAVE_LTK:
	{
		if (keys) {
			ret = sizeof(keys->slave_ltk);
			memcpy(data, &keys->slave_ltk, ret);
		}
	}
	break;
	case BT_STORAGE_LTK:
	{
		if (keys) {
			ret = sizeof(keys->ltk);
			memcpy(data, &keys->ltk, ret);
		}
	}
	break;
	case BT_STORAGE_LOCAL_CSRK:
	{
		if (keys) {
			ret = sizeof(keys->local_csrk);
			memcpy(data, &keys->local_csrk, ret);
		}
	}
	break;
	case BT_STORAGE_REMOTE_CSRK:
	{
		if (keys) {
			ret = sizeof(keys->remote_csrk);
			memcpy(data, &keys->remote_csrk, ret);
		}
	}
	case BT_STORAGE_LTK_P256:
	{
		if (keys) {
			ret = sizeof(keys->ltk);
			memcpy(data, &keys->ltk, ret);
		}
	}
	break;
	case BT_STORAGE_BT_KEYS:
	{
		if (keys) {
			ret = sizeof(struct bt_keys);
			memcpy(data, keys, ret);
		}
	}
	break;
	}
	return ret;
}

static ssize_t storage_write(const bt_addr_le_t *addr, u16_t key,
			     const void *data, size_t length)
{
	ssize_t ret = 0;
	struct bt_keys *keys = NULL;

	if (addr != NULL) {
		keys = find_bt_keys(addr, key, STORAGE_WRITE);
	}

	switch (key) {
	case BT_STORAGE_ID_ADDR:
	{
		bt_addr_le_copy(&storage_local_bt_addr, data);
		ret = sizeof(storage_local_bt_addr);
	}
	break;
	case BT_STORAGE_LOCAL_IRK:
	{
		memcpy(&storage_local_irk, data, length);
		ret = length;
	}
	case BT_STORAGE_SLAVE_LTK:
	{
		if (keys) {
			memcpy(&keys->slave_ltk, data, length);
			ret = length;
		}
	}
	break;
	case BT_STORAGE_LTK:
	{
		if (keys) {
			memcpy(&keys->ltk, data, length);
			ret = length;
		}
	}
	break;
	case BT_STORAGE_LOCAL_CSRK:
	{
		if (keys) {
			memcpy(&keys->local_csrk, data, length);
			ret = length;
		}
	}
	break;
	case BT_STORAGE_REMOTE_CSRK:
	{
		if (keys) {
			memcpy(&keys->remote_csrk, data, length);
			ret = length;
		}
	}
	case BT_STORAGE_LTK_P256:
	{
		if (keys) {
			memcpy(&keys->ltk, data, length);
			ret = length;
		}
	}
	break;
	case BT_STORAGE_BT_KEYS:
	{
		if (keys) {
			memcpy(keys, data, length);
			ret = length;
		}
	}
	break;
	}

	return ret;
}

static int storage_clear(const bt_addr_le_t *addr)
{
	int err = 0;
	struct bt_keys *keys = NULL;

	if (addr != NULL)
		keys = find_bt_keys(addr, 0, STORAGE_READ);

	if (keys)
		memset(keys, 0, sizeof(struct bt_keys));

	return err;
}

static int storage_init(struct device *unused)
{
	int i;
	static const struct bt_storage storage = {
		.read  = storage_read,
		.write = storage_write,
		.clear = storage_clear
	};
	storage_local_bt_addr.type = BT_ADDR_LE_RANDOM;
	storage_local_bt_addr.a.val[0] = 0xAA;
	storage_local_bt_addr.a.val[1] = 0xBB;
	storage_local_bt_addr.a.val[2] = 0xCC;
	storage_local_bt_addr.a.val[3] = 0xDD;
	storage_local_bt_addr.a.val[4] = 0xEE;
	storage_local_bt_addr.a.val[5] = 0xFF;
	for (i = 0; i < 16; i++)
		storage_local_irk[i] = i;
	bt_storage_register(&storage);

	return 0;
}
#endif

SYS_INIT(storage_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
