#include "mcu_sleep.h"
#include "stdio.h"
#include "body_presence.h"
#include "banyan.h"
#include "ICLM_KS5_LowPower.h"

#include <hosal_gpio.h>
#include "bsp_uart.h"
#include "FreeRTOS.h"

#if ((isMCU_SLEEP_MODE == 1) && (MCU_POWER_UP_NO_SLEEP_EN == 1))
static uint16_t mcu_no_sleep_frame_cnt = 0;
#endif

volatile uint8_t UartRecvFlag = 0;

/*用来在MCU低功耗关闭情况下，Timer唤醒SOC*/
volatile uint8_t TimerOutFlag = 0;

hosal_gpio_dev_t power_gpio;
hosal_gpio_dev_t rext_gpio;
hosal_gpio_dev_t en_gpio;

void power_gpio_init(uint8_t status)
{
    power_gpio.port = 20;
    power_gpio.config = OUTPUT_OPEN_DRAIN_PULL_UP;
    hosal_gpio_init(&power_gpio);
    hosal_gpio_output_set(&power_gpio, status);
}

void radar_en_gpio_init(uint8_t status)
{
    en_gpio.port = 12;
    en_gpio.config = OUTPUT_PUSH_PULL;
    hosal_gpio_init(&en_gpio);
    hosal_gpio_output_set(&en_gpio, status);
}

void power_on(void)
{
    hosal_gpio_output_set(&power_gpio, 0);
}

void power_off(void)
{
    hosal_gpio_output_set(&power_gpio, 1);
}

void rext_gpio_init(uint8_t status)
{
    rext_gpio.port = 21;
    rext_gpio.config = OUTPUT_PUSH_PULL;
    hosal_gpio_init(&rext_gpio);
    hosal_gpio_output_set(&rext_gpio, status);
}

void rext_on(void)
{
    hosal_gpio_output_set(&rext_gpio, 1);
}


static uint8_t isMcuCouldEnterSleep(void)
{
    /* 3个条件，完整frame结束处理，没有收到命令头部， 没有外部中断 */
    //XXX不需要休眠，暂时屏蔽
    // if (isFrameLastChirpDataFinished() && (!getCmdHeadFlag())&& (!getExitFlag())) 
    // {
    //     return 1;
    // }
    return 0;
}

void GPIO_PIN_ReConfig(void)
{
    // Step 1: 断开REXT Pin 与 GND的连接
    rext_gpio_init(0);

    // Step 2: 延迟3ms
    vTaskDelay(pdMS_TO_TICKS(3));

    // Step 3: Soc 上电
    power_gpio_init(0);
}

