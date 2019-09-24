/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "board.h"

extern u8_t deepsleep_disable_vd12;
vdd_val_t normal_vdd_val = VDD_85;
vdd_val_t dp_vdd_val = VDD_80;

static __init_once_data const struct acts_pin_config board_pin_config[] = {
	BOARD_PIN_CONFIG
};

__init_once_text void set_ic_feature(void)
{
	u8_t ic_type;

	ic_type = (u8_t)acts_efuse_read(0);

	switch (ic_type) {
	case 0:
		break;
	case 1:
#if CONFIG_DEEPSLEEP
		deepsleep_disable_vd12 = 1;
#endif
		break;
	default:
		break;
	}
}

__init_once_text int board_early_init(struct device *arg)
{
	/* set ic feature according to ic type */
	set_ic_feature();

	/* set vdd */
	acts_set_vdd(normal_vdd_val);
	acts_delay_us(100);
	
	/* init vd12*/
	acts_request_vd12_pd(false);
	acts_request_vd12_largebias(false);

	/* disable rc_3M */
	acts_request_rc_3M(false);

	/* remap vector */
	sys_write32(REMAP_VECTOR_START, VECTOR_BASE);
	sys_write32(sys_read32(MEM_CTL) | MEM_CTL_VECTOR_TABLE_SEL, MEM_CTL);

	/* init pinmux according to board_pin_config */
	acts_pinmux_setup_pins(board_pin_config, ARRAY_SIZE(board_pin_config));

#ifdef CONFIG_USE_JTAG_IO_FOR_KEY
	/* disable jtag_en */
	sys_write32(0, JTAG_EN);
#endif

	/* apply hw func&code patch */
	patch_hw_func();
	patch_hw_code();
	patch_sw();

	/* enable tick interrupt */
	IRQ_CONNECT(IRQ_ID_TIMER0, 0, timer_irq_handler, NULL, 0);

	return 0;
}

SYS_INIT(board_early_init, PRE_KERNEL_1, 0);
