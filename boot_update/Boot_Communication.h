#ifndef __BOOT_COMMUNICATION_H__
#define __BOOT_COMMUNICATION_H__
//一次性接收
#define BOOT_COMMUNICATION_SIZE 8000


//启动空闲中断的串口接收函数
void Communication_Receive(UART_HandleTypeDef *huart);

#endif
