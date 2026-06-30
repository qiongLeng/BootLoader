#ifndef __BOOT_SPI_H__
#define __BOOT_SPI_H__

#define SPI_FLASH_JEDEC_ID          0x9F //读取JEDEC ID命令
#define SPI_FLASH_CMD_WREN_ENABLE   0x06 //写使能命令
#define SPI_FLASH_CMD_WREN_DISABLE  0x04 //写禁止命令
#define SPI_FLASH_CMD_READ          0x03 //读取数据命令
#define SPI_FLASH_CMD_SECTOR_ERASE  0x20 //扇区擦除命令
#define SPI_FLASH_CMD_RDSR1         0x05 //读取状态寄存器1命令
#define SPI_FLASH_CMD_PAGE_PROG     0x02 //页编程命令
//块大小
#define SPI_FLASH_BLOCK_SIZE        (64*1024) //64K
//区大小
#define SPI_FLASH_SECTOR_SIZE       (4*1024) //4K
//页大小
#define SPI_FLASH_PAGE_SIZE         256      //256字节


//扇区擦除
void SPI_flash_sector_erase(uint32_t addr);
//预先擦除扇区
void SPI_flash_pre_erase(uint32_t addr, uint32_t len);
//页写入,支持跨页写入
void SPI_flash_page_write(uint32_t addr, uint8_t *data, uint32_t len);
//页读取
void SPI_flash_page_read(uint32_t addr, uint8_t *data, uint32_t len);

#endif
