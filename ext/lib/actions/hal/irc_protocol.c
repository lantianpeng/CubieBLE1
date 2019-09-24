/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <misc/printk.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>

#include <string.h>
#include "irc_protocol.h"

#ifdef CONFIG_IRC_SW_RX

#define ESP  	(20)
#define ESP_L  (100 - ESP)
#define ESP_R  (100 + ESP)
#define VAL_ESP_L(a) ((a) * ESP_L / 100)
#define VAL_ESP_R(a) ((a) * ESP_R / 100)

void generate_nec_wave(u16_t *pulse_tab, u16_t tab_len, u64_t data, u16_t bits_of_data)
{
	u32_t mask;
	u8_t i = 0;

	/* header */
	pulse_tab[i++] = NEC_LDC_0;
	pulse_tab[i++] = NEC_LDC_1;
	
	/* code */
	for (mask = (1UL << (bits_of_data - 1));  mask;  mask >>= 1) {
		if (data & mask) {
			pulse_tab[i++] = NEC_COM;
			pulse_tab[i++] = NEC_LOGIC_1;
		} else {
			pulse_tab[i++] = NEC_COM;
			pulse_tab[i++] = NEC_LOGIC_0;
		}
	}
	
	/* stop burst */
	pulse_tab[i++] = NEC_COM;
	
	printk("generate_nec_wave\n");
}

u8_t is_nec_wave(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i, match;
	u32_t data   = 0;
		
	/* check length */
	if(tab_len != (NEC_CODE_NUM * 2 + 2 + 1)) 
		return false;

	/* check LDC0 */
	match = (pulse_tab[0] > VAL_ESP_L(NEC_LDC_0)) && (pulse_tab[0] < VAL_ESP_R(NEC_LDC_0));
	if(!match) 
		return false;

	/* check LDC1 */
	match = (pulse_tab[1] > VAL_ESP_L(NEC_LDC_1)) && (pulse_tab[1] < VAL_ESP_R(NEC_LDC_1));
	if(!match) 
		return false;
	
	/* check cmd */
	for (i = 0; i < (NEC_CODE_NUM * 2); i = i+2) {
		match = ((pulse_tab[i+2] > VAL_ESP_L(NEC_COM)) && (pulse_tab[i+2] < VAL_ESP_R(NEC_COM)));
		if (!match)
			return false;
		
		if((pulse_tab[i+3] > VAL_ESP_L(NEC_LOGIC_0)) && (pulse_tab[i+3] < VAL_ESP_R(NEC_LOGIC_0)))
			data = (data << 1) | 0;
		else if ((pulse_tab[i+3] > VAL_ESP_L(NEC_LOGIC_1)) && (pulse_tab[i+3] < VAL_ESP_R(NEC_LOGIC_1)))
			data = (data << 1) | 1;
		else 
			return false;
	}
	
	generate_nec_wave(pulse_tab, tab_len, data, NEC_CODE_NUM);
	
	printk("nec code : 0x%x\n", data);
	return true;
}

void generate_9012_wave(u16_t *pulse_tab, u16_t tab_len, u64_t data, u16_t bits_of_data)
{
	u32_t mask;
	u8_t i = 0;

	/* header */
	pulse_tab[i++] = _9012_LDC_0;
	pulse_tab[i++] = _9012_LDC_1;
	
	/* code */
	for (mask = (1UL << (bits_of_data - 1));  mask;  mask >>= 1) {
		if (data & mask) {
			pulse_tab[i++] = _9012_COM;
			pulse_tab[i++] = _9012_LOGIC_1;
		} else {
			pulse_tab[i++] = _9012_COM;
			pulse_tab[i++] = _9012_LOGIC_0;
		}
	}
	
	/* stop burst */
	pulse_tab[i++] = _9012_COM;
	
	printk("generate_9012_wave\n");
}

