/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include "errno.h"
#include <zephyr.h>
#include <misc/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/storage.h>
#include <misc/byteorder.h>
#include <console/uart_pipe.h>
#include "act_cmd.h"
#include "system_app_ble.h"

#define STACKSIZE 2048
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread cmd_thread;

#define CMD_QUEUED 10
struct act_cmd_buf {
    u32_t _reserved;
    union {
        u8_t data[ACT_CMD_MTU];
        struct act_cmd_hdr hdr;
    };
};

static struct act_cmd_buf cmd_buf[CMD_QUEUED];

static K_FIFO_DEFINE(cmds_queue);
static K_FIFO_DEFINE(avail_queue);

extern void act_cmd_handle_gap(u8_t opcode, u8_t index, 
		u8_t *data, u16_t len);
extern void act_cmd_handle_gatt(u8_t opcode, u8_t index, 
		u8_t *data, u16_t len);
extern void act_cmd_handle_rmc(u8_t opcode, u8_t index, 
	u8_t *data, u16_t len);



void show_act_cmd_hdr(const struct act_cmd_hdr *hdr)
{
    printk("cmd_type=0x%x\n", hdr->cmd_type);
    printk("opcode=0x%x\n", hdr->opcode);
    printk("len=0x%04x\n", sys_le16_to_cpu(hdr->len));
}

void show_act_cmd_data(u8_t *cmd_data, int len)
{	
	if (len) {
		int i = 0;
		printk("cmd_data: ");
		for (i = 0; i < len; i++) {
			printk("%02x ", cmd_data[i]);
		}
		printk("\n");
	}
}

