#ifndef __BOOT_COMMUNICATION_H__
#define __BOOT_COMMUNICATION_H__

//1024字节等待100ms
#define BOOT_COMMUNICATION_SIZE 1024


//启动空闲中断的串口接收函数
void Communication_Receive(UART_HandleTypeDef *huart);

#endif
