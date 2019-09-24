/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ADC configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PATCH_H_
#define	_ACTIONS_SOC_PATCH_H_

#ifndef _ASMLANGUAGE

#define     __IO    volatile             /*!< Defines 'read / write' permissions */

#define MEM_CTL_BASE (0x40009000)

#define FUNREPLACE_NUM_MAX 12
typedef struct {
	__IO u32_t CTL;          /* Offset: 0x0070 (R/W)  CodeReplace CTL */
	u32_t RESERVE1[3];
	__IO u32_t func[FUNREPLACE_NUM_MAX];           /* Offset: 0x80~0xAC (R/W)  The alternate instruction code. */
	__IO u32_t FIXADDR[FUNREPLACE_NUM_MAX];           /* Offset: 0xB0~0xDC (R/W)  Fix address */
} FUNREPLACE_TypeDef;

#define FUNREPLACE				((FUNREPLACE_TypeDef *) (MEM_CTL_BASE + 0x70))

#define CODEREPLACE_NUM_MAX 12
typedef struct {
	__IO u32_t CTL;          /* Offset: 0x0C (R/W)  CodeReplace CTL */
	__IO u32_t Instr[CODEREPLACE_NUM_MAX];           /* Offset: 0x10~0x3C (R/W)  The alternate instruction code. */
	__IO u32_t FIXADDR[CODEREPLACE_NUM_MAX];           /* Offset: 0x040~0x6C (R/W)  Fix address */
} CODEREPLACE_TypeDef;

#define CODEREPLACE				((CODEREPLACE_TypeDef *) (MEM_CTL_BASE + 0x0c))

struct function_patch_t {
	void *new_function;
	void *old_function;
};

struct code_patch_t {
	void *new_code;
	void *old_code_addr;
};

#define FUNCTION_PATCH_REGISTER(name, patch, function) \
	static struct function_patch_t __function_patch_##name __used \
__attribute__((used, section(".patch_hw_func")))  = { \
		  .new_function = (void *)patch, \
		  .old_function = (void *)function \
	}

#define CODE_PATCH_REGISTER(name, code, addr) \
	static struct code_patch_t __code_patch_##name __used \
__attribute__((used, section(".patch_hw_code")))  = { \
		  .new_code = (void *)code, \
		  .old_code_addr = (void *)addr \
	}

extern struct shell_module Image$$RW_IRAM_PATCH_HW_FUNC$$Base[];
extern struct shell_module Image$$RW_IRAM_PATCH_HW_FUNC$$Limit[];

extern struct code_patch_t Image$$RW_IRAM_PATCH_HW_CODE$$Base[];
extern struct code_patch_t Image$$RW_IRAM_PATCH_HW_CODE$$Limit[];

void patch_hw_func(void);
void patch_hw_code(void);
void patch_sw(void);


#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_PATCH_H_	*/
