
#define BLE_ADV_STATE_DEF 		1  					/* default; start adv when power on */
#define BLE_ADV_INTERVAL_DEF 	0x0140 			/* 200 ms */
#define BLE_ADV_NAME_DATA_DEF	'a', 'c', 't', '-', 'a', 'n', 'c', 's', '\0',
#define BLE_ADV_NAME_LEN_DEF	9
#define BLE_ADV_NAME_DATA_MAX 20
#define BLE_CONN_INTERVAL_DEF 0x20        /* 40 ms  */


/* PIN */
#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \





