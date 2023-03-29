#include "bsp_spi.h"

#include <stdlib.h>
#include <stdio.h>
#include <hosal_spi.h>
#include <blog.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <bl602.h>
#include <bl_irq.h>
#include <bl602_gpio.h>
#include <bl602_glb.h>
#include <bl602_spi.h>
#include <hosal_gpio.h>
#include "dataprocess.h"
#include "mcu_sleep.h"

#include "bl602_dma.h"
#include <bl_dma.h>
#include "hosal_dma.h"

#include "axk_radar_cfg.h"

/* About Pin:
 * if pin % 4 is 0 ===> this pin can be used as spi mosi function
 * if pin % 4 is 1 ===> this pin can be used as spi miso function
 * if pin % 4 is 2 ===> this pin can be used as spi cs   function
 * if pin % 4 is 3 ===> this pin can be used as spi sclk function
 * such as: GLB_GPIO_PIN_0 ===> mosi
 *          GLB_GPIO_PIN_1 ===> miso
 *          GLB_GPIO_PIN_2 ===> cs
 *          GLB_GPIO_PIN_3 ===> sclk
 * about cs pin: for master device, user can use hardware cs pin like pin2,and can also use software to select any pin for cs , for slave device ,user can only use hardwrae cs
 * about mosi and miso pin: mosi can be used as miso when miso can be uesd as mosi
 */

#define SPI_STACK_SIZE (128)
/* Structure that will hold the TCB of the task being created. */
StaticTask_t xTaskBuffer;
StackType_t xStack[ SPI_STACK_SIZE ];
QueueHandle_t SpiDataQueue = NULL;
TaskHandle_t SpiRecTaskHandle = NULL;

void spi_start_recv_task(void *param);
uint8_t g_dataRecvBuf[CHANNEL_MAX][DATA_RECV_BUF_SIZE];
volatile uint8_t g_dataRecvFlag[CHANNEL_MAX][DMA_RECV_FLAG_MAX];
volatile uint8_t g_frame_flag = 0;
static uint8_t sg_spi_enable = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////
#define SPI_DMA_BUFF_LEN                    (48)
// uint8_t rx_ping_buf[SPI_DMA_BUFF_LEN] __attribute__((at(0x42020000)));   // __attribute__((section(".ram_data")));
// uint8_t rx_pong_buf[SPI_DMA_BUFF_LEN] __attribute__((at(0x42021000)));   //__attribute__((section(".ram_data")));
// uint8_t rx_buf_max[SPI_DMA_BUFF_LEN*2] __attribute__((at(0x42022000)));  // __attribute__((section(".ram_data")));
uint8_t rx_ping_buf[SPI_DMA_BUFF_LEN];
uint8_t rx_pong_buf[SPI_DMA_BUFF_LEN];
uint8_t rx_buf_max[SPI_DMA_BUFF_LEN*2];
#define EVT_GROUP_SPI_TX    (1<<0)
#define EVT_GROUP_SPI_RX    (1<<1)
#define EVT_GROUP_SPI_TR    (EVT_GROUP_SPI_TX | EVT_GROUP_SPI_RX)

#define EVT_GROUP_PRINT    (1<<1)
#define EVT_GROUP_PRINT1    (1<<2)

uint8_t *pbuff;

typedef struct {
    int8_t tx_dma_ch;
    int8_t rx_dma_ch;
    uint8_t rx_enable;
    EventGroupHandle_t spi_event_group;
} spi_dma_priv_t;

DMA_LLI_PP_Struct rx_dmaLliPPStruct = {0};
EventGroupHandle_t print_event_group;
volatile static uint8_t dma_irq_cnt = 0;

