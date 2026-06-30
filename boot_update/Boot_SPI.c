#include "main.h"
#include "spi.h"
#include "gpio.h"
#include "Boot_SPI.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include "Boot_Communication.h"

#define FLASH_CS_LOW() HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_RESET)
#define FLASH_CS_HIGH() HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET)

uint8_t flash_read_status(void)
{
    uint8_t cmd = SPI_FLASH_CMD_RDSR1;
    uint8_t status;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, &status, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
    return status;
}

static void flash_wait_ready(void)
{
    while (flash_read_status() & 0x01);  // 等待 BUSY 位清零
}

static void flash_write_enable(void)
{
    uint8_t cmd = SPI_FLASH_CMD_WREN_ENABLE;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
}

void SPI_flash_sector_erase(uint32_t addr)
{
    uint8_t cmd[4];
    cmd[0] = SPI_FLASH_CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    flash_write_enable();
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
    FLASH_CS_HIGH();

    flash_wait_ready();
}

void SPI_flash_pre_erase(uint32_t addr, uint32_t len)
{
    for(uint32_t i=0; i<(len-1)/SPI_FLASH_SECTOR_SIZE+1; i++)
    {
        SPI_flash_sector_erase(((addr / SPI_FLASH_SECTOR_SIZE) + i) * SPI_FLASH_SECTOR_SIZE);
    }
}

void SPI_flash_page_write(uint32_t addr, uint8_t *data, uint32_t len)
{
    if(len == 0)
    {
        return;
    }
    if (addr >= 0x200000 || addr + len > 0x200000)
    {
         return; // 地址越界
    } 
    uint32_t plen = len;
    uint32_t paddr=addr;
    uint8_t *pdata=data;
    uint8_t cmd[4];
    uint32_t write_len;
    while(plen > 0)
    {
        cmd[0] = SPI_FLASH_CMD_PAGE_PROG;
        cmd[1] = (paddr >> 16) & 0xFF;
        cmd[2] = (paddr >> 8) & 0xFF;
        cmd[3] = paddr & 0xFF;
        if(plen < SPI_FLASH_PAGE_SIZE - (paddr % SPI_FLASH_PAGE_SIZE))
        {
            write_len = plen;
        }
        else
        {
            write_len = SPI_FLASH_PAGE_SIZE - (paddr % SPI_FLASH_PAGE_SIZE);
        }
        flash_write_enable();
        FLASH_CS_LOW();
        HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
        HAL_SPI_Transmit(&hspi2, pdata, write_len, HAL_MAX_DELAY);
        FLASH_CS_HIGH();
        paddr += write_len;
        pdata += write_len;
        plen -= write_len;
        flash_wait_ready();
    }
}

void SPI_flash_page_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t cmd[4];
    cmd[0] = SPI_FLASH_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, data, len, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
}