u8_t is_9012_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i, match;
	u32_t data   = 0;
	
	/* check length */
	if(tab_len != (_9012_CODE_NUM * 2 + 2 + 1)) 
		return false;
	
	/* check LDC0 */
	match = (pulse_tab[0] > VAL_ESP_L(_9012_LDC_0)) && (pulse_tab[0] < VAL_ESP_R(_9012_LDC_0));
	if(!match) 
		return false;

	/* check LDC1 */
	match = (pulse_tab[1] > VAL_ESP_L(_9012_LDC_1)) && (pulse_tab[1] < VAL_ESP_R(_9012_LDC_1));
	if(!match) 
		return false;
	
	/* check code */
	for (i = 0; i < (_9012_CODE_NUM * 2); i = i + 2) {
		match = ((pulse_tab[i+2] > VAL_ESP_L(_9012_COM)) && (pulse_tab[i+2] < VAL_ESP_R(_9012_COM)));
		if (!match)
			return false;

		if ((pulse_tab[i+3] > VAL_ESP_L(_9012_LOGIC_0)) && (pulse_tab[i+3] < VAL_ESP_R(_9012_LOGIC_0)))
			data = (data << 1) | 0;
		else if ((pulse_tab[i+3] > VAL_ESP_L(_9012_LOGIC_1)) && (pulse_tab[i+3] < VAL_ESP_R(_9012_LOGIC_1)))
			data = (data << 1) | 1;
		else 
			return false;
	}
	
	generate_9012_wave(pulse_tab, tab_len, data, _9012_CODE_NUM);
	
	printk("9012 code : 0x%x\n", data);
	return true;
}
 
#define SPACE_LEVEL 0
#define CARRIER_LEVEL 1
static inline int8_t get_pulse_level(u16_t pulse_width, u8_t *offset, u8_t *used_len, u16_t RC_COM)
{
	u8_t len;
	u8_t pulse_level = (*offset) % 2 ? SPACE_LEVEL : CARRIER_LEVEL;
	if ((pulse_width > VAL_ESP_L(RC_COM)) && (pulse_width < VAL_ESP_R(RC_COM)))
		len = 1;
	else if ((pulse_width > VAL_ESP_L(RC_COM * 2)) && (pulse_width < VAL_ESP_R(RC_COM * 2)))
		len = 2;
	else if ((pulse_width > VAL_ESP_L(RC_COM * 3)) && (pulse_width < VAL_ESP_R(RC_COM * 3)))
		len = 3;
	else 
		return -1;

	(*used_len)++;
	if (*used_len >= len) {
		*used_len = 0;
		(*offset)++;
	}

	return pulse_level;
}

u8_t is_rc5_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i = 0;
	u32_t  data = 0;
	u8_t used_len = 0;
	u8_t offset = 0;
	
	/* check length */
	if (tab_len < RC5_CODE_NUM + 2)
		return false;
	
	/* check start bit */
	if (get_pulse_level(pulse_tab[offset], &offset, &used_len, RC5_COM) != CARRIER_LEVEL)
		return false ;
	if (get_pulse_level(pulse_tab[offset], &offset, &used_len, RC5_COM) != SPACE_LEVEL)
		return false ;
	if (get_pulse_level(pulse_tab[offset], &offset, &used_len, RC5_COM) != CARRIER_LEVEL)
		return false ;

	for (i = 0;  offset < tab_len;  i++) {
		int8_t  levelA = get_pulse_level(pulse_tab[offset], &offset, &used_len, RC5_COM);
		int8_t  levelB;
		if (offset == tab_len) { 
			levelB = SPACE_LEVEL;
		} else {
			levelB = get_pulse_level(pulse_tab[offset], &offset, &used_len, RC5_COM);
		}

		if ((levelA == SPACE_LEVEL) && (levelB == CARRIER_LEVEL))
			data = (data << 1) | 1;
		else if ((levelA == CARRIER_LEVEL ) && (levelB == SPACE_LEVEL))
			data = (data << 1) | 0;
		else
			return false ;
	}

	printk("rc5 code : 0x%x\n", data);
	return true;
}

