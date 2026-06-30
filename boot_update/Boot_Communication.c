#include <string.h>
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "Boot_SPI.h"
#include "Boot_I2C.h"
#include "Boot_Communication.h"
#include "Jump.h"

//更新标志位
extern uint8_t update_flag;
//25Q16转到flash标志位
extern uint8_t flash_flag;
//现在的地址
extern uint32_t current_address;
//SPI处理标志位
extern uint8_t spi_receive_flag;

uint8_t pData[BOOT_COMMUNICATION_SIZE];
//接收数据包
uint8_t Data[BOOT_COMMUNICATION_SIZE];
//接收数据大小
uint16_t Data_Size;

void Communication_Receive(UART_HandleTypeDef *huart)
{
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    HAL_UARTEx_ReceiveToIdle_IT(huart, pData, BOOT_COMMUNICATION_SIZE);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1)
    {
        if(update_flag==1)
        {
            memcpy(Data,pData,Size);
            spi_receive_flag=1;
            Data_Size=Size;
        }
        else if(update_flag==0 && strncmp((char *)pData, "UPDATE", 6) == 0)
        {
            //接收到更新命令，进入接收状态
            update_flag = 1;
            
        }
		__HAL_UART_CLEAR_OREFLAG(huart);
		__HAL_UART_CLEAR_IDLEFLAG(huart);
		HAL_UARTEx_ReceiveToIdle_IT(huart, pData, BOOT_COMMUNICATION_SIZE); 
//        memset(pData, 0, BOOT_COMMUNICATION_SIZE); //清除未使用的缓冲区
    }
}
