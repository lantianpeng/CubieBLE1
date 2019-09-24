/* llc_hci.h - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LLC_HCI_H
#define LLC_HCI_H

#ifdef __cplusplus
extern "C" {
#endif


enum {
	LLCC_RXCMD_VALID_IRQn		=   4,      /* LLCC_RXCMD Interrupt */
	LLCC_RXEVT_VALID_IRQn		=   5,      /* LLCC_RXEVT Interrupt */
	LLCC_TXCMD_EMPTY_IRQn		=   6,      /* LLCC_TXCMD Interrupt */
	LLCC_TXEVT_EMPTY_IRQn		=   7,      /* LLCC_TXEVT Interrupt */
	LLCC_TXDMAL_DONE_IRQn		=   8,      /* LLCC_TXDMAL_DONE Interrupt */
	LLCC_RXDMAL_DONE_IRQn		=   9,      /* LLCC_RXDMAL_DONE Interrupt */
	LLCC_TXDMAH_DONE_IRQn		=   10,      /* LLCC_TXDMAH_DONE Interrupt */
	LLCC_RXDMAH_DONE_IRQn		=   11,      /* LLCC_RXDMAH_DONE Interrupt */
	LLCC_CLK_ENABLE_REQ_IRQn	=   12,      /* LLCC_CLK_ENABLE_REQ Interrupt */
};

struct trim_efuse_t {
	u8_t valid;
	u8_t trim_offset;
	u8_t efuse_offset;
	u8_t size;
	void *addr;
};


/* Packet types */
#define HCI_CMD_TYPE                                 1       /*!< HCI command packet */
#define HCI_ACL_TYPE                                 2       /*!< HCI ACL data packet */
#define HCI_EVT_TYPE                                 4       /*!< HCI event packet */

typedef void (*llcc_rx_event_cback_t)(const u8_t *pEvt);


struct llc_hci_cback_t {
	u8_t *(*buf_alloc)(u16_t len, u8_t type);
	void (*buf_free)(u8_t *buf);
	bool (*read_done)(u8_t type, u8_t *p_data, u16_t len);
	void (*write_done)(u8_t type, u8_t *p_data, int32_t err, void *p_context);
};

/*! \brief    LLCC state */
enum {
	LLCC_STATE_IDLE = 0,                /*!< Awaiting reset. */
	LLCC_STATE_WAIT,                    /*!< Waiting for an ack from a command. */
	LLCC_STATE_READY,                   /*!< Ready to transmit or receive. */
};

/*! \brief    Types of LLCC commands. */
enum {
	LLCC_CMD_SOFT_RESET     = 1,        /*!< Soft reset command. */
	LLCC_CMD_SOFT_RESET_ACK = 2,        /*!< Soft reset acknowledgement command. */
	LLCC_CMD_HCI_XFER_REQ   = 3,        /*!< HCI transfer request command. */
	LLCC_CMD_ERROR          = 4,        /*!< Error command. */

	LLCC_CMD2_HCI_XFER_REQ   = 11,      /*!< New transfer request command */
	LLCC_CMD2_HCI_XFER_ACK   = 12,      /*!< New transfer request ack */
	LLCC_CMD2_HCI_XFER_DONE  = 13,      /*!< New transfer request complete */

	LLCC_CMD2_ERROR          = 14,      /*!< New error command */
};

/*! \brief    LLCC errors. */
enum {
	LLCC_SUCCESS          = 0,          /*!< Receive succeeded. */
	LLCC_ERROR_ALLOC_FAIL = 1,          /*!< Allocation failed for receive. */
	LLCC_ERROR_BAD_XFER   = 2,          /*!< Transfer request is bad. */
	LLCC_ERROR_XFER_FAIL  = 3           /*!< Transfer failed. */
};

/*! \brief    Xfer flags. */
enum {
	LLCC_XFER_FLAG_NONE = 0,            /*!< No flag. */
	LLCC_XFER_FLAG_ACK  = (1 << 0)      /*!< Request acknowledgment. */
};

enum {
	LLCC_OK = 0,
	LLCC_ERROR2_HDR_ERR   = 1,          /*!< Header CRC failure */
	LLCC_ERROR2_BAD_XFER  = 2,          /*!< Transfer request is bad (illegal length) */
	LLCC_ERROR2_DATA_ERR  = 3,          /*!< Data checksum error */
	LLCC_ERROR2_ALLOC_FAIL = 4,         /*!< Allocation failed for receive. */
	LLCC_ERROR2_TIMEOUT = 5,            /*!< Timeout waiting for response. */
	LLCC_ERROR2_TRFAIL = 6,             /*!< Transfer was dropped due to repeated failures. */
	LLCC_ERROR2_BADSEQ = 7              /*!< Bad sequence number received. */
};