static void spi_dma_handler_rx(void *arg, uint32_t flag)
{
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    hosal_spi_dev_t *dev = (hosal_spi_dev_t *)arg;
    if (NULL != dev) 
    {
        if (dev->config.dma_enable)
        {
            // printf("2");
            spi_dma_priv_t *priv=  (spi_dma_priv_t *)dev->priv;
            bl_dma_int_clear(priv->rx_dma_ch);
            // if (priv->spi_event_group != NULL) {
            //     xResult = xEventGroupSetBitsFromISR(priv->spi_event_group,
            //                                         EVT_GROUP_SPI_RX,
            //                                         &xHigherPriorityTaskWoken);
            // }
     
            // if(xResult != pdFAIL) {
            //     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            // }

            if (SpiDataQueue != NULL)
            {
                uint8_t *buff;

                if (dma_irq_cnt == 0)
                {
                    buff = rx_ping_buf;
                    dma_irq_cnt = 1;
                }
                else
                {
                    buff = rx_pong_buf;
                    dma_irq_cnt = 0;
                }

                xResult = xQueueSendFromISR( SpiDataQueue, buff, &xHigherPriorityTaskWoken );
                if (xResult != pdFAIL)
                {
                    // printf("1");
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
            else
            {
                printf("\r\nSpiDataQueue==null\r\n");
            }
        } 
    } 
    else 
    {
        printf("hosal_spi_int_handler_rx no clear isr.\r\n");
    }
}

int hosal_spi_dma_pp_recv(hosal_spi_dev_t *arg, uint16_t len, uint8_t *ping_buf, uint8_t *pong_buf, void (*dma_rx_cb)(void *arg, uint32_t flag))
{
    EventBits_t uxBits;
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DMA_LLI_Cfg_Type rxllicfg;
    DMA_LLI_Ctrl_Type *ptxlli = NULL;
    spi_dma_priv_t *dma_arg = (spi_dma_priv_t*)arg->priv;

    struct DMA_Control_Reg rx_dmaCtrlRegVal = {
        .TransferSize = len/4,
        .SBSize = DMA_BURST_SIZE_1,
        .DBSize = DMA_BURST_SIZE_1,
        .SWidth = DMA_TRNS_WIDTH_32BITS,
        .DWidth = DMA_TRNS_WIDTH_32BITS,
        .SI = DMA_MINC_DISABLE,
        .DI = DMA_MINC_ENABLE,
        .I = 1,
    };

    if (!arg) {
        printf("arg err.\r\n");
        return -1;
    }

    if (dma_arg->rx_dma_ch == -1) {
        dma_arg->rx_dma_ch = hosal_dma_chan_request(0);
        if (dma_arg->rx_dma_ch < 0) {
            printf("SPI TX DMA CHANNEL get failed!\r\n");
            return -1;
        }
    }
    
    rxllicfg.dir = DMA_TRNS_P2M;
    rxllicfg.srcPeriph = DMA_REQ_SPI_RX; 
    rxllicfg.dstPeriph = DMA_REQ_NONE;

    xEventGroupClearBits(dma_arg->spi_event_group, EVT_GROUP_SPI_TR);

    SPI_Enable(arg->port, SPI_WORK_MODE_SLAVE);

    rx_dmaLliPPStruct.dmaChan = dma_arg->rx_dma_ch;
    rx_dmaLliPPStruct.dmaCtrlRegVal = rx_dmaCtrlRegVal;
    rx_dmaLliPPStruct.DMA_LLI_Cfg = &rxllicfg;
    rx_dmaLliPPStruct.operatePeriphAddr = SPI_BASE + SPI_FIFO_RDATA_OFFSET;
    rx_dmaLliPPStruct.is_single_mode = DISABLE;
    rx_dmaLliPPStruct.chache_buf_addr[0] = (uint32_t)ping_buf;
    rx_dmaLliPPStruct.chache_buf_addr[1] = (uint32_t)pong_buf;
    
    memset(ping_buf, 0x00, len);
    memset(pong_buf, 0x00, len);

    /* dma lli pp struct steup */
    DMA_LLI_PpStruct_Init(&rx_dmaLliPPStruct);

    DMA_IntMask(rx_dmaLliPPStruct.dmaChan, DMA_INT_ALL, MASK);
    DMA_IntMask(rx_dmaLliPPStruct.dmaChan, DMA_INT_TCOMPLETED, UNMASK);
    DMA_IntMask(rx_dmaLliPPStruct.dmaChan, DMA_INT_ERR, UNMASK);

    /* Install the interrupt callback function */
    hosal_dma_irq_callback_set(dma_arg->rx_dma_ch, dma_rx_cb, arg);
    rx_dmaLliPPStruct.trans_index = 0; 
    pbuff = rx_buf_max;
    bl_irq_enable(DMA_ALL_IRQn);
    DMA_Enable();
    DMA_LLI_PpStruct_Start(&rx_dmaLliPPStruct);
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct {
    uint8_t *tx_data;
    uint8_t *rx_data;
    uint32_t length;
    uint32_t tx_index;
    uint32_t rx_index;
    EventGroupHandle_t spi_event_group;
} spi_priv_t;

hosal_spi_dev_t spi;


void queue_data_process_task(void *param)
{
    uint8_t spidatabuff[48];
    uint8_t index = 0;

    for (;;)
    {
    #if 1
        xQueueReceive(SpiDataQueue, spidatabuff, portMAX_DELAY);

    #if RAW_DATA_LOG_ENABLE
        for (size_t i = 0; i < sizeof(spidatabuff); i++)
        {
            printf("0x%02X ", spidatabuff[i]);
        }
        printf("\r\n");
    #endif

        memcpy(&g_dataRecvBuf[0][index], spidatabuff, RadarPara.dataLen);

        if (index == 0)
        {
            g_dataRecvFlag[0][0] = 1;
            index = RadarPara.dataLen;
        }
        else
        {
            g_dataRecvFlag[0][1] = 1;
            index = 0;
        }

        DataProc_Recv();
    #endif
        // printf("test\r\n");
        // vTaskDelay(500);
    }
}

void spi_slave_cfg(void)
{
    /* spi port set */
    spi.port = 0;
    /* spi master mode */
    spi.config.mode  = HOSAL_SPI_MODE_SLAVE;  
     
    spi.config.dma_enable = 1;      // 使能DMA        

    spi.config.polar_phase= 0;               
    spi.config.freq= 40000000;      // 雷达spi主机clk freq为12.5MHz，需要比它大
    spi.config.pin_clk = 11;
    /* pin cs now must be pin 2 */
    spi.config.pin_mosi= 0;
    spi.config.pin_miso= 0xff;
    /* init spi device */
    hosal_spi_init(&spi);           // XXX此处CS是修改了SDK，后续可能要自己封装一个函数

    sg_spi_enable = 1;
}

void bsp_spi_slave_init(void)
{
    spi_slave_cfg();

    memset(g_dataRecvBuf[0], 0, sizeof(g_dataRecvBuf[0]));
#if 0
    SpiRecTaskHandle = xTaskCreateStatic(
                                        spi_start_recv_task,       /* Function that implements the task. */
                                        "spi_rec_t",          /* Text name for the task. */
                                        SPI_STACK_SIZE,      /* Number of indexes in the xStack array. */
                                        NULL,    /* Parameter passed into the task. */
                                        configMAX_PRIORITIES-1,/* Priority at which the task is created. */
                                        xStack,          /* Array to use as the task's stack. */
                                        &xTaskBuffer );  /* Variable to hold the task's data structure. */
    if (SpiRecTaskHandle == NULL)
    {
        printf("spi_start_recv_task task create failedr\r\n");
    }
#endif

    int err = hosal_spi_dma_pp_recv(&spi, SPI_DMA_BUFF_LEN, rx_ping_buf, rx_pong_buf, spi_dma_handler_rx);
    if (err != 0)
    {
        printf("spi DMA init failed\r\n");
    }
}

#if 0
void __attribute__((section(".data"))) spi_start_recv_task(void *param)
{
    EventBits_t uxBits;
    spi_dma_priv_t *dma_arg = (spi_dma_priv_t*)spi.priv;
    BaseType_t xResult = pdFAIL;

    hosal_spi_dma_pp_recv(&spi, SPI_DMA_BUFF_LEN, rx_ping_buf, rx_pong_buf, spi_dma_handler_rx);

    for (;;)
    {
#if 0
    #if USE_FULL_FRAME
        hosal_spi_recv(&spi, g_total_data_buff, RadarPara.dataLen * RadarPara.chirpNum, g_spi_recv_timeout);
    #elif USE_FULL_DUAL_FRAME
        hosal_spi_recv(&spi, g_total_data_buff, RadarPara.dataLen * RadarPara.chirpNum * 2, g_spi_recv_timeout);
    #else
        hosal_spi_recv(&spi, p2g_dataRecvBuf, RadarPara.dataLen, g_spi_recv_timeout);
    #endif
#endif
        uxBits = xEventGroupWaitBits(dma_arg->spi_event_group, EVT_GROUP_SPI_TR, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & EVT_GROUP_SPI_RX)
        {
        #if 1
            if ((rx_dmaLliPPStruct.trans_index % 2) == 0)
            {
                // memcpy(pbuff, rx_ping_buf, SPI_DMA_BUFF_LEN);
                if (xQueueSend(SpiDataQueue, rx_ping_buf, ( TickType_t ) 0 ) != pdPASS)
                {
                    printf("xQueueSend failed\r\n");
                }
            }
            else
            {
                // memcpy(pbuff, rx_pong_buf, SPI_DMA_BUFF_LEN);
                if (xQueueSend(SpiDataQueue, rx_pong_buf, ( TickType_t ) 0 ) != pdPASS)
                {
                    printf("xQueueSend failed\r\n");
                }
            }
            ++rx_dmaLliPPStruct.trans_index;
            // if (rx_dmaLliPPStruct.trans_index == 1)
            // {
            //     //rx_dmaLliPPStruct.trans_index = 0;
            //     //pbuff = rx_buf_max;
            //     if (print_event_group != NULL) {
            //     xResult = xEventGroupSetBits(print_event_group, EVT_GROUP_PRINT);
            //     }
            // }
            if (rx_dmaLliPPStruct.trans_index == 2)
            {
                //DMA_Disable();
                rx_dmaLliPPStruct.trans_index = 0;
                // pbuff = rx_buf_max;
                // if (print_event_group != NULL) {
                // xResult = xEventGroupSetBits(print_event_group, EVT_GROUP_PRINT1);
                // }
            }
        #elif
            if ((rx_dmaLliPPStruct.trans_index % 2) == 0)
            {
                memcpy(pbuff, rx_ping_buf, SPI_DMA_BUFF_LEN);
            }
            else
            {
                memcpy(pbuff, rx_pong_buf, SPI_DMA_BUFF_LEN);
            }
            pbuff += SPI_DMA_BUFF_LEN;
            ++rx_dmaLliPPStruct.trans_index;
            if (rx_dmaLliPPStruct.trans_index == 1)
            {
                if (print_event_group != NULL) {
                xResult = xEventGroupSetBits(print_event_group, EVT_GROUP_PRINT);
                }
            }
            else if (rx_dmaLliPPStruct.trans_index == 2)
            {
                rx_dmaLliPPStruct.trans_index = 0;
                pbuff = rx_buf_max;
                if (print_event_group != NULL) {
                xResult = xEventGroupSetBits(print_event_group, EVT_GROUP_PRINT1);
                }
            }
        #endif
        }


    }

    vTaskDelete(NULL);
}
#endif

/**
 * @brief 直接串口打印系统任务管理器
 */
void show_task_state_task(void *param)
{
    char *pcWriteBuffer = malloc(1024);
    // int val;
    // volatile uint32_t temp;
    for (;;)
    {
        // val = (int)param;
        // printf("[task] %d\r\n", val);
        printf("\r\n=============================================\r\n");
        printf("name \t\tstatus \tprio \tfree \tpid\r\n");
        vTaskList((char *)pcWriteBuffer);
        printf("%s", pcWriteBuffer);
        // printf("---------------------------------------------\n");
        // printf("name \t\trun_count \tusage\n");
        // vTaskGetRunTimeStats((char *)pcWriteBuffer);
        // printf("%s", pcWriteBuffer);
        printf("\n=============================================\r\n");
        // temp = 1000000;
        // for (size_t i = 0; i < 1000000; i++)
        // {
        //     temp += i;
        // }
        printf("Free heap size is %d bytes\r\n", xPortGetFreeHeapSize());

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void spi_gpio_deinit(void)
{
    hosal_gpio_dev_t spi_gpio;
    spi_gpio.port = 22;     // cs
    spi_gpio.config = ANALOG_MODE;
    hosal_gpio_init(&spi_gpio);
    spi_gpio.port = 11;     // clk
    hosal_gpio_init(&spi_gpio);
    spi_gpio.port = 0;      // mosi
    hosal_gpio_init(&spi_gpio);
}

// spi disable
void bsp_spi_deinit(void)
{
    // if (SpiRecTaskHandle != NULL)
    // {
    //     vTaskDelete(SpiRecTaskHandle);
    //     SpiRecTaskHandle = NULL;
    // }

    // if (QueueTaskHandle != NULL)
    // {
    //     vTaskDelete(QueueTaskHandle);
    //     QueueTaskHandle = NULL;
    // }

    // if (SpiDataQueue != NULL)
    // {
    //     vQueueDelete(SpiDataQueue);
    //     SpiDataQueue = NULL;
    // }

    if (sg_spi_enable != 0)
    {
       hosal_spi_finalize(&spi);
       sg_spi_enable = 0;
    }

    // spi_gpio_deinit();
}


void vSpiTimeoutTimerCallback( TimerHandle_t xTimer )
{
    printf("\r\n\r\n[SPI TIMER CB] timeout, prepare to restart!!!\r\n\r\n\r\n");
    bsp_spi_deinit();
    power_off();
    vTaskDelay(pdMS_TO_TICKS(50));
    bsp_spi_slave_init();
    setSOCEnterNormalMode();
}
