/* common config */
#include "autoconf.h"

#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 1

/* input-adckey */

#define CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME "ADCKEY_0"
#define CONFIG_INPUT_DEV_ACTS_ADCKEY_1_NAME "ADCKEY_1"

#undef CONFIG_BT_LLCC

#define CONFIG_UART_0 1

/* use bat val 3v for default */
#define ADCKEY_0_MAPS \
	{0,     0,     KEY_1},  /* (0/30)*1024/3.6 = 0       */ \
	{41,    123,   KEY_2},  /* (5/35)*1024/3.6 = 40.63   */ \
	{80,    240,   KEY_3},  /* (12/42)*1024/3.6 = 80.27  */ \
	{124,   372,   KEY_4},  /* (24/54)*1024/3.6 = 124.42 */ \
	{169,   507,   KEY_5},  /* (44/74)*1024/3.6 = 169.13 */ \
	{211,   633,   KEY_6},  /* (87/117)*1024/3.6 = 211.51 */

#define ADCKEY_1_MAPS \
	{0,     0,     KEY_7},  /* (0/30)*1024/3.6 = 0       */ \
	{41,    123,   KEY_8},  /* (5/35)*1024/3.6 = 40.63   */ \
	{80,    240,   KEY_9},  /* (12/42)*1024/3.6 = 80.27  */ \
	{124,   372,   KEY_10}, /* (24/54)*1024/3.6 = 124.42 */ \
	{169,   507,   KEY_11}, /* (44/74)*1024/3.6 = 169.13 */ \
	{211,   633,   KEY_12}, /* (87/117)*1024/3.6 = 211.51 */

#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{0, 1 },\
	{1, 1 },


