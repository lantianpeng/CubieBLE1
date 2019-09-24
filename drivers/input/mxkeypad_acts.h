/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for mxkeypad
 */

#ifndef __MXKEYPAD_ACTS_H__
#define __MXKEYPAD_ACTS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* KEY_CTL bitfield */
#define KEY_CTL_WTS_SHIFT		(21)
#define KEY_CTL_WTS_MASK		(0x7)
#define KEY_CTL_WTS(x)			(((x) & KEY_CTL_WTS_MASK) << KEY_CTL_WTS_SHIFT)
#define KEY_CTL_DTS_SHIFT		(17)
#define KEY_CTL_DTS_MASK		(0x7)
#define KEY_CTL_DTS(x)			(((x) & KEY_CTL_DTS_MASK) << KEY_CTL_DTS_SHIFT)
#define KEY_CTL_IRQ_PD			(0x1 << 15)
#define KEY_CTL_IRQ_EN			(0x1 << 14)
#define KEY_CTL_LF_EN(x)		((x) << 13)
#define KEY_CTL_OTYP			(0x1 << 12)
#define KEY_CTL_PIN_EN_SHIFT		(4)
#define KEY_CTL_PIN_EN_MASK		(0xff)
#define KEY_CTL_PIN_EN(x)		(((x) & KEY_CTL_PIN_EN_MASK) << KEY_CTL_PIN_EN_SHIFT)
#define KEY_CTL_MODE_MARTRIX		(0x1 << 1)
#define KEY_CTL_EN			(0x1 << 0)

#define WAIT_TIME_BASE_STEP_MS		(32)
#define DEBOUNCE_TIME_BASE_STEP_MS	(10)


struct mxkeypad_map {
	u32_t scan_key[3];
	u16_t key_code;
};

struct acts_mxkeypad_controller {
	volatile u32_t ctrl;
	volatile u32_t info[3];
};

struct acts_mxkeypad_config {
	struct acts_mxkeypad_controller *base;
	void (*irq_config)(void);
	u16_t clock_id;
	u16_t reset_id;

	u16_t pin_mask;
	u16_t sample_wait_ms;
	u16_t sample_debounce_ms;
	u16_t poll_interval_ms;

	u8_t use_low_frequency;
	u8_t reserved;
	u16_t key_cnt;
	const struct mxkeypad_map *key_maps;
};

struct acts_mxkeypad_data {
	struct k_timer timer;
	input_notify_t notify;
	u32_t last_key_down[3];
	u16_t prev_stable_keycode;
};
extern u16_t mxkeypad_acts_get_keycode(const struct acts_mxkeypad_config *cfg, u32_t *scan_key);
extern void mxkeypad_acts_report_key(const struct acts_mxkeypad_config *cfg, struct acts_mxkeypad_data *keypad, int key_code, int value);
extern void mxkeypad_acts_isr(struct device *dev);
extern void mxkeypad_acts_wakeup_isr(struct device *dev);
extern const struct input_dev_driver_api mxkeypad_acts_driver_api;
extern int mxkeypad_acts_init(struct device *dev);
struct acts_mxkeypad_data mxkeypad_acts_ddata;

void mxkeypad_acts_irq_config(void);

#ifdef __cplusplus
}
#endif

#endif	/* __MXKEYPAD_ACTS_H__ */
