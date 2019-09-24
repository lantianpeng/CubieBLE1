/* hci.c - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "llc_hci.h"
#include "nvram_config.h"

struct k_timer tx_timer, rx_timer;

__init_once_text int patch_llc_init_all_trim_upload(void);

__init_once_text int LlcIrqConnect(struct device *unused)
{
	IRQ_CONNECT(LLCC_RXCMD_VALID_IRQn,
			1,
			llcc_rxcmd_valid_handler_new, NULL,
			0);
	IRQ_CONNECT(LLCC_RXEVT_VALID_IRQn,
			1,
			llcc_rxevt_valid_handler, NULL,
			0);
	IRQ_CONNECT(LLCC_TXDMAL_DONE_IRQn,
			1,
			llcc_txdmal_done_handler, NULL,
			0);
	IRQ_CONNECT(LLCC_RXDMAL_DONE_IRQn,
			1,
			llcc_rxdmal_done_handler_new, NULL,
			0);

	IRQ_CONNECT(LLCC_CLK_ENABLE_REQ_IRQn,
			0,
			llcc_clk_enable_req_handler, 0,
			0);

	wdg_use_systick = 1;
	p_tx_timer = &tx_timer;
	p_rx_timer = &rx_timer;
	
	patch_llc_init_all_trim_upload();

	return 0;
}

SYS_INIT(LlcIrqConnect, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

int _bt_llcc_init(struct device *dev);
SYS_INIT(_bt_llcc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#ifdef CONFIG_TRIM_FROM_EFUSE

__init_once_data struct trim_efuse_t outer_ic_trim_efuse[4] = {
	{1, 0, 9, 4, NULL},
	{1, 4, 0xff, 8, NULL},
	{1, 12, 13, 20, NULL},
	{1, 30, 0xff, 2, NULL}
};

#define EXT_BOARD_TRIM_DATA_LEN 8
__init_once_data u8_t ext_board_trim[EXT_BOARD_TRIM_DATA_LEN] = {
		0x16, 0x05, 0x07, 0x20, 0x0B, 0x00, 0x01, 0x00};

__init_once_data u16_t sleep_clk_accuracy = 20;
__init_once_data u8_t retry_timeout = 50;
__init_once_data struct trim_efuse_t outer_board_trim_efuse[4] = {
	{1, 0, 0xff, EXT_BOARD_TRIM_DATA_LEN, ext_board_trim},
	{1, 8, 31, 24, NULL},
	{1, 12, 0xff, 2, &sleep_clk_accuracy},
	{1, 31, 0xff, 1, &retry_timeout},
};

__init_once_data struct trim_efuse_t outer_final_board_trim_efuse[3] = {
	{0, 18, 111, 2, NULL},
	{0, 20, 113, 4, NULL},
	{0, 24, 117, 4, NULL},
};

#define EXT_TX_TRIM_DATA_LEN 40

__init_once_data u8_t ext_tx_trim[EXT_TX_TRIM_DATA_LEN];
__init_once_data u8_t runtime_tx[16] = {
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1, 0xEE, 0xEB};
__init_once_data struct trim_efuse_t outer_tx_trim_efuse[9] = {
	{1, 0, 0xff, 4, NULL}, /* 82/89/96 */
	{1, 4, 54, 8, NULL},
	{1, 12, 0xff, 16, runtime_tx},
	{1, 28, 78, 4, NULL},
	{1, 30, 0xff, 1, NULL},
	{1, 31, 0xff, EXT_TX_TRIM_DATA_LEN, ext_tx_trim},
	{1, 71, 0xff, 40, NULL},
	{1, 111, 80, 4, NULL},
	{1, 113, 0xff, 3, NULL},
};

__init_once_data struct trim_efuse_t outer_tx_trim_aband_efuse[6] = {
	{1, 116, 103, 1, NULL},
	{1, 120, 104, 1, NULL},
	{1, 124, 105, 1, NULL},
	{1, 128, 106, 1, NULL},
	{1, 132, 107, 1, NULL},
	{1, 117, 0xff, 3, NULL}, /* 86/93/100 */
};
#else

