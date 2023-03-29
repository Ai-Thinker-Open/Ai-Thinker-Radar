/**
 * Copyright (c) 2016-2021 Bouffalolab Co., Ltd.
 *
 * Contact information:
 * web site:    https://www.bouffalolab.com/
 */

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
// #include <timers.h>
#include <event_groups.h>

#include <aos/kernel.h>
#include <aos/yloop.h>
#include <cli.h>
#include <blog.h>
#include <easyflash.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <bl_timer.h>
#include <hal_sys.h>

#include "../D103/inc/platform.h"
#include "../D103/inc/config.h"
#include "../D103/inc/banyan.h"
#include "../D103/inc/dataprocess.h"
#include "../D103/inc/cmdprocess.h"
#include "../D103/inc/bsp_uart.h"
#include "../D103/inc/bsp_spi.h"
#include "../D103/inc/mcu_sleep.h"


#include <hosal_gpio.h>
#include <blog.h>
#include "bsp_spi.h"
// #include "axk_ota_import.h"
// #include "at_ota.h"
#include "wifi_interface.h"
#include "ft_banyan.h"
#include "axk_radar_cfg.h"

#define OTA_TEST_ENABLE     (1)
#define OTA_TEST_URL        "aithinker-static.oss-cn-shenzhen.aliyuncs.com"

TimerHandle_t spi_timeout_handle = NULL;

EventGroupHandle_t CmdProcessEventGroup;

TaskHandle_t axk_tcp_task_handle = NULL;

TaskHandle_t QueueTaskHandle = NULL;

#define TIME_5MS_IN_32768CYCLE  (164) // (5000/(1000000/32768))

int pds_start = 0;

#if ( configUSE_TICKLESS_IDLE != 0 )
void vApplicationSleep( TickType_t xExpectedIdleTime_ms )
{
#if defined(CFG_BLE_PDS)
    int32_t bleSleepDuration_32768cycles = 0;
    int32_t expectedIdleTime_32768cycles = 0;
    eSleepModeStatus eSleepStatus;
    bool freertos_max_idle = false;

    if (pds_start == 0)
        return;

    if(xExpectedIdleTime_ms + xTaskGetTickCount() == portMAX_DELAY){
        freertos_max_idle = true;
    }else{
        xExpectedIdleTime_ms -= 1;
        expectedIdleTime_32768cycles = 32768 * xExpectedIdleTime_ms / 1000;
    }

    if((!freertos_max_idle)&&(expectedIdleTime_32768cycles < TIME_5MS_IN_32768CYCLE)){
        return;
    }

    /*Disable mtimer interrrupt*/
    *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 0;

    eSleepStatus = eTaskConfirmSleepModeStatus();
    if(eSleepStatus == eAbortSleep || ble_controller_sleep_is_ongoing())
    {
        /*A task has been moved out of the Blocked state since this macro was
        executed, or a context siwth is being held pending.Restart the tick
        and exit the critical section. */
        /*Enable mtimer interrrupt*/
        *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 1;
        //printf("%s:not do ble sleep\r\n", __func__);
        return;
    }

    bleSleepDuration_32768cycles = ble_controller_sleep();

	if(bleSleepDuration_32768cycles < TIME_5MS_IN_32768CYCLE)
    {
        /*BLE controller does not allow sleep.  Do not enter a sleep state.Restart the tick
        and exit the critical section. */
        /*Enable mtimer interrrupt*/
        //printf("%s:not do pds sleep\r\n", __func__);
        *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 1;
    }
    else
    {
        printf("%s:bleSleepDuration_32768cycles=%ld\r\n", __func__, bleSleepDuration_32768cycles);
        if(eSleepStatus == eStandardSleep && ((!freertos_max_idle) && (expectedIdleTime_32768cycles < bleSleepDuration_32768cycles)))
        {
           hal_pds_enter_with_time_compensation(1, expectedIdleTime_32768cycles - 40);//40);//20);
        }
        else
        {
           hal_pds_enter_with_time_compensation(1, bleSleepDuration_32768cycles - 40);//40);//20);
        }
    }
#endif
}
#endif


