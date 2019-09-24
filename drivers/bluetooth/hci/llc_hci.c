/* llc_hci.c - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <device.h>

#include "llc_llcc_defs.h"
#include "llc_hci.h"

#include "soc.h"

#define ENABLE_XFER2
#define CONFIG_LLCC_WA_FIX 1

#define SYS_LOG_DOMAIN "llc_hci"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>


/*! \brief      CRC-8 implementation. */
__ramfunc static const u8_t crc8_table[256] = {
	0x00, 0x1d, 0x3a, 0x27, 0x74, 0x69, 0x4e, 0x53, 0xe8, 0xf5, 0xd2, 0xcf, 0x9c, 0x81, 0xa6, 0xbb,
	0xcd, 0xd0, 0xf7, 0xea, 0xb9, 0xa4, 0x83, 0x9e, 0x25, 0x38, 0x1f, 0x02, 0x51, 0x4c, 0x6b, 0x76,
	0x87, 0x9a, 0xbd, 0xa0, 0xf3, 0xee, 0xc9, 0xd4, 0x6f, 0x72, 0x55, 0x48, 0x1b, 0x06, 0x21, 0x3c,
	0x4a, 0x57, 0x70, 0x6d, 0x3e, 0x23, 0x04, 0x19, 0xa2, 0xbf, 0x98, 0x85, 0xd6, 0xcb, 0xec, 0xf1,
	0x13, 0x0e, 0x29, 0x34, 0x67, 0x7a, 0x5d, 0x40, 0xfb, 0xe6, 0xc1, 0xdc, 0x8f, 0x92, 0xb5, 0xa8,
	0xde, 0xc3, 0xe4, 0xf9, 0xaa, 0xb7, 0x90, 0x8d, 0x36, 0x2b, 0x0c, 0x11, 0x42, 0x5f, 0x78, 0x65,
	0x94, 0x89, 0xae, 0xb3, 0xe0, 0xfd, 0xda, 0xc7, 0x7c, 0x61, 0x46, 0x5b, 0x08, 0x15, 0x32, 0x2f,
	0x59, 0x44, 0x63, 0x7e, 0x2d, 0x30, 0x17, 0x0a, 0xb1, 0xac, 0x8b, 0x96, 0xc5, 0xd8, 0xff, 0xe2,
	0x26, 0x3b, 0x1c, 0x01, 0x52, 0x4f, 0x68, 0x75, 0xce, 0xd3, 0xf4, 0xe9, 0xba, 0xa7, 0x80, 0x9d,
	0xeb, 0xf6, 0xd1, 0xcc, 0x9f, 0x82, 0xa5, 0xb8, 0x03, 0x1e, 0x39, 0x24, 0x77, 0x6a, 0x4d, 0x50,
	0xa1, 0xbc, 0x9b, 0x86, 0xd5, 0xc8, 0xef, 0xf2, 0x49, 0x54, 0x73, 0x6e, 0x3d, 0x20, 0x07, 0x1a,
	0x6c, 0x71, 0x56, 0x4b, 0x18, 0x05, 0x22, 0x3f, 0x84, 0x99, 0xbe, 0xa3, 0xf0, 0xed, 0xca, 0xd7,
	0x35, 0x28, 0x0f, 0x12, 0x41, 0x5c, 0x7b, 0x66, 0xdd, 0xc0, 0xe7, 0xfa, 0xa9, 0xb4, 0x93, 0x8e,
	0xf8, 0xe5, 0xc2, 0xdf, 0x8c, 0x91, 0xb6, 0xab, 0x10, 0x0d, 0x2a, 0x37, 0x64, 0x79, 0x5e, 0x43,
	0xb2, 0xaf, 0x88, 0x95, 0xc6, 0xdb, 0xfc, 0xe1, 0x5a, 0x47, 0x60, 0x7d, 0x2e, 0x33, 0x14, 0x09,
	0x7f, 0x62, 0x45, 0x58, 0x0b, 0x16, 0x31, 0x2c, 0x97, 0x8a, 0xad, 0xb0, 0xe3, 0xfe, 0xd9, 0xc4,
};

