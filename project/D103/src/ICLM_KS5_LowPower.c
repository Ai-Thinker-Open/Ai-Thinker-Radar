#include "stdint.h"
#include "ICLM_KS5_LowPower.h"
#include "banyan.h"

#include "bsp_iic.h"


static uint16_t (*I2CWriteCallback)(uint16_t, uint8_t, uint16_t);
static uint16_t (*I2CReadCallback)(uint16_t, uint8_t, uint16_t*);

void RegisterI2CWriteCallback(uint16_t (*I2C_Write)(uint16_t, uint8_t, uint16_t))
{
    I2CWriteCallback = I2C_Write;
}

void RegisterI2CReadCallback(uint16_t (*I2C_Read)(uint16_t, uint8_t, uint16_t*))
{
    I2CReadCallback = I2C_Read;
}

static const RADAR_REG_LOWPWR RegList_LowPWR[] =
{
    {0x70, 0x1020},
    {0x6C, 0x8880},
    {0x6D, 0x8800},
    {0x72, 0x0650},
    {0x67, 0x0000},
    {0x66, 0xF0F0},
    {0x6E, 0x03FC},
    {0x41, 0xC864},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
};

static const RADAR_REG_LOWPWR RegList_ExtraLowPWR[] =
{
    {0x70, 0x1020},
    {0x6C, 0x8880},
    {0x6D, 0x8800},
    {0x72, 0x0650},
    {0x67, 0x0000},
    {0x66, 0xF0F0},
    {0x6E, 0x03FC},
    {0x41, 0x4804},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
};

static RADAR_REG_LOWPWR RegListNormal[] =
{
    {0x41, 0x0000},
    {0x72, 0x0000},
    {0x6C, 0x0000},
    {0x6D, 0x0000},
    {0x70, 0x0000},
    {0x6E, 0x0000},
    {0x66, 0x0000},
    {0x67, 0x0000},
    {0x00, 0x0000} 
};

static void Get_Radar_Reg(void)
{
    uint16_t loop = 0;
    while(RegListNormal[loop].addr)
    {
        // I2CReadCallback(I2C_ADDR_BanYan_Chip0,(uint8_t)RegListNormal[loop].addr,&RegListNormal[loop].val);
        bsp_iic_read(I2C_ADDR_BanYan_Chip0,(uint8_t)RegListNormal[loop].addr,&RegListNormal[loop].val);
        loop++;
    }
}

void Radar_ReEnterPDMode(uint8_t modetype)
{
	static uint8_t getTimes = 0;
    uint16_t loop = 0;
	if(!getTimes)
	{
		Get_Radar_Reg();
		getTimes++;
	}
    if (modetype == LOW_POWER)
    {
        while(RegList_LowPWR[loop].addr) 
        {
            // I2CWriteCallback(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegList_LowPWR[loop].addr), RegList_LowPWR[loop].val);
            bsp_iic_write(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegList_LowPWR[loop].addr), RegList_LowPWR[loop].val);
            loop++;
        }
    }
    if (modetype == EXTRA_LOW_POWER)
    {
        while(RegList_ExtraLowPWR[loop].addr) 
        {
            // I2CWriteCallback(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegList_ExtraLowPWR[loop].addr), RegList_ExtraLowPWR[loop].val);
            bsp_iic_write(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegList_ExtraLowPWR[loop].addr), RegList_ExtraLowPWR[loop].val);
            loop++;
        }
    }
}

void Radar_ReEnterNormalMode(void)
{
    uint16_t loop = 0;
    while(RegListNormal[loop].addr) 
    {
        // I2CWriteCallback(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegListNormal[loop].addr), RegListNormal[loop].val);
        bsp_iic_write(I2C_ADDR_BanYan_Chip0, (uint8_t)(RegListNormal[loop].addr), RegListNormal[loop].val);
        loop++;
    }
}