u8_t is_rc6_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i = 0;
	u8_t match;
	u32_t data = 0;
	u8_t used_len = 0;
	u8_t offset = 0;  

	if (tab_len < RC6_CODE_NUM)
		return false ;

	/* check LDC0 */
	match = (pulse_tab[offset] > VAL_ESP_L(RC6_LDC_0)) && (pulse_tab[offset] < VAL_ESP_R(RC6_LDC_0));
	if(!match) 
		return false;
	offset++;

	/* check LDC1 */
	match = (pulse_tab[offset] > VAL_ESP_L(RC6_LDC_1)) && (pulse_tab[offset] < VAL_ESP_R(RC6_LDC_1));
	if(!match) 
		return false;
	offset++;
	
	/* check startbit */
	if (get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM) != CARRIER_LEVEL)
		return false ;
	if (get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM) != SPACE_LEVEL)
		return false ;

	for (i = 0;  offset < tab_len;  i++) {
		int8_t  levelA, levelB;  

		levelA = get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM);
		if (i == 3) {
			if (levelA != get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM))
				return false;
		}

		levelB = get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM);
		if (i == 3) {
			if (levelB != get_pulse_level(pulse_tab[offset], &offset, &used_len, RC6_COM))
				return false;
		}

		if ((levelA == CARRIER_LEVEL ) && (levelB == SPACE_LEVEL))
			data = (data << 1) | 1 ;  
		else if ((levelA == SPACE_LEVEL) && (levelB == CARRIER_LEVEL ))
			data = (data << 1) | 0 ;  
		else 
			return false ;            
	}
	
	printk("rc6 code : 0x%x\n", data);
	
	return true;
}

void generate_50462_wave(u16_t *pulse_tab, u16_t tab_len, u64_t data, u16_t bits_of_data)
{
	u32_t mask;
	u8_t i = 0;

	/* code */
	for (mask = (1UL << (bits_of_data - 1));  mask;  mask >>= 1) {
		if (data & mask) {
			pulse_tab[i++] = _50462_COM;
			pulse_tab[i++] = _50462_LOGIC_1;
		} else {
			pulse_tab[i++] = _50462_COM;
			pulse_tab[i++] = _50462_LOGIC_0;
		}
	}
	
	/* stop burst */
	pulse_tab[i++] = _50462_COM;
	
	printk("generate_50462_wave\n");
}

u8_t is_50462_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i, match;
	u32_t data   = 0;

	/* check length */
	if(tab_len != (_50462_CODE_NUM * 2 +1))
		return false;
	
	/* check code */
	for (i = 0; i < (_50462_CODE_NUM * 2); i = i + 2) { 
		match = (pulse_tab[i] > VAL_ESP_L(_50462_COM)) && (pulse_tab[i] < VAL_ESP_R(_50462_COM));
		if (!match) 
			return false;
		
		if ((pulse_tab[i + 1] > VAL_ESP_L(_50462_LOGIC_0)) && (pulse_tab[i + 1] < VAL_ESP_R(_50462_LOGIC_0)))
			data = (data << 1) | 0;
		else if ((pulse_tab[i + 1] > VAL_ESP_L(_50462_LOGIC_1)) && (pulse_tab[i + 1] < VAL_ESP_R(_50462_LOGIC_1)))
			data = (data << 1) | 1;
		else 
			return false;
	}
	
	generate_50462_wave(pulse_tab, tab_len, data, _50462_CODE_NUM);
	
	printk("50462 code : 0x%x\n", data);
	return true;
}

void generate_7461_wave(u16_t *pulse_tab, u16_t tab_len, u64_t data, u16_t bits_of_data)
{
	u32_t mask;
	u8_t i = 0;

	/* header */
	pulse_tab[i++] = _7461_LDC_0;
	pulse_tab[i++] = _7461_LDC_1;
	
	/* code */
	for (mask = (1UL << (bits_of_data - 1));  mask;  mask >>= 1) {
		if (data & mask) {
			pulse_tab[i++] = _7461_COM;
			pulse_tab[i++] = _7461_LOGIC_1;
		} else {
			pulse_tab[i++] = _7461_COM;
			pulse_tab[i++] = _7461_LOGIC_0;
		}
	}
	
	/* stop burst */
	pulse_tab[i++] = _7461_COM;
	
	printk("generate_7461_wave\n");
}

