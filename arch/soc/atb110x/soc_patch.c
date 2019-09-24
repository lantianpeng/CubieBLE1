/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc suspend interface for Actions SoC
 */

#include <kernel.h>
#include <init.h>
#include <power.h>
#include <soc.h>

#define SYS_LOG_DOMAIN "patch"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

__init_once_text void patch_hw_func(void)
{
	struct function_patch_t *__patch_start, *__patch_end, *tmp;
	int i, function_patch_num;

	__patch_start = (struct function_patch_t *)Image$$RW_IRAM_PATCH_HW_FUNC$$Base;
	__patch_end = (struct function_patch_t *)Image$$RW_IRAM_PATCH_HW_FUNC$$Limit;
	function_patch_num = ((u32_t)__patch_end - (u32_t)__patch_start) / sizeof(struct function_patch_t);

	for (i = 0, tmp = __patch_start; i < function_patch_num; i++, tmp++) {
		FUNREPLACE->func[i] = ((u32_t)tmp->new_function) & (~1);
		FUNREPLACE->FIXADDR[i] = ((u32_t)tmp->old_function) & (~1);
		FUNREPLACE->CTL |= (0x1<<i);
	}
	SYS_LOG_INF("CTL:0x%x", FUNREPLACE->CTL);
}

void dump_hw_func(void)
{
	int i;

	for (i = 0; i < 12; i++) {
		SYS_LOG_INF("FUNREPLACE[%d]:", i);
		SYS_LOG_INF("FIXADDR:0x%x, ", FUNREPLACE->FIXADDR[i]);
		SYS_LOG_INF("func:0x%x", FUNREPLACE->func[i]);
	}
	SYS_LOG_INF("CTL:0x%x", FUNREPLACE->CTL);
}

__init_once_text void patch_hw_code(void)
{
	struct code_patch_t *__patch_start, *__patch_end, *tmp;
	int i, code_patch_num;

	__patch_start = (struct code_patch_t *)Image$$RW_IRAM_PATCH_HW_CODE$$Base;
	__patch_end = (struct code_patch_t *)Image$$RW_IRAM_PATCH_HW_CODE$$Limit;
	code_patch_num = ((u32_t)__patch_end - (u32_t)__patch_start) / sizeof(struct code_patch_t);

	for (i = 0, tmp = __patch_start; i < code_patch_num; i++, tmp++) {
		CODEREPLACE->Instr[i] = (u32_t)tmp->new_code;
		CODEREPLACE->FIXADDR[i] = (u32_t)tmp->old_code_addr;
		CODEREPLACE->CTL |= (0x1<<i);
	}
	SYS_LOG_INF("CTL:0x%x", CODEREPLACE->CTL);
}

extern void patch_sys_soc_suspend(void);
__init_once_text void patch_sw(void)
{
#if CONFIG_DEEPSLEEP
	patch_sys_soc_suspend();
#endif
}
