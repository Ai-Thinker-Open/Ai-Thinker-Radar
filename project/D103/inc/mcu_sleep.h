


#ifndef __MCU_SLEEP_H__
#define __MCU_SLEEP_H__


#include "dataprocess.h"
#include "cmdprocess.h" 

void McuSetStatus(void);
void setUartRecv_Flag (void);
void clearUartRec_Flag (void);
uint8_t getUartRecvFlag(void);
void setSOCEnterNormalMode(void);
void  setTimerOut_Flag(void);
void  clearTimerOut_Flag(void);
uint8_t  getTimerOut_Flag(void);
void SocSetStatus(void);

void rext_gpio_init(uint8_t status);
void rext_on(void);
void power_gpio_init(uint8_t status);
void power_on(void);
void power_off(void);
void radar_en_gpio_init(uint8_t status);

#endif