u8_t is_7461_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i, match;
	u64_t data   = 0;

	/* check length */
	if(tab_len != (_7461_CODE_NUM * 2 + 2 + 1)) 
		return false;
	
	/* check LDC0 */
	match = (pulse_tab[0] > VAL_ESP_L(_7461_LDC_0)) && (pulse_tab[0] < VAL_ESP_R(_7461_LDC_0));
	if (!match) 
		return false;

	/* check LDC1 */
	match = (pulse_tab[1] > VAL_ESP_L(_7461_LDC_1)) && (pulse_tab[1] < VAL_ESP_R(_7461_LDC_1));
	if (!match) 
		return false;

	/* check code */
	for (i = 0; i < (_7461_CODE_NUM * 2); i = i + 2) {
		match = (pulse_tab[i+2] > VAL_ESP_L(_7461_COM)) && (pulse_tab[i+2] < VAL_ESP_R(_7461_COM));
		if (!match)
			return false;
		
		if ((pulse_tab[i+3] > VAL_ESP_L(_7461_LOGIC_0)) && (pulse_tab[i+3] < VAL_ESP_R(_7461_LOGIC_0)))
			data = (data << 1) | 0;
		else if ((pulse_tab[i+3] > VAL_ESP_L(_7461_LOGIC_1)) && (pulse_tab[i+3] < VAL_ESP_R(_7461_LOGIC_1)))
			data = (data << 1) | 1;
		else 
			return false;
	}
	
	generate_7461_wave(pulse_tab, tab_len, data, _7461_CODE_NUM);
	
	printk("7461 code : 0x%x%x\n", (u32_t)(data >> 32),(u32_t)data);
	return true;
}

void generate_50560_wave(u16_t *pulse_tab, u16_t tab_len, u64_t data, u16_t bits_of_data)
{
	u32_t mask;
	u8_t i = 0;

	/* header */
	pulse_tab[i++] = _50560_LDC_0;
	pulse_tab[i++] = _50560_LDC_1;
	
	/* code */
	for (mask = (1UL << (bits_of_data - 1));  mask;  mask >>= 1) {
		if (i == 18) {
			/* skip segment code */
			i = i + 2; 
		}
		if (data & mask) {
			pulse_tab[i++] = _50560_COM;
			pulse_tab[i++] = _50560_LOGIC_1;
		} else {
			pulse_tab[i++] = _50560_COM;
			pulse_tab[i++] = _50560_LOGIC_0;
		}
	}
	
	/* stop burst */
	pulse_tab[i++] = _50560_COM;
	
	printk("generate_50560_wave\n");
}

/* SC50560-001,003P */
u8_t is_50560_protocol(u16_t *pulse_tab, u16_t tab_len)
{
	u8_t i, match;
	u32_t data   = 0;
	/* check length */
	if(tab_len != (_50560_CODE_NUM * 2 + 2 + 2 + 1)) 
		return false;
	
		/* check LDC0 */
	match = (pulse_tab[0] > VAL_ESP_L(_50560_LDC_0)) && ((pulse_tab[0] < VAL_ESP_R(_50560_LDC_0)));
	if (!match)
		return false;

	/* check LDC1 */
	match = (pulse_tab[1] > VAL_ESP_L(_50560_LDC_1)) && (pulse_tab[1] < VAL_ESP_R(_50560_LDC_1));
	if (!match)
		return false;
	
	/* check code */
	for (i = 2; i < (tab_len - 1); i = i + 2) {	
		if (i == 18) { 
			/* skip segment code */
			i = i + 2;
		}
		match = (pulse_tab[i] > VAL_ESP_L(_50560_COM)) && (pulse_tab[i] < VAL_ESP_R(_50560_COM));
		if (!match)
			return false;
 
		if ((pulse_tab[i+1] > VAL_ESP_L(_50560_LOGIC_0)) && (pulse_tab[i+1] < VAL_ESP_R(_50560_LOGIC_0)))
			data = (data << 1) | 0;
		else if ((pulse_tab[i+1] > VAL_ESP_L(_50560_LOGIC_1)) && (pulse_tab[i+1] < VAL_ESP_R(_50560_LOGIC_1)))
			data = (data << 1) | 1;
		else 
			return false;
	}
	
	//generate_50560_wave(pulse_tab, tab_len, data, _50560_CODE_NUM);
	
	printk("50560 code : 0x%x\n", data);
	return true;
}

#endif
