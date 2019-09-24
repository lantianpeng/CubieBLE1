/* common config */
#include "autoconf.h"

/* application config */

/* msg type */
enum {
	MSG_NULL,

	MSG_KEY_INPUT,
	MSG_LOW_POWER,

	MSG_BLE_STATE,
	MSG_AUDIO_INPUT,
	MSG_APP_TIMER,
	MSG_OTA_EVENT,
};

/* BT */
#undef CONFIG_BT_DEVICE_NAME
#undef CONFIG_BT_MAX_CONN
#undef CONFIG_BT_MAX_PAIRED
#undef CONFIG_BT_RX_BUF_COUNT
#undef CONFIG_BT_L2CAP_TX_BUF_COUNT

#define CONFIG_BT_DEVICE_NAME "BLE_RMC"
#define CONFIG_BT_DEVICE_NAME_1 "BLE_RMC_TEST_1"
#define CONFIG_BT_DEVICE_NAME_2 "BLE_RMC_TEST_2"
#define CONFIG_BT_DEVICE_NAME_3 "BLE_RMC_TEST_3"
#define CONFIG_BT_DEVICE_NAME_4 "BLE_RMC_TEST_4"

#define CONFIG_BT_MAX_CONN 1
#define CONFIG_BT_MAX_PAIRED 1
#define CONFIG_BT_RX_BUF_COUNT 3
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 4

/* STACK */
#undef CONFIG_IDLE_STACK_SIZE
#undef CONFIG_BT_RX_STACK_SIZE

#define CONFIG_IDLE_STACK_SIZE (256+256)
#define CONFIG_BT_RX_STACK_SIZE (1280 - 300)

/* set it if use undirect adv for reconncting, IOS must set it, optional for android */
//#define CONFIG_USE_UNDIRECT_ADV_FOR_RECONN 1

/* choose appropriate encode algorithm */
//#define USE_AL_ENCODE_3_4_1
//#define USE_AL_ENCODE_2_8_1
//#define USE_AL_ENCODE_2_8_1_2
#define USE_AL_ENCODE_1

#ifdef USE_AL_ENCODE_1
#undef CONFIG_KERNEL_SHELL
#define CONFIG_KERNEL_SHELL 0
#endif

#if defined(USE_AL_ENCODE_1)
	#define CONFIG_HID_VOICE_REPORT_ID 5
#elif defined(USE_AL_ENCODE_2_8_1)
	#define CONFIG_HID_VOICE_REPORT_ID 6
#elif defined(USE_AL_ENCODE_3_4_1)
	#define CONFIG_HID_VOICE_REPORT_ID 2
#elif defined(USE_AL_ENCODE_3_4_1_2)
	#define CONFIG_HID_VOICE_REPORT_ID 8
#elif defined(USE_AL_ENCODE_2_8_1_2)
	#define CONFIG_HID_VOICE_REPORT_ID 9
#else
	#define CONFIG_HID_VOICE_REPORT_ID 2
#endif

/* use 0x0221 SEARCH key as default voice key */
#define CONFIG_ANDROID_TV_BOX

#define CONFIG_IRC_RX 1

//#include "rmc_atb1103_yt.h"
//#include "rmc_atb1103_yt_v2.h"
//#include "rmc_atb1103_yt_v21.h"
//#include "rmc_atb110x_dvb_v10.h"
//#include "rmc_atb1103_atv.h"
#include "rmc_atb1109_ft.h"
