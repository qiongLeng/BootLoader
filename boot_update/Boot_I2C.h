#ifndef  __BOOT_I2C_H__
#define  __BOOT_I2C_H__


#define AT24C20_SIZE        256
#define AT24C20_PAGE_SIZE   8
#define AT24C20_ADDRESS     0xA0
//APP地址
#define UPDATE_FLAG_ADDRESS 0x00
//NEW_APP地址
#define UPDATE_FLAG_NEW_ADDRESS 0x02
#define UPDATE_FLAG_OK      0x01
#define UPDATE_FLAG_FAIL    0x00


void Boot_I2C_writebyte(uint8_t add, uint8_t data);
void Boot_I2C_readbyte(uint8_t add, uint8_t *data);
void Boot_I2C_writebag(uint8_t add, uint8_t *data, uint16_t len);
void Boot_I2C_readbag(uint8_t add, uint8_t *data, uint16_t len);
//擦除整个AT24C20芯片
void Boot_I2C_erase(void);
//写入更新标志
void Boot_I2C_writeflag(uint8_t add,uint8_t key);
uint8_t Boot_I2C_readkey(void);

#endif