static void cmd_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		struct act_cmd_buf *cmd;
		u16_t len;

		cmd = k_fifo_get(&cmds_queue, K_FOREVER);

		len = sys_le16_to_cpu(cmd->hdr.len);

		/* TODO
		 * verify if service is registered before calling handler
		 */

		switch (cmd->hdr.opcode) {
		case CMD_WRITE_WORK_MODE:
			//status = writeWorkMode(pCmd,dataLen,TRUE);
			break;
		case CMD_READ_WORK_MODE:
			//paramLen = readWorkMode(&pParam);
			break;
		case CMD_WRITE_DEVICE_NAME:
			//status = writeDeviceName(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_DEVICE_NAME:
			//paramLen = readDeviceName(&pParam);
      break;
		case CMD_WRITE_TX_POWER:
			//status = writeTxPwr(pCmd,dataLen,TRUE);	
      break;
		case CMD_WRITE_DEF_TX_POWER:
			//status = writeDefTxPwr(pCmd,dataLen,TRUE);
      break;
		case CMD_WRITE_CONN_PARAM:
			//status = writeConnParam(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_CONN_PARAM:
			//paramLen = readConnParam(&pParam);
      break;
		case CMD_WRITE_MAX_CONN:
			//status = writeConnMax(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_MAX_CONN:
			//paramLen = readConnMax(&pParam);
      break;
		case CMD_WRITE_BD_ADDR:
			//status = writeBDAddr(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_BD_ADDR:
			//paramLen = readBDAddr(&pParam);
      break;
		case CMD_WRITE_ADV_DATA:
			//status = writeAdvData(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_ADV_DATA:
			//paramLen = readAdvData(&pParam);
      break;
		case CMD_WRITE_ADV_INTERVAL:
			//status = writeAdvInterval(pCmd,dataLen,TRUE);
			break;  /* fall through  readback the set data*/
		case CMD_READ_ADV_INTERVAL:
			//paramLen = readAdvInterval(&pParam);
			break;
		case CMD_READ_RSSI:
			//status = readRssi(pCmd,dataLen,cback);
			//return TRUE;
		case CMD_READ_BLE_STATE:
			//paramLen = readBleState(&pParam);
      break;
		case CMD_BLE_DISCONNECT:
			//status = writeBleDisconnect(pCmd,dataLen,TRUE);
      break;
		case CMD_WRITE_SCAN_DATA:
			//status = writeScanData(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_SCAN_DATA:
			//paramLen = readScanData(&pParam);
      break;
		case CMD_WRITE_UARTBAUD:
			//status = writeUartBaud(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_UARTBAUD:
			//paramLen = readUartBaud(&pParam);
      break;
		case CMD_WRITE_UARTMODE:
			//status = writeUartMode(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_UARTMODE:
			//paramLen = readUartMode(&pParam);
      break;
		case CMD_WRITE_GPIO_WAKEUP_CFG:
			//status = writeGpioWakeupCfg(pCmd,dataLen,TRUE);
      break;
		case CMD_READ_GPIO_WAKEUP_CFG:
			//paramLen = readGpioWakeupCfg(&pParam);
      break;
		case CMD_FACTORY_RESET:
			//status = ACT_SUCCESS;
      break;
		case CMD_WRITE_HCL_32KMODE:
			//status = writeHcl32KMode(pCmd,dataLen,TRUE);
      break;
		case CMD_WRITE_APP_MODE:
			//status = writeAppMode(pCmd,dataLen,TRUE);
      break;
		case CMD_WRITE_HOST_CFG_COMPLETED:
			//status = writeHostConfigCompleted(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_EXT_IRQ_CLOCK:
			//status = writeExtIrqClkSrc(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_SLEEP_CLK_ACCURACY:
			//status = writeSleepClkAccuracy(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_BOARD_TRIM_DATA:
			//status = writeBoardTrimData(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_TX_TRIM_DATA:
			//status = writeTxTrimData(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_TX_POWER_LEVEL_3V:
			//status = writeTxPowerLevel3VData(pCmd,dataLen,TRUE);
			break;
		case CMD_TXDATA_CMDMODE:
			writeDataOnCmdMode(0, cmd->hdr.data, cmd->hdr.len,0);
      break;
		case CMD_SET_BT4PATCH_ENTRY:
			//status = SetBt4PatchEntry(pCmd,dataLen);
      break;
		case CMD_READ_GPIO:
			//if(dataLen == 1) {
				//paramLen = readGpioVal(pCmd[0],&pParam);
			//}
      break;
		case CMD_WRITE_GPIO:
			//status = writeGpioVal(pCmd,dataLen,TRUE);
			break;
		case CMD_WRITE_UUID:
			//status = writeSvcUuid(pCmd,dataLen,TRUE);
			break;
		case CMD_INQUIRY_STATUS:
			//status = InquiryStatus(pCmd,dataLen,&pParam, &paramLen);
			break;
		case CMD_DOWNLOAD_PATCH:
		case CMD_WRITE_RAM:
			//status = downLoadPatchToRam(pCmd,dataLen);
      break;
		case CMD_EXEC_PATCH:
			/*status = doCheckSum(pCmd,dataLen);
			if(status == ACT_SUCCESS) {
				status = execRamPatch(pCmd,dataLen);
			} */
			break;
		case CMD_SWITCH_FIRMWARE:
			//status = doCheckSum(pCmd,dataLen);
      break;
		case CMD_READ_RAM:
			//status = readRamPatch(pCmd,dataLen,&pParam, &paramLen);
			break;
    case CMD_SWITCH_BATCH:
      //pstoreSet(PSTORE_TAG_REBOOT_BATCH, 1);
      break;
    case CMD_WRITE_COMP_ID:
			//writeCompId(pCmd,dataLen,FALSE);
			break;
		case CMD_WRITE_IMPL_REV:
			//writeImplRev(pCmd,dataLen,FALSE);
			break;
		case CMD_WRITE_BT_VER:
			//writeBtVer(pCmd,dataLen,FALSE);
			break;
		case CMD_WRITE_PWR_MODE:
			//writePowerMode(pCmd,dataLen,FALSE);
			break;     
		default:
		act_cmd_rsp(cmd->hdr.cmd_type, cmd->hdr.opcode, ACT_ERR_UNKNOWN_CMD);
		break;
		}
		//act_cmd_rsp(cmd->hdr.cmd_type, cmd->hdr.opcode, ACT_SUCCESS);
		memset(cmd, 0, sizeof(*cmd));
		k_fifo_put(&avail_queue, cmd);
    }
}


