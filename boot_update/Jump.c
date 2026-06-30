#include <string.h>
#include <stdio.h>
#include "main.h"
#include "Boot_I2C.h"
#include "Boot_SPI.h"
#include "Jump.h"
#include "Boot_Communication.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "crc.h"

//初始化状态机
Boot_Communication_StateTypeDef boot_communication_state = BOOT_COMMUNICATION_IDLE;
//3s更新标志位
volatile uint8_t update_flag = 0;
//25Q16转到flash标志位
volatile uint8_t flash_flag=0;
//SPI处理标志位
volatile uint8_t spi_receive_flag=0;
//全局缓冲区
uint8_t buff[SPI_TO_FLASH_SIZE]={0};
//串口接收
uint32_t current_address = SPI_FLASH_APP_ADDRESS;
//接收数据包
extern uint8_t Data[BOOT_COMMUNICATION_SIZE];
extern uint8_t pData[BOOT_COMMUNICATION_SIZE];
//接收数据大小
extern uint16_t Data_Size;


void JumpToApp(void)
{
    uint32_t app_stack=*(volatile uint32_t*)APP_ADDRESS; //获取应用程序的栈顶地址
    void (*app_entry)(void);
    if((app_stack&0x2ffe0000)!=STACK_POINTER) //检查栈顶地址是否有效
    {
        return; //如果无效，则返回
    }
    app_entry=(void (*)(void))(*(volatile uint32_t*)(APP_ADDRESS+4)); //获取应用程序的复位处理函数地址
    HAL_UART_AbortReceive_IT(&huart1);
    //手动关闭 UART 外设（比 DeInit 更底层）
    CLEAR_BIT(huart1.Instance->CR1, USART_CR1_RXNEIE | USART_CR1_IDLEIE);
    CLEAR_BIT(huart1.Instance->CR1, USART_CR1_UE);   // 关闭 USART
    HAL_I2C_DeInit(&hi2c1);
    HAL_SPI_DeInit(&hspi2);
    HAL_UART_DeInit(&huart1);
    __disable_irq(); //禁用中断
    __set_MSP(app_stack); //设置主堆栈指针
    app_entry(); //跳转到应用程序的复位处理函数
}

void JumpToBackupApp(void)
{
    uint32_t app_stack=*(volatile uint32_t*)APP_NEW_VERSION_ADDRESS; //获取应用程序的栈顶地址
    void (*app_entry)(void);
    if((app_stack&0x2ffe0000)!=STACK_POINTER) //检查栈顶地址是否有效
    {
        return; //如果无效，则返回
    }
    app_entry=(void (*)(void))(*(volatile uint32_t*)(APP_NEW_VERSION_ADDRESS+4)); //获取应用程序的复位处理函数地址
    HAL_UART_AbortReceive_IT(&huart1);
    //手动关闭 UART 外设（比 DeInit 更底层）
    CLEAR_BIT(huart1.Instance->CR1, USART_CR1_RXNEIE | USART_CR1_IDLEIE);
    CLEAR_BIT(huart1.Instance->CR1, USART_CR1_UE);   // 关闭 USART
    HAL_I2C_DeInit(&hi2c1);
    HAL_SPI_DeInit(&hspi2);
    HAL_UART_DeInit(&huart1);
    __disable_irq(); //禁用中断
    __set_MSP(app_stack); //设置主堆栈指针
    app_entry(); //跳转到应用程序的复位处理函数
}

