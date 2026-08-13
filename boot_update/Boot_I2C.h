#ifndef  __BOOT_I2C_H__
#define  __BOOT_I2C_H__


#define AT24C20_SIZE        256
#define AT24C20_PAGE_SIZE   8
#define AT24C20_ADDRESS     0xA0
//NEW_APP是否可用标志位地址,AT24C02
#define UPDATE_FLAG_ADDRESS 0x00
//存放Bug标志位,>6则程序正常,更新APP为NEW_APP程序,等于0则NEW_APP异常,进入APP(在boot里减一,在NEW_APP里加二)
#define UPDATE_BUG_ADDRESS 0x02
//地址对应的值
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
uint8_t Boot_I2C_bug(void);

#endif