__ramfunc static u8_t crc8_compute(const u8_t *data, u32_t len)
{
	u8_t crc = 0;

	while (len--)	{
		/* XOR-in next input byte */
		/* get current CRC value = remainder */
		crc = crc8_table[*data++ ^ crc];
	}
	return crc;
}

__ramfunc u8_t compute_checksum(const u8_t *ptr, u32_t len)
{
	return crc8_compute(ptr, len);
}

#define WSF_CS_INIT(cs) u32_t cs
#define WSF_CS_ENTER(cs)        (cs = irq_lock())
#define WSF_CS_EXIT(cs)         irq_unlock(cs)

/*************************************************************************************************/
/*!
 *  \brief  Transmit a soft reset acknowledgment command over LLCC.
 *
 *  \return None.
 */
/*************************************************************************************************/
void llcc_tx_soft_reset_ack_cmd_new(void)
{
	llcc_cmd_t cmd;

	cmd.basic.type = LLCC_CMD_SOFT_RESET_ACK;
#ifdef ENABLE_XFER2
	cmd.basic.pad[0] = LLCC_CMD_SOFT_RESET_XFER2_FLAG0;
	cmd.basic.pad[1] = LLCC_CMD_SOFT_RESET_XFER2_FLAG1;
	cmd.basic.pad[2] = LLCC_CMD_SOFT_RESET_CRC_FLAG2;
#endif
	llcc_tx_cmd_crc((u8_t *)&cmd);
}

/*************************************************************************************************/
/*!
 *  \brief  Transmit a transfer request command over LLCC.
 *
 *  \param  type      Type of packet.
 * \param   p_data     Packet data.
 * \param   len       Packet length, in bytes
 * \param   p_context  Context pointer that will be returned in the `write_done()` callback
 *
 * \return  None.
 */
/*************************************************************************************************/
extern void (*p_llcc_write)(u32_t cmd, const u8_t *p_data, u16_t len, void *p_context);
__ramfunc void llcc_write_new(u32_t cmd, const u8_t *p_data, u16_t len, void *p_context)
{
	u8_t align = (u32_t)p_data & 3;
	u8_t checksum = 0;

	WSF_CS_INIT(cs);

	/* Wait for channel to become free */
	WSF_CS_ENTER(cs);
	while ((llc_hci_cb.llcc.state != LLCC_STATE_READY) || llc_hci_cb.llcc.tx_busy_flag) {
		WSF_CS_EXIT(cs);
		/*__WFI();  // or just yield to the OS.*/
		WSF_CS_ENTER(cs);
	}
	if (!llc_hci_cb.llcc.tx_busy_flag) {
		platform_wake_lock_inc();
		llc_hci_cb.llcc.tx_busy_flag = true;
	}
	WSF_CS_EXIT(cs);

	/* Counters */
	switch (cmd) {
	case HCI_ACL_TYPE:
		hci_stats.num_data_out++;
		break;
	case HCI_EVT_TYPE:
		hci_stats.num_evt_out++;
		break;
	case HCI_CMD_TYPE:
		hci_stats.num_cmd_out++;
		break;
	default:
		break;
	}

	/* Store pointer */
	llc_hci_cb.llcc.tx.p_buf = (u8_t *)p_data;

	/* Calculate checksum, only for new transfers. */
	if (llc_hci_cb.llcc.use_xfer2)
		checksum = compute_checksum(p_data, len);

	/* Round up to next multiple of 32 bits */
	u16_t tlen = (len + 3) / 4;

	p_data -= align;

	llc_hci_cb.llcc.tx.type    = cmd;
	llc_hci_cb.llcc.tx.align   = align & 0x3;
	llc_hci_cb.llcc.tx.len     = tlen;
	llc_hci_cb.llcc.tx.checksum = checksum;
	llc_hci_cb.llcc.p_tx_context = p_context;

	if (llc_hci_cb.llcc.use_xfer2) {
		/* Allow true length to be recovered by storing the leftover bits too */
		llc_hci_cb.llcc.tx.align |= ((tlen * 4) - len - align) << 2;

		llc_hci_cb.llcc.tx.seq_num++;

		llcc_wdog_tx_ena(llc_hci_cb.llcc.wdog_timeout_ticks);
		llcc_tx_xfer_req2_cmd(llc_hci_cb.llcc.tx.type, llc_hci_cb.llcc.tx.align,
											llc_hci_cb.llcc.tx.len, llc_hci_cb.llcc.tx.checksum,
											llc_hci_cb.llcc.tx.seq_num);
	} else {
		llcc_tx_xfer_req_cmd(cmd, llc_hci_cb.llcc.tx.align, llc_hci_cb.llcc.tx.len);
		llcc_dma_setup_tx(p_data, llc_hci_cb.llcc.tx.len);
	}
}
/*************************************************************************************************/
/*!
 *  \brief  Handle LLCC received command valid interrupt.
 *
 *  \return None.
 */
