/**
  ******************************************************************************
  * @file    platform.h
  * @author  ting.gao@iclegend.com
  * @brief   platform dependent header file
  ******************************************************************************
  */
#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>

#ifdef GD32_PLATFORM
#include "gd32_systick.h"
#include "gd32_gpio.h"
#include "gd32_uart.h"
#include "gd32_i2c.h"
#include "gd32_spi.h"
#include "gd32_flash.h"
#include "gd32_rtc.h"
#include "gd32_exit.h"
#include "gd32_timer.h"
#include "gd32_fwdgt.h"
#include "ICLM_KS5_LowPower.h"
#endif

#ifdef STM32_PLATFORM
#include "stm32_clock.h"
#include "stm32_gpio.h"
#include "stm32_uart.h"
#include "stm32_i2c.h"
#include "stm32_spi.h"
#include "stm32_flash.h"
#include "stm32_misc.h"
#ifdef DEVMB
#include "w25qxx.h"
#include "stm32_qspi.h"
#endif
#endif

#ifdef TEAK
#include "iclm32_spi.h"
#include "iclm32_misc.h"
#include "iclm32_int.h"
#include "iclm32_gpio.h"
#include "iclm32_timer.h"
#include "iclm32_uart.h"
#include "iclm32_flash.h"
#include "iclm32_i2c.h"
#include "iclm32_mstspi.h"
#endif

#include "bsp_spi.h"

#define INDICATOR_RECV_THRESHOLD            (1000)
#define INDICATOR_RECV_THD_DFFT_SHIFT       (3)
#define INDICATOR_RECV_THD_DPEAK_SHIFT      (6)
#define INDICATOR_SEND_OF_THRESHOLD         (10)

extern TimerHandle_t timer_60ms_handle;

void Platform_Init(void);
void Delay(uint32_t Delay);
void Indicator_RadarDataReceived(uint16_t threshold);
void Indicator_RadarDataIndexError(void);
void Indicator_RadarDataRecvOverFlow(void);
void Indicator_RadarDataSendOverFlow(void);
void Indicator_CmdDataRecvOverFlow(void);

#ifdef __cplusplus
}
#endif

#endif