#define DEBUG_DUMP 1

#if DEBUG_DUMP
static struct k_work debug_work;
static u8_t recv_data[128];
static u8_t recv_cnt = 0;
void debug_work_handle(struct k_work *work){
	if (recv_cnt) {
		int i = 0;
		printk("recv_data: ");
		for (i = 0; i < recv_cnt; i++) {
			printk("%02x ", recv_data[i]);
		}
		printk("\n");
	}
	memset(recv_data, 0, sizeof(recv_data));
	recv_cnt = 0;
}

void start_debug_work(u8_t *buf, size_t *off)
{
	recv_cnt = *off;
	memcpy(&recv_data[0], buf, (recv_cnt > 128) ? 128 : recv_cnt);
	k_work_submit_to_queue(&k_sys_work_q, &debug_work);
}
#endif

/* Attention: recv_cb is called form uart interruption */
static u8_t *recv_cb(u8_t *buf, size_t *off)
{
	struct act_cmd_hdr *cmd = (void *) buf;
	struct act_cmd_buf *new_buf;
	u16_t len;

	if (*off < sizeof(*cmd)) {
		return buf;
	}

    len = sys_le16_to_cpu(cmd->len);
	if (len > ACT_CMD_MTU - sizeof(*cmd)) {
		//SYS_LOG_ERR("BT tester: invalid packet length");
#if DEBUG_DUMP
        start_debug_work(buf, off);
#endif
        *off = 0;
        return buf;
    }

	if (*off < sizeof(*cmd) + len) {
		return buf;
	}

	new_buf =  k_fifo_get(&avail_queue, K_NO_WAIT);
	if (!new_buf) {
		//SYS_LOG_ERR("BT tester: RX overflow");
#if DEBUG_DUMP
        start_debug_work(buf, off);
#endif
		*off = 0;
		return buf;
	}

	k_fifo_put(&cmds_queue, CONTAINER_OF(buf, struct act_cmd_buf, data));

	*off = 0;
	return new_buf->data;
}

void act_cmd_init(void)
{
    int i;
    struct act_cmd_buf *buf;
	
    for (i = 0; i < CMD_QUEUED; i++) {
        k_fifo_put(&avail_queue, &cmd_buf[i]);
    }

    k_thread_create(&cmd_thread, stack, STACKSIZE, cmd_handler,
                    NULL, NULL, NULL, 7, 0, K_NO_WAIT);
		
		printk("struct act_cmd_hdr %d\n",sizeof(struct act_cmd_hdr));
    buf = k_fifo_get(&avail_queue, K_NO_WAIT);
    uart_pipe_register(buf->data, ACT_CMD_MTU, recv_cb);
    
    k_work_init(&debug_work, debug_work_handle);

   // act_cmd_send(ACT_CMD_SERVICE_ID_CORE, CORE_EV_IUT_READY, NULL, 0);
}

void act_cmd_send(u8_t cmd_type, u16_t opcode, u8_t *data, size_t len)
{
    struct act_cmd_hdr msg;

    msg.cmd_type = cmd_type;
    msg.opcode = opcode;
    msg.len = len;

    uart_pipe_send((u8_t *)&msg, sizeof(msg));
    if (data && len) {
        uart_pipe_send(data, len);
    }
}

void act_cmd_rsp(u8_t service, u16_t opcode, u8_t status)
{
    struct act_cmd_status s;

    if (status == ACT_SUCCESS) {
        act_cmd_send(service, opcode, NULL, 0);
        return;
    }

    s.code = status;
    act_cmd_send(service, ACT_CMD_STATUS, (u8_t *) &s, sizeof(s));
}
