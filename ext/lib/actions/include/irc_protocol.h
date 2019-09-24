/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IRC_PROTOCOL_H__
#define __IRC_PROTOCOL_H__


#define MIN_FIRST_CODE_NUM (20)


/* 9012 protocol feature */
#define		_9012_RPC_0				4500
#define		_9012_RPC_1				4500
#define		_9012_LDC_0				4500
#define		_9012_LDC_1				4500
#define		_9012_LOGIC_1			1690
#define		_9012_LOGIC_0			560
#define		_9012_COM					560
#define		_9012_CODE_NUM		32
#define		_9012_TOGGLE	    0

/* nec protocol feature */
#define		NEC_RPC_0				9000
#define		NEC_RPC_1				2000
#define		NEC_LDC_0				9000
#define		NEC_LDC_1				4500
#define		NEC_LOGIC_1			1690
#define		NEC_LOGIC_0			560
#define		NEC_TOGGLE	    0		
#define		NEC_COM					560
#define		NEC_CODE_NUM		32

/* rc5 protocol feature */
#define		RC5_RPC_0				844
#define		RC5_RPC_1				844
#define		RC5_LDC_0				844
#define		RC5_LDC_1				844
#define		RC5_LOGIC_1			844
#define		RC5_LOGIC_0			844
#define		RC5_TOGGLE			0
#define		RC5_COM					844
#define		RC5_CODE_NUM		11

/* rc6 protocol feature */
#define		RC6_RPC_0				0
#define		RC6_RPC_1       0
#define		RC6_LDC_0				2666
#define		RC6_LDC_1				889
#define		RC6_LOGIC_1			0
#define		RC6_LOGIC_0			0
#define		RC6_TOGGLE			889
#define		RC6_COM					444
#define		RC6_CODE_NUM		15


/* 50462 protocol feature */
#define		_50462_RPC_0				0
#define		_50462_RPC_1       0
#define		_50462_LDC_0				0
#define		_50462_LDC_1				0
#define		_50462_LOGIC_1			1799
#define		_50462_LOGIC_0			780
#define		_50462_TOGGLE			0
#define		_50462_COM					260
#define		_50462_CODE_NUM		16

/* 7461 protocol feature */
#define		_7461_RPC_0				0
#define		_7461_RPC_1       0
#define		_7461_LDC_0				9000
#define		_7461_LDC_1				4500
#define		_7461_LOGIC_1			1680
#define		_7461_LOGIC_0			560
#define		_7461_TOGGLE			0
#define		_7461_COM					560
#define		_7461_CODE_NUM		42

/* 50560 protocol feature */
#define		_50560_RPC_0				0
#define		_50560_RPC_1       0
#define		_50560_LDC_0				8000
#define		_50560_LDC_1				4000
#define		_50560_LOGIC_1			1560
#define		_50560_LOGIC_0			520
#define		_50560_TOGGLE			0
#define		_50560_COM					520
#define		_50560_CODE_NUM		16

u8_t is_nec_wave(u16_t *pulse_tab, u16_t tab_len);

u8_t is_9012_protocol(u16_t *pulse_tab, u16_t tab_len);

u8_t is_rc5_protocol(u16_t *pulse_tab, u16_t tab_len);

u8_t is_rc6_protocol(u16_t *pulse_tab, u16_t tab_len);

u8_t is_50462_protocol(u16_t *pulse_tab, u16_t tab_len);

u8_t is_7461_protocol(u16_t *pulse_tab, u16_t tab_len);

u8_t is_50560_protocol(u16_t *pulse_tab, u16_t tab_len);

#endif   /* __IRC_PROTOCOL_H__ */
