#ifndef LOW_POWER_H
#define LOW_POWER_H
#endif

#include "stdint.h"

typedef enum
{
	LOW_POWER=0,
	EXTRA_LOW_POWER
}Power_type;

void RegisterI2CWriteCallback(uint16_t (*I2C_Write)(uint16_t, uint8_t, uint16_t));
void RegisterI2CReadCallback(uint16_t (*I2C_Read)(uint16_t, uint8_t, uint16_t*));
void Radar_ReEnterPDMode(uint8_t modetype);
void Radar_ReEnterNormalMode(void);

typedef struct RADAR_REGT
{
    uint16_t addr;
    uint16_t val;
}RADAR_REG_LOWPWR;
