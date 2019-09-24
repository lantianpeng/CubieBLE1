#include <string.h>

#include "../MmBp_Embedded_1.0.4/protobuf/epb_MmBp.h"
#include "../MmBp_Embedded_1.0.4/crc32/crc32.h"

#include  "wechat_protocol_priv.h"
#include  "wechat_protocol.h"

#include <misc/printk.h>
#include <misc/byteorder.h>

#define ntohs(x) sys_be16_to_cpu(x)
#define ntohl(x) sys_be32_to_cpu(x)
#define htons(x) sys_cpu_to_be16(x)
#define htonl(x) sys_cpu_to_be32(x)

static wxbt_state_t wxbt_state;

static const uint8_t challeange[CHALLENAGE_LENGTH] = {
	0x11, 0x22, 0x33, 0x44
};

static uint8_t wx_bond_addr[6];

void wx_set_mac_address(uint8_t *mac_addr)
{
	memcpy(wx_bond_addr, mac_addr, 6);
}

void data_produce_func_send(cmd_parameter_t *param, uint8_t **data,
			    uint32_t *len)
{
	BaseRequest basReq = { NULL };

	const uint8_t fix_head_len = sizeof(cmd_fix_head_t);
	cmd_fix_head_t fix_head = { 0xFE, 1, 0, htons(ECI_req_auth), 0 };

	wxbt_state.seq++;
	*len = 0;

	switch (param->cmd) {
	case CMD_AUTH: {
		AuthRequest authReq = {
			&basReq, false, { NULL, 0}, PROTO_VERSION, AUTH_PROTO,
			(EmAuthMethod)AUTH_METHOD, false, { NULL, 0}, true,
			{ wx_bond_addr, MAC_ADDRESS_LENGTH }, false,
			{ NULL, 0 }, false, { NULL, 0 }, true,
			{ DEVICE_ID, sizeof(DEVICE_ID) }
		};

		*len = epb_auth_request_pack_size(&authReq) + fix_head_len;
		*data = (uint8_t *) malloc(*len);
		if (*data == NULL)
			return;

		if (epb_pack_auth_request(&authReq, *data + fix_head_len,
					  *len - fix_head_len) < 0) {
			free(*data);
			*data = NULL;
			return;
		}

		fix_head.nCmdId = htons(ECI_req_auth);
		break;
	}

	case CMD_INIT: {
		/* has challeange */
		InitRequest initReq = {
			&basReq, false, { NULL, 0 }, true,
			{ (uint8_t *)challeange, CHALLENAGE_LENGTH }
		};

		*len = epb_init_request_pack_size(&initReq) + fix_head_len;
		*data = (uint8_t *) malloc(*len);
		if (*data == NULL)
			return;
		
		if (epb_pack_init_request(&initReq, *data + fix_head_len,
					  *len - fix_head_len) < 0) {
			free(*data);
			*data = NULL;
			return;
		}

		fix_head.nCmdId = htons(ECI_req_init);
		break;
	}

	case CMD_SENDDAT: {
		SendDataRequest sendDatReq = {
			&basReq,
			{ (uint8_t *)param->send_msg.str, param->send_msg.len },
			true, (EmDeviceDataType)EDDT_wxDeviceHtmlChatView
		};

		*len = epb_send_data_request_pack_size(&sendDatReq) + fix_head_len;
		*data = (uint8_t *)malloc(*len);
		if (*data == NULL)
			return;

		if (epb_pack_send_data_request(&sendDatReq, *data + fix_head_len,
					       *len - fix_head_len) < 0) {
			free(*data);
			*data = NULL;
			return;
		}

		fix_head.nCmdId = htons(ECI_req_sendData);
		wxbt_state.send_data_seq++;
		break;
	}
	}

	fix_head.nLength = htons(*len);
	fix_head.nSeq = htons(wxbt_state.seq);
	memcpy(*data, &fix_head, fix_head_len);

	return;
}

/*
 * return value:
 *	< 0 : error, check the error code for detail
 *	0   : data is not enough, need more data
 *	> 0 : parsed a complete pakcet,
 *	      return the consumed data in bytes.
 */ 
