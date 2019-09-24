/* common config */
#include "autoconf.h"

/* application config */

/* BT */
#undef CONFIG_BT_DEVICE_NAME
#undef CONFIG_BT_RX_BUF_COUNT

#define CONFIG_BT_DEVICE_NAME "BLE_RMC_#8"
#define CONFIG_BT_RX_BUF_COUNT 4

/* STACK */
#undef CONFIG_IDLE_STACK_SIZE
#undef CONFIG_BT_RX_STACK_SIZE

#define CONFIG_IDLE_STACK_SIZE (256+256)
#define CONFIG_BT_RX_STACK_SIZE (1024 - 300)

/* DEEPSLEEP */
#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0
#define CONFIG_UART_0 1
#define CONFIG_UART_1 1
#undef CONFIG_UART_CONSOLE_ON_DEV_NAME
#define CONFIG_UART_CONSOLE_ON_DEV_NAME "UART_1"

#define CONFIG_SEND_SINGLE_TONE 

//#include "rmc_atb1103_yt.h"
//#include "rmc_atb1103_yt_v2.h"
#include "rmc_atb1103_yt_v21.h"
//#include "rmc_atb110x_dvb_v10.h"

/* LED */
#define CONFIG_LED_PIN LED_BTN_PIN
