# BootLoader
这是使用串口接收发来数据的更新程序,当然可以改成你要的接收程序,比如CAN,LORA等通信方式
I2C连接的是AT24C02,里面存放的是新旧程序标志位;SPI连接的是W25Q16,里面存放的是串口接收的更新程序的.bin(可以在keil的魔法棒的User下的After Build/Rebuild的run #1分析以下内容
"D:\stm32\Keil\ARM\ARMCC\bin\fromelf.exe" --bin -o"$L@L.bin" "#L"  根据自己keil的地址填写修改,可以右键keil找到位置)
