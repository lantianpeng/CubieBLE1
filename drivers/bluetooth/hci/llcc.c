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

#define IC_TRIM_DATA_LEN 32
#define BOARD_TRIM_DATA_LEN 32
#define TRANSMIT_TRIM_DATA_LEN 136

enum {
	BLE_TRIM_TYPE_IC    = 0x0A,    /*!< IC-level trim and calibration data. */
	BLE_TRIM_TYPE_BOARD = 0x0B,    /*!< Board-level trim, calibration, and configuration data. */
	BLE_TRIM_TYPE_TX_1V = 0x14,    /*!< Transmit trim (1v mode). */
	BLE_TRIM_TYPE_TX_3V = 0x15     /*!< Transmit trim (3v mode). */
};

/* enable compose trim from efuse */
#define CONFIG_TRIM_FROM_EFUSE 1

/* update vdda trim after composing trim from efuse*/
#define CONFIG_IC_TRIM_UPDATE_VDDA 1

/* update batt_comp trim after composing trim from efuse
 * #define CONFIG_BOARD_TRIM_UPDATE_BATT_COMP 1
 */

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

#ifdef CONFIG_IC_TRIM_UPDATE_VDDA
__init_once_text bool llcc_upload_ic_trim_new(void)
{
	bool ret = true;
	u8_t ic_trim[IC_TRIM_DATA_LEN];

	/* compose ic trim from efuse & external */
	llcc_compose_trim(ic_trim, ic_trim_efuse, ic_trim_count);

	/* update vdda_trim */
	ic_trim[28] -= 0x5<<2;

	ret = llc_init_trim_upload(BLE_TRIM_TYPE_IC, IC_TRIM_DATA_LEN, ic_trim);

	return ret;
}
#endif

#ifdef CONFIG_BOARD_TRIM_UPDATE_BATT_COMP
__init_once_text bool llcc_upload_board_trim_new(void)
{
	bool ret = true;
	u8_t board_trim[BOARD_TRIM_DATA_LEN];

	/* compose board trim from efuse & external */
	llcc_compose_trim(board_trim, board_trim_efuse, board_trim_count);

	/* update batt_comp */
	board_trim[10] = 0x40;
	board_trim[11] = 0x01;
	ret = llc_init_trim_upload(BLE_TRIM_TYPE_BOARD, BOARD_TRIM_DATA_LEN, board_trim);

	return ret;
}
#endif

__init_once_text bool llc_init_all_trim_upload_new(void)
{
	bool ret = true;

	/* set vd12 before efuse read */
	acts_set_vd12_before_efuse_read();

#ifdef CONFIG_IC_TRIM_UPDATE_VDDA
	ret = llcc_upload_ic_trim_new();
#else
	ret = llcc_upload_ic_trim();
#endif
	if (ret == false)
		goto out;

#ifdef CONFIG_BOARD_TRIM_UPDATE_BATT_COMP
	ret = llcc_upload_board_trim_new();
#else
	ret = llcc_upload_board_trim();
#endif
	if (ret == false)
		goto out;

	ret = llcc_upload_tx_trim();

out:
	/* set vd12 after efuse read */
	acts_set_vd12_after_efuse_read();

	return ret;
}

#else

#include "ble_trim_value.h"

__init_once_text bool llc_init_all_trim_upload_new(void)
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

#include "hci_defs.h"
#include "bstream.h"
#include "ble_boot_defs.h"
#include "ble_boot_defs.h"
#include <misc/printk.h>
#include <string.h>

typedef struct {
  u8_t     type;
  u16_t    len;
  u8_t     *p_data;
} llc_init_msg_t;

extern void llc_init_tx_message(llc_init_msg_t *p_msg);
extern void llc_init_wait_for_rx_message(llc_init_msg_t *p_msg);
extern void llc_hci_buf_free(u8_t *buf);
extern bool (*p_llc_init_trim_upload)(u8_t type, u16_t len, u8_t *buf);

__init_once_text static bool llc_init_tx_command(llc_init_msg_t *p_msg, bool wait_complete, bool vs_command)
{
  llc_init_msg_t msg;
  u8_t   *p_data;
  u8_t   evt_code, status, ext_status;

  /* Transmit message. */
  llc_init_tx_message(p_msg);
  if (!wait_complete)
    return true;

  /* Wait for event response. */
  llc_init_wait_for_rx_message(&msg);
  if (msg.type != HCI_EVT_TYPE)
    goto fail;

  p_data = msg.p_data;
  BSTREAM_TO_UINT8(evt_code,  p_data);
  p_data += 4; /* paramLen, numHciCommandPackets, commandOpcode */
  if (evt_code != HCI_CMD_CMPL_EVT)
    goto fail;

  BSTREAM_TO_UINT8(status, p_data);
  if (vs_command) {
    BSTREAM_TO_UINT8(ext_status, p_data);
    if ((status != HCI_SUCCESS) || (ext_status != BLE_EXT_STATUS_SUCCESS))
      goto fail;
  } else {
    if (status != HCI_SUCCESS)
      goto fail;
  }

  llc_hci_buf_free(msg.p_data);
  return true;

fail:
  llc_hci_buf_free(msg.p_data);
  return false;
}

__init_once_text bool p_llc_init_trim_up_load_new(u8_t type, u16_t len, u8_t *buf)
{
  llc_init_msg_t msg;
  u8_t   *p_buf;
  u8_t   *p_cmd;
  u8_t   tmp_buf[256];

  printk("llcInitTrimUpload\n");

  /* Allocate buffer for trim upload. */
  p_buf = tmp_buf;
  msg.type  = HCI_CMD_TYPE;
  msg.p_data = p_buf;

  /* ---------- start trim load ---------- */
  p_cmd = p_buf;
  UINT16_TO_BSTREAM(p_cmd, BLE_BOOT_OPCODE_TRIM_LOAD);
  UINT8_TO_BSTREAM (p_cmd, BLE_BOOT_LEN_TRIM_LOAD);
  UINT16_TO_BSTREAM(p_cmd, type);
  UINT16_TO_BSTREAM(p_cmd, len);
  msg.len = HCI_CMD_HDR_LEN + BLE_BOOT_LEN_TRIM_LOAD;

  if (!llc_init_tx_command(&msg, true, true)) {
    printk("Tx TRIM_LOAD failed, type:0x%x\n", type);
    goto fail;
  }
  /* ---------- send trim data ---------- */
  p_cmd = p_buf;
  UINT16_TO_BSTREAM(p_cmd, BLE_BOOT_OPCODE_TRIM_DATA);
  UINT8_TO_BSTREAM (p_cmd, BLE_BOOT_LEN_TRIM_DATA(len));
  memcpy(p_cmd, buf, len);
  msg.len = HCI_CMD_HDR_LEN + BLE_BOOT_LEN_TRIM_DATA(len);
  if (!llc_init_tx_command(&msg, true, true)) {
    printk("Tx TRIM_DATA failed, type:0x%x\n", type);
    goto fail;
  }
  printk("Tx TRIM_DATA done\n");
  return true;
fail:
  return false;
}

int patch_llc_init_all_trim_upload(void)
{

#ifdef CONFIG_TRIM_FROM_EFUSE
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

#endif
	p_llc_init_all_trim_upload = llc_init_all_trim_upload_new;
	p_llc_init_trim_upload = p_llc_init_trim_up_load_new;
	p_llcc_write = llcc_write_new;
	return 0;
}