/*************************************************************************************************/
#define PATCH_RX_CMD_VALID
#ifdef  PATCH_RX_CMD_VALID
__ramfunc void llcc_rx_cmd_valid_handler_new(void)
{
	llcc_cmd_t cmd;

	/* Read command. */
	cmd.data.data[0] = LLCC_RXD->CMD_DATA0;
	cmd.data.data[1] = LLCC_RXD->CMD_DATA1;

	/* Validate message checksum */
	if (llc_hci_cb.llcc.use_xfer2) {
		/* Validate header checksum */
		int i;
		u8_t *pCmd8 = (u8_t *) &cmd;
		u8_t csum = ~0;

		for (i = 0 ; i < 7 ; i++)
			csum += pCmd8[i];

		/* Kick back an error if haeder CRC failes */
		if (csum != pCmd8[7]) {
			hci_stats.num_hdr_crc_err_rx++;
			llcc_tx_error2_cmd(LLCC_ERROR2_HDR_ERR, 0);
			return;
		}
	}

	switch (cmd.basic.type) {
	case LLCC_CMD_SOFT_RESET:
	{
		llcc_init();
		llc_hci_cb.llcc.state = LLCC_STATE_READY;
		llc_hci_cb.llcc.tx.seq_num = 0;
#ifdef ENABLE_XFER2
		llc_hci_cb.llcc.use_xfer2 = (cmd.basic.pad[0] == LLCC_CMD_SOFT_RESET_XFER2_FLAG0 &&
															cmd.basic.pad[1] == LLCC_CMD_SOFT_RESET_XFER2_FLAG1);
#endif
		llcc_tx_soft_reset_ack_cmd_new();
		return;
	}

	case LLCC_CMD2_ERROR:
	{
		switch (cmd.error2.error[0]) {
		case LLCC_ERROR2_DATA_ERR: /* Payload checksum failure */
		{
			/* First make sure we're in a transfer state */
			if (!llc_hci_cb.llcc.tx_busy_flag)
				return;

			/* Re-validate payload checksum */
			u8_t csum;
			u8_t *ptr = llc_hci_cb.llcc.tx.p_buf + (llc_hci_cb.llcc.tx.align & 0x3);
			u16_t len = (llc_hci_cb.llcc.tx.len * 4) - (llc_hci_cb.llcc.tx.align & 0x3) - ((llc_hci_cb.llcc.tx.align >> 2) & 0x3);

			csum = compute_checksum(ptr, len);

			SYS_LOG_DBG(" csum %x/%x", csum, llc_hci_cb.llcc.tx.checksum);

			if (csum != llc_hci_cb.llcc.tx.checksum) {
				/* Error out, data is corrupted, don't retry */
				hci_stats.num_tx_buffer_err++;
				llc_hci_cb.llcc.consecutive_err = LLCC_MAX_CONSECUTIVE_ERR;
				llcc_tx_retry_helper(true);
				return;
			}

			hci_stats.num_data_crc_err_tx++;
			llcc_tx_retry_helper(true);
			return;
		}
		default:  /* Ignore all others */
			break;
		}
	}

	default:
		break;
	}
void llcc_rxcmd_valid_handler(u32_t , u32_t);
	return llcc_rxcmd_valid_handler(cmd.data.data[0], cmd.data.data[1]);
}

CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p1, 0x46c04cb6, 0xea2c);
CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p2, 0x46c09000, 0xea30);
CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p3, 0x78e39101, 0xea34);

#else
FUNCTION_PATCH_REGISTER(llcc_tx_soft_reset_ack_cmd, llcc_tx_soft_reset_ack_cmd_new, llcc_tx_soft_reset_ack_cmd);
extern u8_t p_llcc_rx_cmd_valid(void);
/*
        ecd0:    b403        ..      PUSH     {r0,r1}
        ecd2:    4806        .H      LDR      r0,[pc,#24] ; [0xecec] = 20001001
        ecd4:    9001        ..      STR      r0,[sp,#4]
        ecd6:    bd01        ..      POP      {r0,pc}
        ...
				ecec:    0x20001001 (p_llcc_rx_cmd_valid)

*/
CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p1, 0x4806b403, 0xecd0);
CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p2, 0xbd019001, 0xecd4);
CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p3, p_llcc_rx_cmd_valid, 0xecec);

//CODE_PATCH_REGISTER(llcc_rx_cmd_valid_p3, 0x4b0c0011, 0xecd8);
#endif

/*************************************************************************************************/
/*!
 *  \brief  Handle LLCC receive DMA done interrupt.
 *
 *  \return None.
 */
/*************************************************************************************************/
extern u8_t p_llcc_rx_dmal_done(void);
/*
        ede4:    b403        ..      PUSH     {r0,r1}
        ede6:    480d        .H      LDR      r0,[pc,#52] ; [0xecec] = 20001001
        ede8:    9001        ..      STR      r0,[sp,#4]
        edea:    bd01        ..      POP      {r0,pc}
				edec:	   46c0      	         nop
				edee:	4b44      	ldr	r3, [pc, #272]	; (ef00 <llccRxDmalDoneHandler+0x16c>)

				ee1c:    0x20001001 (p_llcc_rx_dmal_done)

*/
CODE_PATCH_REGISTER(llcc_rx_dmal_done_p1, 0x480db403, 0xede4);
CODE_PATCH_REGISTER(llcc_rx_dmal_done_p2, 0xbd019001, 0xede8);
CODE_PATCH_REGISTER(llcc_rx_dmal_done_p3, p_llcc_rx_dmal_done, 0xee1c);

__ramfunc void llcc_rxcmd_valid_handler_new(void)
{
#ifdef  PATCH_RX_CMD_VALID
	llcc_rx_cmd_valid_handler_new();
#else
	llcc_rxcmd_valid_handler();
#endif
}

__ramfunc void llcc_rxdmal_done_handler_new(void)
{
	llcc_rxdmal_done_handler();
}

bool hci_vs_read_done(u8_t type, u8_t *p_data, u16_t len);
__ramfunc bool hci_vs_read_done_new(u8_t type, u8_t *p_data, u16_t len)
{
	if (type == HCI_EVT_TYPE) {
		u8_t *evt_data = p_data;

		if ((evt_data[0] == 0xff) /* HCI_VENDOR_SPEC_EVT */
			&& (evt_data[1] == 0x3e) /* len */
			&& (evt_data[2] == 0x80) && (evt_data[3] == 0xfe) /* BOOT_EVT_STARTUP */) {

			/* Filter the first reset evt regardless of the reason of reset */
			if (evt_data[7] == 0x1) /* POWER_ON_RESET */ {
				printk("reboot for power_on evt\n");
				sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
			}
		}
	}
	
	return hci_vs_read_done(type, p_data, len);
}

CODE_PATCH_REGISTER(hci_vs_read_done, hci_vs_read_done_new, 0x26bf4);

#if CONFIG_LLCC_WA_FIX
#include "soc.h"
#include "soc_patch.h"
#include "soc_pm.h"
void hci_tx_failure_new(void)
{
	/* reboot for failure */
	printk("reboot for llcc retry failed\n");
	sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
}

FUNCTION_PATCH_REGISTER(hci_tx_failure, hci_tx_failure_new, hci_tx_failure);

#endif