/*! \brief    LLCC command. */
typedef union {
	/*!< Data. */
	struct	{
		u32_t              data[2];            /*!< Command data. */
	} data;

	/*!< Basic command. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               pad[7];             /*!< Padding. */
	} basic;

	/*!< Error command. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               error[7];           /*!< Error parameters. */
	} error;

	/*!< HCI transfer request. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               hci_type;            /*!< HCI transfer type. */
		u16_t              len;                /*!< Transfer length, in words. */
		u8_t               align;              /*!< Transfer alignment. */
		u8_t               checksum;           /*!< Checksum. */
		u8_t               flag;               /*!< Transfer flag. */
		u8_t               pad[1];             /*!< Padding. */
	} hci_xfer_req;

	/*!< Basic command. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               pad[6];             /*!< Padding. */
		u8_t               csum;
	} basic2;

	/*!< Error2 command. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               error[6];           /*!< Error parameters. */
		u8_t               csum;               /*!< Message Checksum. */
	} error2;

	/*!< HCI transfer request2. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               hci_type;            /*!< HCI transfer type. */
		u16_t              len;                /*!< Transfer length, in words. */
		u8_t               align;              /*!< Transfer alignment. */
		u8_t               checksum;           /*!< Data Checksum. */
		u8_t               seq;                /*!< Transfer flag. */
		u8_t               csum;               /*!< Message Checksum. */
	} hci_xfer_req2;

	/*!< ACK2/DONE2 response. */
	struct	{
		u8_t               type;               /*!< Command type. */
		u8_t               seq;                /*!< Sequence. */
		u8_t               pad[5];             /*!< Padding. */
		u8_t               csum;               /*!< Message Checksum. */
	} hci_xfer_ack2;
} llcc_cmd_t;

/*! \brief      Transfer data. */
struct llcc_xfer_data_t {
	u8_t                 *p_buf;              /*!< Pointer to buffer. */
	u8_t                 align;              /*!< Alignment of buffer. */
	u8_t                 checksum;           /*!< Payload checksum. */
	u16_t                len;                /*!< Buffer length, in words. */
	u16_t                type;               /*!< Packet type. */
	u8_t                 flag;               /*!< Transfer flag. */
	u8_t                 seq_num;             /*!< Sequence Number */
	u32_t                timeout;            /*!< Watchdog timeout */
};

/*! \brief      Driver control block. */
struct llc_hci_ctrl_blk_t {
	struct {
		volatile bool       rx_busy_flag;         /*!< Receive busy flag. */
		volatile bool       tx_busy_flag;         /*!< Transmit busy flag. */
		volatile u8_t      state;              /*!< Driver state. */
		bool                use_xfer2;           /*!< Use new xfer protocol */
		volatile bool       wdog_rx_tx;
		u8_t               last_rx_seqnum;       /*!< Seqnum of previous transfer */
		u8_t               consecutive_err;     /*!< Number of back-to-back errors */
		u8_t               retry_limit;         /*!< Number of times to retry */
		u32_t              wdog_timeout_ticks;   /*!< Timeout in native ticks */
		u32_t              alloc_fail_backoff_ticks;   /*!< Timeout in native ticks */
		u32_t              last_llcc_cmd[2];     /*!< Last sent cmd. */
		struct llcc_xfer_data_t        tx;                 /*!< Transmit data. */
		struct llcc_xfer_data_t        rx;                 /*!< Receive data. */
		struct llcc_xfer_data_t        next_rx;             /*!< Next receive data. */

		void                  *p_tx_context;        /*/  Transmit Context */
		llcc_rx_event_cback_t    rx_event_cback;       /*!< Received event callback. */
	} llcc;

	struct {
		bool                hci_up;              /*!< Indicates whether HCI is up or down. */
		const struct llc_hci_cback_t *p_cbacks;            /*!< Pointer to application callbacks. */
		u8_t               *p_rx_buf;            /*!< Pointer to receive buffer. */
	} hci;
};

#define LLCC_CMD_SOFT_RESET_XFER2_FLAG0        0x5a
#define LLCC_CMD_SOFT_RESET_XFER2_FLAG1        0xa5
#define LLCC_CMD_SOFT_RESET_CRC_FLAG2          0xbe

#define LLCC_WDOG_TIMEOUT_MS_DFLT              50
#define LLCC_WDOG_ALLOCFAIL_BACKOFF_MS_DFLT    10
#define LLCC_MAX_CONSECUTIVE_ERR               250

#define LLCC_MAX_XFER_WORDS                    (256/4 + 4) /* 256 + some overhead */

/*! \brief      Packet statistics. */
struct hci_pkt_stats_t {
	u16_t        num_data_out;                 /*!< Number of data packets written. */
	u16_t        num_data_in;                  /*!< Number of data packets read. */
	u16_t        num_cmd_in;                   /*!< Number of command packets read. */
	u16_t        num_cmd_out;                  /*!< Number of command packets read. */
	u16_t        num_evt_in;                   /*!< Number of event packets written. */
	u16_t        num_evt_out;                  /*!< Number of event packets written. */
	u16_t        num_xfer_fail_err;             /*!< Number of failed transfer errors processed. */
	u16_t        num_hdr_crc_err_tx;             /*!< Number of header crc errors notified about */
	u16_t        num_hdr_crc_err_rx;             /*!< Number of header crc errors received */
	u16_t        num_data_crc_err_tx;            /*!< Number of data crc errors notified about */
	u16_t        num_data_crc_err_rx;            /*!< Number of data crc errors received */
	u16_t        num_rx_timeout_err;            /*!< Number of Rx Timeouts occurred */
	u16_t        num_tx_timeout_err;            /*!< Number of Tx Timeouts occurred */
	u16_t        num_rx_timeout_err_tx;          /*!< Number of Rx timeouts notifed about */
	u16_t        num_tx_abort_err;              /*!< Number of Tx Aborts occurred (unable to complete tx operation) */
	u16_t        num_tx_abort_err_rx;            /*!< Number of Tx Aborts notified */
	u16_t        num_bad_xfer_err_rx;            /*!< Number of bad xfer requests received. */
	u16_t        num_bad_xfer_err_tx;            /*!< Number of bad xfer requests notified about */
	u16_t        num_alloc_fail_err_rx;          /*!< Number of allocation failures occurred */
	u16_t        num_alloc_fail_err_tx;          /*!< Number of allocation failures notified about */
	u16_t        num_spurious_tx_done;          /*!< Number of spurious tx done messages received */
	u16_t        num_dup_rx_seq;                /*!< Number of duplicate sequnce numbers received */
	u16_t        num_bad_seq;                  /*!< Number of bad sequnce numbers detected */
	u16_t        num_bad_seq_rx;                /*!< Number of bad sequnce numbers notified */
	u16_t        num_tx_buffer_err;             /*!< Number of corruption tx buffers detected */
};

extern u8_t wdg_use_systick;

extern u8_t llcc_tx_power_level;

extern struct trim_efuse_t *ic_trim_efuse;
extern u8_t ic_trim_count;

extern struct trim_efuse_t *board_trim_efuse;
extern u8_t board_trim_count;

extern struct trim_efuse_t *final_board_trim_efuse;
extern u8_t final_board_trim_count;

extern struct trim_efuse_t *tx_trim_efuse;
extern u8_t tx_trim_count;

extern struct trim_efuse_t *tx_trim_aband_efuse;
extern u8_t tx_trim_aband_count;

extern struct k_timer *p_tx_timer, *p_rx_timer;

extern void (*p_llcc_write)(u32_t cmd, const u8_t *p_data, u16_t len, void *p_context);

extern bool (*p_llc_init_all_trim_upload)(void);

extern struct llc_hci_ctrl_blk_t llc_hci_cb;
extern struct hci_pkt_stats_t hci_stats;

//void llcc_rxcmd_valid_handler(void);
void llcc_rxevt_valid_handler(void);
void llcc_txdmal_done_handler(void);
void llcc_rxdmal_done_handler(void);
void llcc_clk_enable_req_handler(void);

void llcc_rxcmd_valid_handler_new(void);
void llcc_rxdmal_done_handler_new(void);

bool llc_init_trim_upload(u8_t type, u16_t len, u8_t *buf);
void llcc_write_new(u32_t cmd, const u8_t *p_data, u16_t len, void *p_context);

bool llcc_upload_ic_trim(void);
bool llcc_upload_board_trim(void);
bool llcc_upload_tx_trim(void);
void llcc_compose_trim(uint8_t *buf, struct trim_efuse_t *p_trim_efuse, uint8_t count);

void platform_wake_lock_inc(void);
void platform_wake_lock_dec(void);

void llc_hci_wr_cback(u8_t type, u8_t *p_data, void *p_context, int32_t error);
void *llc_hci_alloc_cback(u16_t len, u8_t type);
void llc_hci_free_cback(void *ptr);
bool llc_hci_legacy_rd_cback(u8_t type, u8_t *p_data, u8_t align, u16_t len);
void llcc_tx_cmd(const llcc_cmd_t *pCmd);
void llcc_tx_cmd_crc(const void *pCmd);
void llcc_tx_event(const void *pEvent);

void llcc_tx_xfer_req_cmd(u8_t type, u8_t align, u16_t len);
void llcc_tx_xfer_req2_cmd(u8_t type, u8_t align, u16_t len, u8_t checksum, u8_t seq);
void llcc_tx_error_cmd(u8_t code0, u8_t code1);
void llcc_tx_error2_cmd(u8_t code0, u8_t code1);
void llcc_hci_xfer_ack2_cmd(u8_t seq_num);
void llcc_hci_xfer_done2_cmd(u8_t seq_num);
bool llcc_dma_setup_rx(u8_t type, u8_t align, u32_t len, u8_t flag);
void llcc_dma_setup_tx(const u8_t *p_buf, u32_t len);
void hci_tx_done(void);
void hci_tx_failure(void);
void llcc_tx_retry_helper(bool consecutive_counts);
void llcc_tx_soft_reset_ack_cmd(void);

void llcc_wdog_rx_ena(u32_t ticks);
void llcc_wdog_tx_dis(void);
void llcc_wdog_rx_dis(void);
void llcc_wdog_tx_ena(u32_t ticks);
void llcc_init(void);

#ifdef __cplusplus
};
#endif

#endif /* LLC_HCI_H */
