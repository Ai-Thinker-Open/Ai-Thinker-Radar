/**
  ******************************************************************************
  * @file    platform.c
  * @author  ting.gao@iclegend.com
  * @brief   platform dependent services
  ******************************************************************************
  */
#include "platform.h"
#include "stdio.h"

#include <FreeRTOS.h>
#include <task.h>
#include <hosal_uart.h>
#include <hosal_gpio.h>
#include "bsp_uart.h"
#include "bsp_iic.h"
#include "mcu_sleep.h"

#include "axk_radar_cfg.h"

TimerHandle_t timer_60ms_handle;
#define RADAR_POWER_TIMEOUT (60)

static void timer_timeout_func(TimerHandle_t xTimer)
{
    // clearExitFlag(); // 外部中断唤醒
#if COMPLETE_DATA_LOG_ENABLE
    printf("radar sleep timeout\r\n");
#endif

    clearCmdHeadFlag(); 

    // #if (isMCU_SLEEP_MODE == 1)
    //     CmdProc_InCmdModeClear();
    // #else
    //     setTimerOut_Flag();
    // #endif

    setSOCEnterNormalMode();
    // clearTimerOut_Flag();

    extern void create_spi_init_task(void (*spi_slave_init_type)(void));
    create_spi_init_task(bsp_spi_slave_init);
}


void Delay(uint32_t time)
{
    vTaskDelay(pdMS_TO_TICKS(time));
}

#ifndef TEAK
void Indicator_RadarDataReceived(uint16_t threshold)
{
}

void Indicator_RadarDataIndexError(void)
{
}

void Indicator_RadarDataRecvOverFlow(void)
{
}

void Indicator_RadarDataSendOverFlow(void)
{
}

void Indicator_CmdDataRecvOverFlow(void)
{
}
#endif


void Platform_Init(void)
{
#if  0
#ifdef GD32_PLATFORM
    SYSTICK_Init();
#endif
    UART_Init();
    I2C_Init(I2C_Speed_1M);

    // 上电后，SOC先断电
    PW_GPIO_Init(1);

    // 上电后，REXT先不接地
    REXT_GPIO_Init(0);

    // 上电后，无用的引脚都配置为模拟输入
    NoneUseGPIO_Init(); 
    
    //REXT 断开后延迟3毫秒，来自AN10014C
    delay_1ms(3);
    
    //Soc上电
    PW_On();
    
    delay_1ms(7);

    GPIO_PORTA_PIN10_Exit_RCU_En();     // 开启某些时钟
    FWDGT_Init();                       // 看门狗2s使能
    #if (isMCU_SLEEP_MODE == 1)   
    TimerInit(TIMER2, 3000, 1);
    #else
    TimerInit(TIMER2, 60, 1);           // 开启定时器，60ms触发中断
    #endif

    // 注册iic读写回调
    RegisterI2CWriteCallback(I2C_Write);
    RegisterI2CReadCallback(I2C_Read);
#endif
    bsp_uart_init(1, 256000);
    bsp_hosal_i2c_master_init(14, 3);

    radar_en_gpio_init(1);

    // soc断电
    power_gpio_init(1); //pmos控制，高电平不导通
    // 上电后，REXT先不接地
    rext_gpio_init(0);  //nmos控制，低电平不导通
    // 上电后，无用的引脚都配置为模拟输入

    //REXT 断开后延迟3毫秒，来自AN10014C
    Delay(3);    

    // Soc上电   
    power_on();
    Delay(7);  

    // 使能看门狗 

    // 配置定时器60ms中断
    timer_60ms_handle = xTimerCreate("timer_ms", pdMS_TO_TICKS(RADAR_POWER_TIMEOUT), pdFALSE, (void *)0, timer_timeout_func);

    printf("Platform_Init done\r\n");
}