//擦除flash的特定区域
void InternalFlash_Erase(uint32_t start_addr, uint32_t len)
{
    uint32_t PageError;
    FLASH_EraseInitTypeDef erase;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGERR | FLASH_FLAG_BSY);
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = start_addr;
    erase.NbPages = (len-1)/FLASH_PAGE_SIZE+1;
    if(HAL_FLASHEx_Erase(&erase,&PageError)!=HAL_OK)
    {
        printf("Flash erase error");
    }
    HAL_FLASH_Lock();
}
//给flash的特点区域写数据,半字方式写入
void InternalFlash_Write(uint32_t start_addr, uint8_t *data, uint32_t len)
{
    uint16_t pdata=0;
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGERR | FLASH_FLAG_BSY);
    for(uint32_t i=0;i<len/2;i++)
    {
        pdata=data[2*i]|(data[2*i+1]<<8);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,start_addr+i*2,pdata);
    }
    if(len%2)
    {
        pdata=data[len-1]|0xFF00;
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,start_addr+len-1,pdata);
    }
    HAL_FLASH_Lock();
}
//把25Q16的SPI_FLASH_APP_ADDRESS地址的内容放入flash的APP_NEW_VERSION_ADDRESS
uint8_t SPI_To_Flash_Internal(uint32_t W25Q16_start_addr,uint32_t flash_start_addr,uint32_t len)
{
    uint32_t dW25Q16_start_addr=W25Q16_start_addr;
    uint32_t dflash_start_addr=flash_start_addr;
    uint32_t dlen = 0;
    if(flash_start_addr==APP_NEW_VERSION_ADDRESS)
    {
        Boot_I2C_writebyte(UPDATE_FLAG_NEW_ADDRESS,UPDATE_FLAG_FAIL);
    }
    else if(flash_start_addr==APP_ADDRESS)
    {
        Boot_I2C_writebyte(UPDATE_FLAG_ADDRESS,UPDATE_FLAG_FAIL);
    }
    InternalFlash_Erase(flash_start_addr,len);
    while(dlen<len)
    {
        if(len-dlen>=SPI_TO_FLASH_SIZE)
        {
            SPI_flash_page_read(dW25Q16_start_addr,buff,SPI_TO_FLASH_SIZE);
            InternalFlash_Write(dflash_start_addr,buff,SPI_TO_FLASH_SIZE);
            dW25Q16_start_addr+=SPI_TO_FLASH_SIZE;
            dflash_start_addr+=SPI_TO_FLASH_SIZE;
            dlen+=SPI_TO_FLASH_SIZE;
            memset(buff,0,SPI_TO_FLASH_SIZE);
        }
        else
        {
            SPI_flash_page_read(dW25Q16_start_addr,buff,len-dlen);
            InternalFlash_Write(dflash_start_addr,buff,len-dlen);
            Boot_I2C_writebyte(UPDATE_FLAG_NEW_ADDRESS,1);
            break;
        }
    }
    if(flash_start_addr==APP_NEW_VERSION_ADDRESS)
    {
        Boot_I2C_writebyte(UPDATE_FLAG_NEW_ADDRESS,UPDATE_FLAG_OK);
        return 1;
    }
    else if(flash_start_addr==APP_ADDRESS)
    {
        Boot_I2C_writebyte(UPDATE_FLAG_ADDRESS,UPDATE_FLAG_OK);
        return 1;
    }
    return 0;
}
uint32_t CRC_flash_cal(uint32_t addr,uint32_t len)
{
	uint32_t *curr_addr = (uint32_t *)addr;
	uint32_t dlen = (len + 3)/4;
	//复位CRC
	__HAL_CRC_DR_RESET(&hcrc);
	uint32_t cal = HAL_CRC_Calculate(&hcrc,curr_addr,dlen);
	return cal;
}
uint32_t CRC_uart_cal(const uint8_t *addr,uint32_t len)
{
	uint32_t dlen = (len + 3)/4;
	//复位CRC
	__HAL_CRC_DR_RESET(&hcrc);
	uint32_t cal = HAL_CRC_Calculate(&hcrc,(uint32_t *)addr,dlen);
	return cal;
}
static uint32_t s_ider_enter_tick;
void Boot_Communication_StateMachine(void)
{
    switch(boot_communication_state)
    {
        case BOOT_COMMUNICATION_IDLE:
        {
            Communication_Receive(&huart1);
            s_ider_enter_tick = HAL_GetTick();
            boot_communication_state = BOOT_COMMUNICATION_WAIT_CMD;
            break;
        }
        case BOOT_COMMUNICATION_WAIT_CMD:
        {
            //3S内没有接收到数据，则跳转到应用程序
            if(update_flag)
            {
                boot_communication_state = BOOT_COMMUNICATION_UPDATE_AVAILABLE;
                SPI_flash_pre_erase(SPI_FLASH_APP_ADDRESS, APP_NEW_VERSION_SIZE);
                printf("Update available, waiting for data...\r\n");
            }
            else if(HAL_GetTick() - s_ider_enter_tick> 3000)
            {
                boot_communication_state = BOOT_COMMUNICATION_NO_DATA;
            }
            break;
        }
        case BOOT_COMMUNICATION_NO_DATA:
        {
            //没有接收到数据,判断24C02的标志位,跳转到应用程序
            if(Boot_I2C_readkey() == 1)
            {
                JumpToApp();
            }
            else if(Boot_I2C_readkey() == 2)
            {
                JumpToBackupApp();
            }
            else
            {
                //程序被破坏
                printf("No update flag found, staying in bootloader mode.\r\n");
                boot_communication_state = BOOT_COMMUNICATION_UPDATE_AVAILABLE;
            }
            break;
        }
        case BOOT_COMMUNICATION_UPDATE_AVAILABLE:
        {
            //如果要更新，则进入接收状态，等待接收完成
            if(spi_receive_flag)
            {
                //把接收的数据存入25Q16中
                SPI_flash_page_write(current_address, Data, Data_Size);
                current_address += Data_Size;   // 补上地址更新
                
                spi_receive_flag=0;
                if(current_address >= SPI_FLASH_APP_ADDRESS + APP_NEW_VERSION_SIZE)
                {
                    flash_flag = 1;  // 写完之后马上判断，不依赖下一包回调
					
                }
            }
            if(flash_flag)
            {
                boot_communication_state = BOOT_COMMUNICATION_UPDATE_TO_APP;
            }
            break;
        }
        case BOOT_COMMUNICATION_UPDATE_TO_APP:
        {
            if(SPI_To_Flash_Internal(SPI_FLASH_APP_ADDRESS,APP_NEW_VERSION_ADDRESS,APP_NEW_VERSION_SIZE))
            {
                if(SPI_To_Flash_Internal(SPI_FLASH_APP_ADDRESS,APP_ADDRESS,APP_NEW_VERSION_SIZE))
				{
					if(CRC_flash_cal(APP_ADDRESS,APP_NEW_VERSION_SIZE)==CRC_uart_cal(Data,APP_NEW_VERSION_SIZE))
					{
						//FALLTHROUGH: 写入完成后直接跳转
						boot_communication_state = BOOT_COMMUNICATION_JUMP_TO_APP;
					}
					else
					{
						//重新接收
						boot_communication_state = BOOT_COMMUNICATION_UPDATE_AVAILABLE;
					}
				}
            }
            
            
        }
        case BOOT_COMMUNICATION_JUMP_TO_APP:
        {
            if(Boot_I2C_readkey() == 1)
            {
                JumpToApp();
            }
            else if(Boot_I2C_readkey() == 2)
            {
                JumpToBackupApp();
            }
            else
            {
                //程序被破坏
                printf("No update flag found, staying in bootloader mode.\r\n");
            }
            break;
        }
    }
}
