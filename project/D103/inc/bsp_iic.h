#ifndef _BSP_IIC_H_
#define _BSP_IIC_H_

#include <hosal_i2c.h>


extern hosal_i2c_dev_t i2c0;

void bsp_hosal_i2c_master_init(int scl_pin, int sda_pin);
int bsp_iic_write(uint16_t dev_addr, uint8_t reg_addr, uint16_t data);
int bsp_iic_read(uint16_t devAddr, uint8_t regAddr, uint16_t *regVal);

#endif