int data_consume_func_rec(uint8_t *data, uint32_t len,
			  uint8_t **raw_data, uint32_t *raw_len)
{
	const uint8_t fix_head_len = sizeof(cmd_fix_head_t);
	cmd_fix_head_t head_buf;
	cmd_fix_head_t *fix_head = &head_buf;

	memcpy(fix_head, data, fix_head_len);

	if (len < ntohs(fix_head->nLength)) {
 		/* data is not enough, need more data */
		return 0;
	}

	/* only handle one packet length */
	len = ntohs(fix_head->nLength);

	*raw_len = 0;

	switch (ntohs(fix_head->nCmdId)) {
	case ECI_resp_auth:
	{
		printk("%s, ECI_resp_auth\n", __func__);
		AuthResponse *authResp = epb_unpack_auth_response(data + fix_head_len, len - fix_head_len);
		if (authResp == NULL)
			return errorCodeUnpackAuthResp;

		if (authResp->base_response) {
			if (authResp->base_response->err_code == 0) {
				wxbt_state.auth_state = true;
			} else {
				epb_unpack_auth_response_free(authResp);
				return authResp->base_response->err_code;
			}
		}

		epb_unpack_auth_response_free(authResp);
		break;
	}

	case ECI_resp_init:
	{
		printk("%s, ECI_resp_init\n", __func__);
		InitResponse *initResp = epb_unpack_init_response(data + fix_head_len, len - fix_head_len);
		if (initResp == NULL)
			return errorCodeUnpackInitResp;

		if (initResp->base_response) {
			if (initResp->base_response->err_code == 0) {
				if (initResp->has_challeange_answer) {
					if (wechat_crc32(0, challeange, CHALLENAGE_LENGTH) == initResp->challeange_answer)
						wxbt_state.init_state = true;
				} else {
					wxbt_state.init_state = true;
				}
				wxbt_state.wechats_switch_state = true;
			} else {
				epb_unpack_init_response_free(initResp);
				return initResp->base_response->err_code;
			}
		}

		epb_unpack_init_response_free(initResp);
		break;
	}

	case ECI_resp_sendData:
	{
		printk("%s, ECI_resp_sendData\n", __func__);
		SendDataResponse *sendDataResp = epb_unpack_send_data_response(data + fix_head_len, len - fix_head_len);
		if (sendDataResp == NULL)
			return errorCodeUnpackSendDataResp;

		if (sendDataResp->base_response && sendDataResp->base_response->err_code != 0) {
			epb_unpack_send_data_response_free(sendDataResp);
			return sendDataResp->base_response->err_code;
		}

		epb_unpack_send_data_response_free(sendDataResp);
		break;
	}

	case ECI_push_recvData:
	{
		printk("%s, ECI_push_recvData\n", __func__);
		RecvDataPush *recvDatPush = epb_unpack_recv_data_push(data + fix_head_len, len - fix_head_len);
		if (recvDatPush == NULL)
			return errorCodeUnpackRecvDataPush;

		*raw_data = (uint8_t *)recvDatPush->data.data;
		*raw_len = recvDatPush->data.len;
		
		print_buffer(*raw_data, 1, *raw_len, 16, 0);

		epb_unpack_recv_data_push_free(recvDatPush);
		wxbt_state.push_data_seq++;
		break;
	}

	case ECI_push_switchView:
	{
		printk("%s, ECI_push_switchView\n", __func__);
		wxbt_state.wechats_switch_state = !wxbt_state.wechats_switch_state;
		SwitchViewPush *swichViewPush = epb_unpack_switch_view_push(data + fix_head_len, len - fix_head_len);
		if (swichViewPush == NULL)
			return errorCodeUnpackSwitchViewPush;

		epb_unpack_switch_view_push_free(swichViewPush);
		break;
	}

	case ECI_push_switchBackgroud:
	{
		printk("%s, ECI_push_switchBackgroud\n", __func__);
		SwitchBackgroudPush *switchBackgroundPush = epb_unpack_switch_backgroud_push(data + fix_head_len,
											     len - fix_head_len);
		if (!switchBackgroundPush)
			return errorCodeUnpackSwitchBackgroundPush;

		epb_unpack_switch_backgroud_push_free(switchBackgroundPush);
		break;
	}

	case ECI_none:
	case ECI_err_decode:
	default:
		break;
	}

	return len;
}