#define IC_TRIM_DATA_LEN 32
#define BOARD_TRIM_DATA_LEN 32
#define TRANSMIT_TRIM_DATA_LEN 136

enum {
	BLE_TRIM_TYPE_IC    = 0x0A,    /*!< IC-level trim and calibration data. */
	BLE_TRIM_TYPE_BOARD = 0x0B,    /*!< Board-level trim, calibration, and configuration data. */
	BLE_TRIM_TYPE_TX_1V = 0x14,    /*!< Transmit trim (1v mode). */
	BLE_TRIM_TYPE_TX_3V = 0x15     /*!< Transmit trim (3v mode). */
};

#include "ble_trim_value.h"

__init_once_text bool dbg_llc_init_all_trim_upload(void)
{
	bool ret = 0;

	debug_uart_put_char('i');
	debug_uart_put_char('c');
	ret = llc_init_trim_upload(BLE_TRIM_TYPE_IC, IC_TRIM_DATA_LEN, ic_trim);
	if (ret == 0)
		debug_uart_put_char('+');
	else
		debug_uart_put_char('-');

	debug_uart_put_char('b');
	debug_uart_put_char('d');
	ret = llc_init_trim_upload(BLE_TRIM_TYPE_BOARD, BOARD_TRIM_DATA_LEN, board_trim);
	if (ret == 0)
		debug_uart_put_char('+');
	else
		debug_uart_put_char('-');

	debug_uart_put_char('t');
	debug_uart_put_char('x');
	ret = llc_init_trim_upload(BLE_TRIM_TYPE_TX_3V, TRANSMIT_TRIM_DATA_LEN, tx_trim);
	if (ret == 0)
		debug_uart_put_char('+');
	else
		debug_uart_put_char('-');

	return true;
}
#endif
#ifdef CONFIG_TX_TRIM_MODE_3V0DB
int8_t tx_power_level[16] = {
	0x0F, 0x0B, 0x09, 0x07, 0x06, 0x05, 0x04, 0x03, 0, -3, -6, -9, -10, -15, -18, -21};
#else
int8_t tx_power_level[16] = {
	0x0F, 0x0B, 0x09, 0x07, 0x06, 0x05, 0x04, 0x03, 5, 2, -1, -4, -7, -10, -13, -16};
#endif
extern u8_t tx_power_level_index;
__init_once_text int patch_llc_init_all_trim_upload(void)
{
#ifdef CONFIG_TRIM_FROM_EFUSE
	int err;
	u8_t param_8;
	u8_t i;
	
	err = nvram_config_get("TX_POWER", &param_8, sizeof(u8_t));
	if (err >= 0) {
		tx_power_level_index = param_8;
		if (tx_power_level_index < 8) {
			for (i = 0; i < 8; i++) {
				runtime_tx[i] = tx_power_level[tx_power_level_index];
				runtime_tx[i+8] = tx_power_level[tx_power_level_index+8];
			}
		}
	}

	/* ic trim */
	ic_trim_efuse = outer_ic_trim_efuse;
	ic_trim_count = 4;

	/* board trim */
	board_trim_efuse = outer_board_trim_efuse;
	board_trim_count = 4;

	final_board_trim_efuse = outer_final_board_trim_efuse;
	final_board_trim_count = 3;

	/* tx trim */
	tx_trim_efuse = outer_tx_trim_efuse;
	tx_trim_count = 9;

	tx_trim_aband_efuse = outer_tx_trim_aband_efuse;
	tx_trim_aband_count = 6;

	/* tx trim mode */
#ifdef CONFIG_TX_TRIM_MODE_3V0DB
	llcc_tx_power_level = 0;
#endif

#else
	p_llc_init_all_trim_upload = dbg_llc_init_all_trim_upload;
#endif
	p_llcc_write = llcc_write_new;
	return 0;
}
