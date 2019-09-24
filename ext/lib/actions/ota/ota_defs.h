/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef OTA_DEFS_H
#define OTA_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Constant Definitions
**************************************************************************************************/
	
/*! Device configuration characteristic message header length */
#define OTA_DC_HDR_LEN                 2

/*! Device configuration characteristic operations */
#define OTA_DC_OP_GET                  0x01         /*! Get a parameter value */
#define OTA_DC_OP_SET                  0x02         /*! Set a parameter value */
#define OTA_DC_OP_UPDATE               0x03         /*! Send an update of a parameter value */

/*! Device control characteristic parameter IDs */
#define OTA_DC_ID_CONN_UPDATE_REQ      0x01         /*! Connection Parameter Update Request */
#define OTA_DC_ID_CONN_PARAM           0x02         /*! Current Connection Parameters */
#define OTA_DC_ID_DISCONNECT_REQ       0x03         /*! Disconnect Request */
#define OTA_DC_ID_CONN_SEC_LEVEL       0x04         /*! Connection Security Level */
#define OTA_DC_ID_SECURITY_REQ         0x05         /*! Security Request */
#define OTA_DC_ID_SERVICE_CHANGED      0x06         /*! Service Changed */
#define OTA_DC_ID_DELETE_BONDS         0x07         /*! Delete Bonds */
#define OTA_DC_ID_ATT_MTU              0x08         /*! Current ATT MTU */

/*! Device control parameter lengths */
#define OTA_DC_LEN_SEC_LEVEL           1            /*! Security Level */
#define OTA_DC_LEN_ATT_MTU             2            /*! ATT MTU */
#define OTA_DC_LEN_CONN_PARAM_REQ      8            /*! Connection parameter request */
#define OTA_DC_LEN_CONN_PARAM          7            /*! Current connection parameters */


/*! File transfer control characteristic message header length */
#define OTA_FTC_HDR_LEN                1
#define OTA_FTC_HANDLE_LEN             2

/*! File transfer control characteristic operations */
#define OTA_FTC_OP_NONE                0x00        /*! No operation */
#define OTA_FTC_OP_GET_REQ             0x01        /*! Get a file from the server */
#define OTA_FTC_OP_GET_RSP             0x02        /*! File get response */
#define OTA_FTC_OP_PUT_REQ             0x03        /*! Put a file to the server */
#define OTA_FTC_OP_PUT_RSP             0x04        /*! File put response */
#define OTA_FTC_OP_ERASE_REQ           0x05        /*! Erase a file on the server */
#define OTA_FTC_OP_ERASE_RSP           0x06        /*! File erase response */
#define OTA_FTC_OP_VERIFY_REQ          0x07        /*! Verify a file (e.g. check its CRC) */
#define OTA_FTC_OP_VERIFY_RSP          0x08        /*! File verify response */
#define OTA_FTC_OP_ABORT               0x09        /*! Abort a get, put, or list operation in progress */
#define OTA_FTC_OP_EOF                 0x0a        /*! End of file reached */
#define OTA_FTC_OP_PACKET_RECEIVED     0x0b        /*! One packet received */
#define OTA_FTC_OP_SYSTEM_RESET        0x0c
#define OTA_FTC_OP_GET_VERSION_REQ     0x0d
#define OTA_FTC_OP_GET_VERSION_RSP     0x0e

/*! File transfer control permissions */
#define OTA_FTC_GET_PERMITTED          0x01        /*! File Get Permitted */
#define OTA_FTC_PUT_PERMITTED          0x02        /*! File Put Permitted */
#define OTA_FTC_ERASE_PERMITTED        0x04        /*! File Erase Permitted */
#define OTA_FTC_VERIFY_PERMITTED       0x08        /*! File Verify Permitted */

/*! File transfer control characteristic status */
#define OTA_FTC_ST_SUCCESS             0           /*! Success */
#define OTA_FTC_ST_INVALID_OP_FILE     1           /*! Invalid operation for this file */
#define OTA_FTC_ST_INVALID_HANDLE      2           /*! Invalid file handle */
#define OTA_FTC_ST_INVALID_OP_DATA     3           /*! Invalid operation data */
#define OTA_FTC_ST_IN_PROGRESS         4           /*! Operation in progress */
#define OTA_FTC_ST_VERIFICATION        5           /*! Verification failure */

/*! File transfer data characteristic message header length */
#define OTA_FTD_HDR_LEN                0

#ifdef __cplusplus
}
#endif

#endif /* OTA_DEFS_H */
