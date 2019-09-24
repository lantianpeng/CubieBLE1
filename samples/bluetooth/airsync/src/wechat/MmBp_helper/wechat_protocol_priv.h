#ifndef __WECHAT_PROTOCOL_PRIV_H__
#define __WECHAT_PROTOCOL_PRIV_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	errorCodeUnpackAuthResp		= -0x9990,
	errorCodeUnpackInitResp		= -0x9991,
	errorCodeUnpackSendDataResp	= -0x9992,
	errorCodeUnpackCtlCmdResp	= -0x9993,
	errorCodeUnpackRecvDataPush	= -0x9994,
	errorCodeUnpackSwitchViewPush	= -0x9995,
	errorCodeUnpackSwitchBackgroundPush = -0x9996,
	errorCodeUnpackErrorDecode	= -0x9997,
} mpbledemo2UnpackErrorCode;

typedef enum {
	errorCodeProduce		= -0x9980,
} mpbledemo2PackErrorCode;

typedef enum {
	sendTextReq			= 0x01,
	sendTextResp			= 0x1001,
	openLightPush			= 0x2001,
	closeLightPush			= 0x2002,
} BleDemo2CmdID;

#endif

