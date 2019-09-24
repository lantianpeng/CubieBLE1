
#define BLE_ADV_STATE_DEF 		1  					/* default; start adv when power on */
#define BLE_ADV_INTERVAL_DEF 	0x0140 			/* 200 ms */
#define BLE_ADV_NAME_DATA_DEF	'C', 'M', 'B', 'L', 'E', '-', '1', '1', '0', '3',
#define BLE_ADV_NAME_LEN_DEF	10
#define BLE_ADV_NAME_DATA_MAX 20
#define BLE_CONN_INTERVAL_DEF 0x10        /* 20 ms  */
#define BLE_TX_POWER_DEF			1
#define UART_BAUDRATE_DEF			0x0
#define BLE_ADV_USER_DATA_DEF	0x11, 0x22, 0x33, 0x44, 0x55, 0x66
#define BLE_ADV_USER_DATA_LEN 0x06 
#define DATA_SRV_UUID_L_DEF		0x01
#define DATA_SRV_UUID_H_DEF		0x00
#define TX_UUID_L_DEF					0x02
#define TX_UUID_H_DEF					0x00
#define RX_UUID_L_DEF					0x03
#define RX_UUID_H_DEF					0x00
#define DRV_SRV_UUID_L_DEF		0x04
#define DRV_SRV_UUID_H_DEF		0x00
#define ADC_UUID_L_DEF				0x05
#define ADC_UUID_H_DEF				0x00
#define PWM_UUID_L_DEF				0x06
#define PWM_UUID_H_DEF				0x00
#define BLE_RSSI_MODE_DEF     0						/* default: exit rssi mode and can't get rssi */
#define GPIO_RTS_ENABLE_DEF   1         	/* default: disable rts func */


/* module function pin */
#define MODULE_EN_PIN       18  //18 for 1103
#define MODULE_TX_PIN       3   //3
#define MODULE_RX_PIN       2   //2
#define MODULE_DIO_PIN      13  //SWDIO
#define MODULE_CLK_PIN      12  //SWCLK   
#define MODULE_CTS_PIN  		0   //0
#define MODULE_RTS_PIN      1   //7
#define MODULE_NTF_PIN   		7   //1
#define MODULE_RESET_PIN		8   //9
#define MODULE_ADC0_PIN     5   //5
#define MODULE_FLOW_PIN     9   //8
#define MODULE_PWM0_PIN     6   //6 PWM1
#define MODULE_PWM1_PIN     4   //3 PWM3

/* for debug */
#define MODULE_PRINT_TX     4

/* PIN */
#define BOARD_PIN_CONFIG	\
	{MODULE_RX_PIN, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{MODULE_TX_PIN, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{MODULE_PRINT_TX, 4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{MODULE_ADC0_PIN, 1}, 



