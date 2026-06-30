#include "main.h"
#include "i2c.h"
#include "Boot_I2C.h"
#include <string.h>

static void eeprom_wait_ready(void)
{
    while (HAL_I2C_IsDeviceReady(&hi2c1, AT24C20_ADDRESS, 100, HAL_MAX_DELAY) != HAL_OK);
}

void Boot_I2C_writebyte(uint8_t add, uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c1, AT24C20_ADDRESS,(uint16_t)add, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  eeprom_wait_ready();
}

void Boot_I2C_readbyte(uint8_t add, uint8_t *data)
{
  HAL_I2C_Mem_Read(&hi2c1, AT24C20_ADDRESS,(uint16_t)add, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

void Boot_I2C_writebag(uint8_t add, uint8_t *data, uint16_t len)
{
    uint16_t overlen = len;
    uint8_t *pdata = data;
    uint8_t padd = add;
    while(overlen > 0)
    {
        uint16_t writelen = AT24C20_PAGE_SIZE - (padd % AT24C20_PAGE_SIZE);
        if(writelen > overlen)
        {
            writelen = overlen;
        }
        HAL_I2C_Mem_Write(&hi2c1, AT24C20_ADDRESS,(uint16_t)padd, I2C_MEMADD_SIZE_8BIT, pdata, writelen, 1000);
        eeprom_wait_ready();
        overlen -= writelen;
        pdata += writelen;
        padd += writelen;
    }
}

void Boot_I2C_readbag(uint8_t add, uint8_t *data, uint16_t len)
{
  HAL_I2C_Mem_Read(&hi2c1, AT24C20_ADDRESS,(uint16_t)add, I2C_MEMADD_SIZE_8BIT, data, len, 1000);
}

void Boot_I2C_erase(void)
{
    uint8_t data[AT24C20_PAGE_SIZE];
    memset(data, 0xFF, AT24C20_PAGE_SIZE);
    for(uint16_t i=0; i<AT24C20_SIZE; i+=AT24C20_PAGE_SIZE)
    {
        Boot_I2C_writebag(i, data, AT24C20_PAGE_SIZE);
        eeprom_wait_ready();
    }
}

void Boot_I2C_writeflag(uint8_t add, uint8_t key)
{
    if(key)
    {
        Boot_I2C_writebyte(add , UPDATE_FLAG_OK);
        eeprom_wait_ready();
    }
    else
    {
        Boot_I2C_writebyte(add , UPDATE_FLAG_FAIL);
        eeprom_wait_ready();
    }
}

uint8_t Boot_I2C_readkey(void)
{
  uint8_t data[3];
  HAL_I2C_Mem_Read(&hi2c1, AT24C20_ADDRESS,(uint16_t)UPDATE_FLAG_ADDRESS, I2C_MEMADD_SIZE_8BIT, data, 3, 1000);
  if(data[0] == UPDATE_FLAG_OK)
  {
      return 1;
  }
  else if(data[2] == UPDATE_FLAG_OK)
  {
      return 2;
  }
  else
  {
      return 0;
  }
}
