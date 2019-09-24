nvram.prop：nvram_factory分区的配置项。
upgrade.ini：量产固件的脚本文件
fw_maker.cfg：fw文件的配置文件。
firmware.xml：raw文件/ota.zip的配置文件。

build.bat：批处理文件，实现以下功能：
	1）自动生成nvram.bin。输入文件是nvram.prop
	2）生成parttbl.bin，生成raw.bin。输入文件是firmware.xml和响应的bin文件。
	3) 生成fw。输入文件为fw_maker.cfg。
        4）生成适用于att工具的atf格式的fw。

注：依赖dd，需在pc的环境变量里增加dd的路径scripts\utils\dd-0.6beta3\



cmd环境：
d:\5601A-标案\ZS110A\samples\ble_rmc\test\brom\test\uart_launcher\spi0>python adfu_serial.py test_spi0_no_rand_0x0.ini

