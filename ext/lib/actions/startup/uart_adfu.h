/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_UART_ADFU_H_
#define	_UART_ADFU_H_

#ifndef _ASMLANGUAGE

extern void check_uart_adfu(void);

typedef enum {
	UART_PROTOCOL_TYPE_FUNDAMENTAL  = 0,
	UART_PROTOCOL_TYPE_STUB         = 1,
	UART_PROTOCOL_TYPE_ADFU         = 8,
} UART_PROTOCOL_TYPE_E;

typedef enum __UART_PROTOCOL_STATUS {
	UART_PROTOCOL_OK =  0,              ///< Operation succeeded
	UART_PROTOCOL_BUSY,

	UART_PROTOCOL_ERR = 10,             ///< Unspecified error
	UART_PROTOCOL_TX_ERR,               ///< uart protocol send data error happend
	UART_PROTOCOL_RX_ERR,               ///< uart protocol receive data error happend
	UART_PROTOCOL_DATA_CHECK_FAIL,
	UART_PROTOCOL_PAYLOAD_TOOLARGE,     ///< packet payload length too large to transfer

	UART_PROTOCOL_TIMEOUT = 20,         ///< uart protocol transaction timeout
	UART_PROTOCOL_DISCONNECT,           ///< disconnect UART

	UART_PROTOCOL_NOT_EXPECT_PROTOCOL = 30, ///< NOT expected uart protocol
	UART_PROTOCOL_NOT_SUPPORT_PROTOCOL, ///< NOT support uart protocol
	UART_PROTOCOL_NOT_SUPPORT_BAUDRATE, ///< NOT support uart baudrate
	UART_PROTOCOL_NOT_SUPPORT_CMD,      ///< Not support uart command
} UART_PROTOCOL_STATUS;

typedef enum __ADFU_STATUS {
	ADFU_OK =  0,               ///< Operation succeeded
	ADFU_ERROR,                 ///< Unspecified error
	ADFU_UPLOAD_ERROR,          ///< ADFU upload data error happend
	ADFU_DOWNLOAD_ERROR,        ///< ADFU download data error happend
	ADFU_TIMEOUT,               ///< ADFU transaction timeout
	ADFU_COMMAND_NOT_SUPPORT,   ///< Not support ADFU command
	ADFU_END,                   ///< ADFU transaction finish
} ADFU_STATUS;

char uart_connect(void);
UART_PROTOCOL_STATUS uart_open(unsigned int protocol_type);
ADFU_STATUS uart_adfu_server(void);
int uart_adfu_close(void);

#endif /* _ASMLANGUAGE */

#endif /* _UART_ADFU_H_	*/
