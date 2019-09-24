/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for IRC
 */

#ifndef __IRC_ACTS_H__
#define __IRC_ACTS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define     IRC_REG_BASE                                                       0x40010000
#define     IRC_RX_CTL                                                        (IRC_REG_BASE+0x0000)
#define     IRC_RX_STA                                                        (IRC_REG_BASE+0x0004)
#define     IRC_RX_CC                                                         (IRC_REG_BASE+0x0008)
#define     IRC_RX_KDC                                                        (IRC_REG_BASE+0x000C)
#define     IRC_ANA_CTL                                                       (IRC_REG_BASE+0x0010)
#define     IRC_RX_DBG                                                        (IRC_REG_BASE+0x0014)
#define     IRC_TX_CTL                                                        (IRC_REG_BASE+0x0100)
#define     IRC_TX_STA                                                        (IRC_REG_BASE+0x0104)
#define     IRC_TX_CC                                                         (IRC_REG_BASE+0x0108)
#define     IRC_TX_KDC                                                        (IRC_REG_BASE+0x010c)

#define		TC9012_MODE					0x00
#define		NEC_MODE						0x01
#define		RC5_MODE						0x02
#define		RC6_MODE						0x03

#define IRC_CTRL_IIE						(0x1 << 2)
#define IRC_CTRL_IRE						(0x1 << 3)
#define IRC_CTRL_TX_RPC_EN			(0x1 << 4)
#define IRC_CTRL_TX_MODE_FREQ		(0x3 << 5)
#define IRC_CTRL_TX_MODE_EN			(0x1 << 8)
#define IRC_CTRL_TX_RELEASE			(0x1 << 9)


#define IRC_RX_CTRL_IIE						(0x1 << 2)
#define IRC_RX_CTRL_IRE						(0x1 << 3)

#define		IRC_RX_DEMOD_FREQ			12
#define		IRC_RX_DET_PRO_PD			10
#define		IRC_RX_PROTOCOL				8
#define		IRC_RX_UCMP						6
#define		IRC_RX_KDCM						5
#define		IRC_RX_RCD						4
#define		IRC_RX_IIP						2
#define		IRC_RX_IREP						0
#define   IO_MFP_BASE           0x40016000


#define     IR_TX_IO_CTL                                                      GPIO_REG_CTL(IO_MFP_BASE, CONFIG_IRC_TX_PIN)
#define     IR_RX_IO_CTL                                                      GPIO_REG_CTL(IO_MFP_BASE, CONFIG_IRC_RX_PIN)

struct sw_acts_irc_config {
	u32_t	base;
	u8_t	clock_id;
	u8_t	reset_id;
	u8_t  rx_clock_id;
};

#define MAX_PULSE_LEN 100

struct sw_acts_irc_data {
	u8_t busy;
	u8_t key_state;
	u8_t learn_mode;
	u16_t key_code;
	u16_t pre_key_code;
	u32_t old_mfp_ctl;
	input_notify_t notify;
	struct device *gpio_dev;
	struct device *pwm_dev;
	u16_t *p_pulse_tab;
	u16_t pulse_tab_len;
	u16_t cur_read_pulse_tab_index;
};


#define IR_KEY_DOWN 1
#define IR_KEY_UP   0


#ifdef __cplusplus
}
#endif

#endif	/* __IRC_ACTS_H__ */