/********************************************
 @名称；McuSetStatus
 @功能：设置MCU状态
 @参数：无
 @返回：无
 @作者：AE TEAM
*********************************************/
void McuSetStatus(void)
{
    /*由于使用MOS控制开关SOC，在SOC关闭的情况下接收cmd指令，会导致MCU唤醒，但此时SPI无数据，故加上此判断*/
    if(getUartRecvFlag() == 1)
    {
        // 断开REXT，soc上电
        GPIO_PIN_ReConfig();

        // Step 4: 延迟2ms，晶振匹配电容5pf
        Delay(2);

        // Step 5: 写入极低功耗寄存器, 再写入全部寄存器
        Radar_ReEnterPDMode(EXTRA_LOW_POWER);
        WriteSocAllReg();

        // Step 6: 延迟7ms
        Delay(22);

        // Step 7: REXT 接地， 高有效
        rext_on();

        // Step 8: 延迟1ms
        Delay(1);
        clearUartRec_Flag();
    }
    /* 判断能否进入睡眠模式 */
    // if(!isMcuCouldEnterSleep())
    // {
    //     return;
    // }

    return;
    
  /* 雷达前N帧数据MCU不进入睡眠，需要在MCU睡眠机制下进行 */
    #if ((isMCU_SLEEP_MODE == 1) && (MCU_POWER_UP_NO_SLEEP_EN == 1))
    if(mcu_no_sleep_frame_cnt < MCU_POWER_UP_NO_SLEEP_FRAME_CNT)
    {
        mcu_no_sleep_frame_cnt++;
        clearFrameLsatChirpDataFinished();
        GPIO_PIN_ReConfig();
        // Step 4: 延迟2ms，晶振匹配电容5pf
        Delay(2);

        // Step 5: 写入极低功耗寄存器, 再写入全部寄存器
        Radar_ReEnterPDMode(EXTRA_LOW_POWER);
        WriteSocAllReg();

        // Step 6: 延迟22ms
        Delay(22);

        // Step 7: REXT 接地， 高有效
        gpio_bit_set(GPIOB, GPIO_PIN_7);

        // Step 8: 延迟1ms
        Delay(1);
        return;
    }
    #endif
    /* RTC 初始化 */
    // rtc_alarm_init();    //TODO
    
    /* USART0 PA10脚配置外部中断 */
    // usart_interrupt_disable(USART0, USART_INT_RBNE); //TODO
    
    // GPIO_PORTA_PIN10_Exit_RCU_En();                     //TODO

    /* 再次判断能否进入睡眠模式 */
    if(!isMcuCouldEnterSleep())
    {
        // /* 恢复USART PA10 引脚 */
        // Usart0RxPinInit();
        // /* 读一边DR寄存器，以清除标志位 */
        // usart_data_receive(USART0);
        // usart_interrupt_enable(USART0, USART_INT_RBNE);
        return;
    }
    // GPIO_PIN_AnaConfig();                            //TODO供电IO设置为模拟输入
    /* 清除标志位 */
    clearFrameLsatChirpDataFinished(); 

    // exti_flag_clear(EXTI_10);                        // TODO
    
    /* MCU 进入深度睡眠 */
    // pmu_to_deepsleepmode(PMU_LDO_NORMAL, WFI_CMD);   // TODO

    /* MCU 唤醒后初始化时钟 */
    // SysClkInit();
    // exti_interrupt_disable(EXTI_10);
    
    /* 恢复USART PA10 引脚 */
    // Usart0RxPinInit();

    /* 读一遍DR寄存器，以清除标志位 */
    // usart_data_receive(USART0);
    // usart_interrupt_enable(USART0, USART_INT_RBNE);

    // GPIO_PIN_ReConfig(); //TODO重新上电

    // Step 4: 延迟2ms，晶振匹配电容5pf
    Delay(2);

    // Step 5: 写入极低功耗寄存器, 再写入全部寄存器
    Radar_ReEnterPDMode(EXTRA_LOW_POWER);
    WriteSocAllReg();

    // Step 6: 延迟22ms
    Delay(22);

    // Step 7: REXT 接地， 高有效
    // gpio_bit_set(GPIOB, GPIO_PIN_7); //TODO

    // Step 8: 延迟1ms
    Delay(1);
}

/********************************************
 @名称；setSOCEnterNormalMode
 @功能：打开SOC的供电，在一段时间延迟后写全部寄存器
 @参数：无
 @返回：无
 @作者：AE TEAM
*********************************************/
void setSOCEnterNormalMode(void)
{
    power_on();
    Delay(10);
    vTaskEnterCritical();      // 不进入临界区可能会导致雷达无数据出现
    WriteSocAllReg();
    vTaskExitCritical();
}

/********************************************
 @名称；SocSetStatus
 @功能：设置SOC工作模式
 @参数：无
 @返回：无
 @作者：AE TEAM
*********************************************/
void SocSetStatus(void)
{
    if(getTimerOut_Flag())
    {
        setSOCEnterNormalMode();
        clearTimerOut_Flag();

        void create_spi_init_task(void (*spi_slave_init_type)(void));
        create_spi_init_task(bsp_spi_slave_init);
    }
}
void setUartRecv_Flag (void)
{
    UartRecvFlag = 1;
}

void clearUartRec_Flag (void)
{
    UartRecvFlag = 0;
}

uint8_t getUartRecvFlag(void)
{
    return UartRecvFlag;
}

void  setTimerOut_Flag(void)
{
    TimerOutFlag = 1;
}

void  clearTimerOut_Flag(void)
{
    TimerOutFlag = 0;
}

uint8_t  getTimerOut_Flag(void)
{
    return TimerOutFlag;
}
