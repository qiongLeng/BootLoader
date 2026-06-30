#ifndef __BOOT_COMMUNICATION_H__
#define __BOOT_COMMUNICATION_H__

//1024字节等待100ms
//#define BOOT_COMMUNICATION_SIZE 1024
//根据文件大小写,因为本人使用的是一次就可完成的CRC(根据Data与flash的CRC校验)
#define BOOT_COMMUNICATION_SIZE 4096

//启动空闲中断的串口接收函数
void Communication_Receive(UART_HandleTypeDef *huart);

#endif
