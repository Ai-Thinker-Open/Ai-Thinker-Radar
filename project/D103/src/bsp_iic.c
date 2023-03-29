/**
 * Copyright (c) 2016-2021 Bouffalolab Co., Ltd.
 *
 * Contact information:
 * web site:    https://www.bouffalolab.com/
 */

#include <stdio.h>
#include <blog.h>

#include "bsp_iic.h"
#include "utilities.h"

/* About Pin:
*  if one pin % 2 is 0 ===> this pin can be used as i2c scl function
*  if one pin % 2 is 1 ===> this pin can be used as i2c sda function
*  such as: GLB_GPIO_PIN_0 ===> scl
*           GLB_GPIO_PIN_1 ===> sda
*/

hosal_i2c_dev_t i2c0;

void bsp_hosal_i2c_master_init(int scl_pin, int sda_pin)
{
    int ret     = -1;

    i2c0.port = 0;
    i2c0.config.freq = 100000;                                    /* only support 305Hz~100000Hz */
    i2c0.config.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT;     /* only support 7bit */
    i2c0.config.mode = HOSAL_I2C_MODE_MASTER;                     /* only support master */
    i2c0.config.scl = scl_pin;
    i2c0.config.sda = sda_pin;

    /* init i2c with the given settings */
    ret = hosal_i2c_init(&i2c0);
    if (ret != 0) 
    {
        hosal_i2c_finalize(&i2c0);
        printf("hosal i2c init failed!\r\n");
        return;
    }
}

// 往寄存器reg_addr写data
int bsp_iic_write(uint16_t dev_addr, uint8_t reg_addr, uint16_t data)
{
    // 需要连续写
    uint8_t databuff[3];
    databuff[0] = reg_addr;
    databuff[1] = (uint8_t)(data >> 8);
    databuff[2] = (uint8_t)(data);

    dev_addr >>= 1;

    hosal_i2c_master_send(&i2c0, dev_addr, (const uint8_t *)databuff, sizeof(databuff)/sizeof(databuff[0]), HOSAL_WAIT_FOREVER);

	return 0;
}


int bsp_iic_read(uint16_t devAddr, uint8_t regAddr, uint16_t *regVal)
{  
    uint16_t val = 0;
    uint8_t *pval = (uint8_t *)&val; 

    devAddr >>= 1;

    hosal_i2c_mem_read(&i2c0, devAddr, regAddr, HOSAL_I2C_MEM_ADDR_SIZE_8BIT, pval, sizeof(val), HOSAL_WAIT_FOREVER);
    *regVal = BIG16_TO_LE(val);

	return 0;
}

