#ifndef __JUMP_H__
#define __JUMP_H__

//栈顶地址
#define STACK_POINTER 0x20000000 
//APP应用程序起始地址
#define APP_ADDRESS 0x08008000
//APP应用新程序版本地址
#define APP_NEW_VERSION_ADDRESS 0x08014000 
//25Q16程序起始位置
#define SPI_FLASH_APP_ADDRESS 0x00000000
//要下载的APP_NEW_VERSION大小
//#define APP_NEW_VERSION_SIZE  4008
#define APP_NEW_VERSION_SIZE  3992
//每次从25Q16到FLASH的字节数
#define SPI_TO_FLASH_SIZE     1024

//状态机
typedef enum
{
    //初始状态
    BOOT_COMMUNICATION_IDLE = 0, 
    //等待3S内没有接收到数据，则跳转到应用程序
    BOOT_COMMUNICATION_WAIT_CMD,
    //没有接收到数据,判断24C02的标志位,跳转到应用程序
    BOOT_COMMUNICATION_NO_DATA,
    //如果要更新，则进入接收状态,把程序放进24C02,等待接收完成
    BOOT_COMMUNICATION_UPDATE_AVAILABLE,
    //把新程序更新到APP_NEW_VERSION_ADDRESS中,更新24C02的标志位
    //把APP_NEW_VERSION_ADDRESS中的程序更新到APP_ADDRESS中,更新24C02的标志位
    BOOT_COMMUNICATION_UPDATE_TO_APP,
    //启动APP_ADDRESS应用程序
    BOOT_COMMUNICATION_JUMP_TO_APP,
} Boot_Communication_StateTypeDef;

//跳转程序
void JumpToApp(void);
//备用跳转程序
void JumpToBackupApp(void);
//擦除flash的特定区域
void InternalFlash_Erase(uint32_t start_addr, uint32_t len);
//给flash的特点区域写数据,半字方式写入
void InternalFlash_Write(uint32_t start_addr, uint8_t *data, uint32_t len);
//把25Q16的SPI_FLASH_APP_ADDRESS地址的内容放入flash的APP_NEW_VERSION_ADDRESS
uint8_t SPI_To_Flash_Internal(uint32_t W25Q16_start_addr,uint32_t flash_start_addr,uint32_t len);
//状态机处理
void Boot_Communication_StateMachine(void);

#endif
