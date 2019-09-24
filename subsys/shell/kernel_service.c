/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if CONFIG_KERNEL_SHELL

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>

#define SYS_LOG_DOMAIN "shell"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#define SHELL_KERNEL "kernel"

int shell_cmd_version(int argc, char *argv[]);
int shell_cmd_uptime(int argc, char *argv[]);
int shell_cmd_cycles(int argc, char *argv[]);
int shell_cmd_stack(int argc, char *argv[]);
extern int get_llcc_debug_info(int argc, char *argv[]);

#if defined(CONFIG_CMD_MEMORY)
#include "errno.h"
#define DISP_LINE_LEN	16
static int do_mem_mw(int width, int argc, char * const argv[])
{
	unsigned long writeval;
	unsigned long addr, count;
	void *buf;

	if (argc < 3)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);
	writeval = strtoul(argv[2], NULL, 16);

	if (argc == 4)
		count = strtoul(argv[3], NULL, 16);
	else
		count = 1;

	buf = (void *)addr;
	while (count-- > 0) {
		if (width == 4)
			*((uint32_t *)buf) = (uint32_t)writeval;
		else if (width == 2)
			*((uint16_t *)buf) = (uint16_t)writeval;
		else
			*((uint8_t *)buf) = (uint8_t)writeval;
		buf += width;
	}

	return 0;
}

static int do_mem_md(int width, int argc, char *argv[])
{
	unsigned long addr;
	int count;

	if (argc < 2)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);

	if (argc == 3)
		count = strtoul(argv[2], NULL, 16);
	else
		count = 1;

	print_buffer((void *)addr, width, count, DISP_LINE_LEN / width, -1);

	return 0;
}

static int shell_cmd_mdw(int argc, char *argv[])
{
	return do_mem_md(4, argc, argv);
}

static int shell_cmd_mdh(int argc, char *argv[])
{
	return do_mem_md(2, argc, argv);
}

static int shell_cmd_mdb(int argc, char *argv[])
{
	return do_mem_md(1, argc, argv);
}

static int shell_cmd_mww(int argc, char *argv[])
{
	return do_mem_mw(4, argc, argv);
}

static int shell_cmd_mwh(int argc, char *argv[])
{
	return do_mem_mw(2, argc, argv);
}

static int shell_cmd_mwb(int argc, char *argv[])
{
	return do_mem_mw(1, argc, argv);
}
#endif	/* CONFIG_CMD_MEMORY */

#include <bluetooth/bluetooth.h>
#include "nvram_config.h"
#include "errno.h"

extern int str2bt_addr(const char *str, bt_addr_t *addr);

static int shell_set_bt_addr(int argc, char *argv[])
{
	int err;
	bt_addr_t addr;

	if (argc < 2)
		return -EINVAL;

	err = str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_ERR("Invalid address (err %d)", err);
		return err;
	}

	err = nvram_config_set_factory("BT_ADDR", argv[1], strlen(argv[1]));
	if (err < 0)
		return err;

	return 0;
}

#include <soc.h>
static int shell_cmd_reboot(int argc, char *argv[])
{
	int type;

	if (argc < 2)
		return -EINVAL;

	/* default boot type */
	type = REBOOT_TYPE_GOTO_APP;

	SYS_LOG_INF("reboot type: %s", argv[1]);

	if (strcmp("app", argv[1]) == 0)
		type = REBOOT_TYPE_GOTO_APP;
	else if (strcmp("dtm", argv[1]) == 0)
		type = REBOOT_TYPE_GOTO_DTM;
	else if (strcmp("adfu", argv[1]) == 0)
		type = REBOOT_TYPE_GOTO_ADFU;

	SYS_LOG_INF("reboot type: %d", type);
	sys_pm_reboot(type);

	return 0;
}

#if defined(CONFIG_SPICACHE_PROFILE)

#include <spicache.h>

static struct spicache_profile profile_data;

/*
 * cmd: spicache_profile
 *   start start_addr end_addr
 *   stop
 */
static int shell_cmd_spicache_profile(int argc, char *argv[])
{
	struct spicache_profile *profile;
	int len = strlen(argv[1]);

	profile = &profile_data;
	if (!strncmp(argv[1], "start", len)) {
		if (argc < 4)
			return -EINVAL;

		profile->start_addr = strtoul(argv[2], NULL, 0);
		profile->end_addr = strtoul(argv[3], NULL, 0);

		printk("Start profile: addr range %08x ~ %08x\n",
			profile->start_addr, profile->end_addr);

		spicache_profile_start(profile);

	} else if (!strncmp(argv[1], "stop", len)) {
		printk("Stop profile\n");
		spicache_profile_stop(profile);
		spicache_profile_dump(profile);
	} else {
		printk("usage:\n");
		printk("  spicache_profile start spicache_flash_start_addr flash_end_addr\n");
		printk("  spicache_profile stop\n");

		return -EINVAL;
	}

	return 0;
}
#endif	/* CONFIG_SPICACHE_PROFILE */

struct shell_cmd kernel_commands[] = {
	{ "version", shell_cmd_version, "show kernel version" },
	{ "uptime", shell_cmd_uptime, "show system uptime in milliseconds" },
	{ "cycles", shell_cmd_cycles, "show system hardware cycles" },
#if defined(CONFIG_OBJECT_TRACING) && defined(CONFIG_THREAD_MONITOR)
	{ "tasks", shell_cmd_tasks, "show running tasks" },
#endif
	{ "stacks", shell_cmd_stack, "show system stacks" },
	{ "debugInfo", get_llcc_debug_info, "none" },

#if defined(CONFIG_CMD_MEMORY)
	{ "mdw", shell_cmd_mdw, "display memory by word: mdw address [,count]" },
	{ "mdh", shell_cmd_mdh, "display memory by half-word: mdh address [,count]" },
	{ "mdb", shell_cmd_mdb, "display memory by byte: mdb address [,count]" },

	{ "mww", shell_cmd_mww, "memory write (fill) by word: mww address value [,count]" },
	{ "mwh", shell_cmd_mwh, "memory write (fill) by half-word: mwh address value [,count]" },
	{ "mwb", shell_cmd_mwb, "memory write (fill) by byte: mwb address value [,count]" },
#endif
	{ "set_bt_addr", shell_set_bt_addr, "<address: XX:XX:XX:XX:XX:XX>" },
#if defined(CONFIG_SPICACHE_PROFILE)
	{ "spicache_profile", shell_cmd_spicache_profile, "profile spicache hit rate" },
#endif
	{ "reboot", shell_cmd_reboot, "reboot system" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(SHELL_KERNEL, kernel_commands);
#endif