static void cmd_process_task(void *param)
{
    EventBits_t uxBits;

    for (;;)
    {
        uxBits = xEventGroupWaitBits(CmdProcessEventGroup, CMD_PROCESS_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        if( ( uxBits & CMD_PROCESS_BIT ) != 0 )
        {
            CmdProc_Recv();
        }
    }
}

static hosal_gpio_dev_t cs_io;
static EventGroupHandle_t SPIEventGroup;
#define EVT_GROUP_SPI_FLAG    (1<<1)

void cs_io_irq(void *arg)
{
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (SPIEventGroup != NULL) 
    {
        xResult = xEventGroupSetBitsFromISR(SPIEventGroup, EVT_GROUP_SPI_FLAG, &xHigherPriorityTaskWoken);
        if(xResult != pdFAIL) 
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void cs_interrupt_init(void)
{
    cs_io.port = 22;
    cs_io.config = INPUT_PULL_UP;
    hosal_gpio_init(&cs_io);
    hosal_gpio_irq_set(&cs_io, HOSAL_IRQ_TRIG_NEG_PULSE, cs_io_irq, NULL);
}

#define SPI_INII_STACK_SIZE (512)
/* Structure that will hold the TCB of the task being created. */
StaticTask_t xInitTaskBuffer;
StackType_t xInitStack[ SPI_INII_STACK_SIZE ];

void spi_init_task(void *param)
{
    EventBits_t uxBits;

    void (*func_init)(void) = param;

    for(;;)
    {
        uxBits = xEventGroupWaitBits(SPIEventGroup, EVT_GROUP_SPI_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & EVT_GROUP_SPI_FLAG)
        {
            // hosal_gpio_finalize(&cs_io);
            // hosal_gpio_irq_mask(&cs_io, 1); 
            func_init();
            break;
        }
    }

    vTaskDelete(NULL);
}

void create_spi_init_task(void (*spi_slave_init_type)(void))
{
    TaskHandle_t handle;
    handle = xTaskCreateStatic(
                            spi_init_task,       /* Function that implements the task. */
                            "spi_init_t",          /* Text name for the task. */
                            SPI_INII_STACK_SIZE,      /* Number of indexes in the xStack array. */
                            spi_slave_init_type,    /* Parameter passed into the task. */
                            configMAX_PRIORITIES-1,/* Priority at which the task is created. */
                            xInitStack,          /* Array to use as the task's stack. */
                            &xInitTaskBuffer );  /* Variable to hold the task's data structure. */

    if (handle == NULL)
    {
        printf("create spi_init_task failed\r\n");
    }
}


#define RD_VERSION  "1.0.0"


void axk_at_boot_info(void)
{
    g_uartDmaSendingFlag = 1;
    uart_dma_send((uint8_t *)"################################################\r\n", strlen("################################################\r\n"));
    while (g_uartDmaSendingFlag){};
    g_uartDmaSendingFlag = 1;
    uart_dma_send((uint8_t *)"SDK version:"BL_SDK_VER"\r\n", strlen("SDK version:"BL_SDK_VER"\r\n"));
    while (g_uartDmaSendingFlag){};
    g_uartDmaSendingFlag = 1;
    uart_dma_send((uint8_t *)"compile_time:" __DATE__ " " __TIME__ "\r\n", strlen("compile_time:" __DATE__ " " __TIME__ "\r\n"));
    while (g_uartDmaSendingFlag){};
    g_uartDmaSendingFlag = 1;
    uart_dma_send((uint8_t *)"Bin version:"PROJ_VER"\r\n", strlen("Bin version:"PROJ_VER"\r\n"));
    while (g_uartDmaSendingFlag){};
    g_uartDmaSendingFlag = 1;
    uart_dma_send((uint8_t *)"################################################\r\n", strlen("################################################\r\n"));
    vTaskDelay(1000);
}


/**
 *  APP main entry
 */
int main(void)
{
    bsp_uart0_init();   // 初始化log口

    Platform_Init();    // 初始化串口、IIC

    // ################################################
    // AT version:2.0.0
    // SDK version:release_bl_iot_sdk_1.6.36
    // compile_time:Mar 16 2023 17:31:57
    // Bin version:V4.18_P2.0.0-8bd389b
    // ################################################

    // 启动信息检测
    // uart_dma_send((uint8_t *)"\r\n///////////////////////////////////////////////\r\n", strlen("\r\n///////////////////////////////////////////////\r\n"));
    // uart_dma_send((uint8_t *)"\r\n\r\nRd-01 version:"RD_VERSION"\r\ncompile_time:" __DATE__ " " __TIME__ "\r\n\r\n", strlen("\r\nRd-01 version:"RD_VERSION"\r\ncompile_time:" __DATE__ " " __TIME__ "\r\n\r\n"));
    // uart_dma_send((uint8_t *)"compile_time:" __DATE__ " " __TIME__ "      \r\n\r\n", strlen("compile_time:" __DATE__ " " __TIME__ "      \r\n\r\n"));
    // uart_dma_send((uint8_t *)"///////////////////////////////////////////////\r\n", strlen("///////////////////////////////////////////////\r\n"));
    axk_at_boot_info();
    Config_Init();      // 从flash读取配置，没有则写入默认配置

    CmdProcessEventGroup = xEventGroupCreate();
    SPIEventGroup = xEventGroupCreate();

    extern QueueHandle_t SpiDataQueue;
    SpiDataQueue = xQueueCreate(FUNC_QUEUE_SIZE, 48);
    if (SpiDataQueue == NULL)
    {
        printf("create queue failed!!!\r\n");
    }

    vTaskDelay(pdMS_TO_TICKS(3));
    Radar_Init();       // IIC写雷达reg

    xTaskCreate(cmd_process_task, "cmd_pro", 512, NULL, CMD_PROCESS_TASK_PRIORITY, NULL);

    DataProc_Init();    // 数据初始化、SPI初始化、串口发送回调注册
    CmdProc_Init();

    extern void queue_data_process_task(void *param);
    xTaskCreate(queue_data_process_task, "data_pro", 512, NULL, DATA_PROCESS_TASK_PRIORITY, &QueueTaskHandle);
    cs_interrupt_init();
    create_spi_init_task(bsp_spi_slave_init);

    xTaskCreate(uart_recv_timeout_check_task, "uart_rec_check", 128, NULL, UART_RECV_CHECK_TASK_PRIORITY, NULL);
    
    // spi_timeout_handle = xTimerCreate("spitimeout", pdMS_TO_TICKS(1000), pdTRUE, NULL, vSpiTimeoutTimerCallback);
    // if (spi_timeout_handle != NULL)
    // {
    //     xTimerReset(spi_timeout_handle, pdMS_TO_TICKS(20));
    // }

    // xTaskCreate(show_task_state_task, "showtask", 256, NULL, 5, NULL);   // 开启这个任务可能会导致程序跑飞
#if 0
    wifi_interface_init();
    tcpip_init(NULL, NULL);

    // extern void axk_tcp_client_task(void *params);
    // xTaskCreate(axk_tcp_client_task, "axk_tcp", 1024, NULL, 20, &axk_tcp_task_handle);
#endif

    extern void axk_radar_user(void);
    axk_radar_user();

    return 0;
}
