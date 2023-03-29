#include "ft_bsp_spi.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hosal_spi.h"
#include "bl_dma.h"
#include "blog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "axk_radar_cfg.h"
#include "dataprocess.h"
#include "bsp_spi.h"

#define FT_DATA_LEN     (40)

extern uint8_t g_dataRecvBuf[1][96];
extern hosal_spi_dev_t spi;
void spi_slave_cfg(void);
int hosal_spi_dma_pp_recv(hosal_spi_dev_t *arg, uint16_t len, uint8_t *ping_buf, uint8_t *pong_buf, void (*dma_rx_cb)(void *arg, uint32_t flag));
void DataProc_Recv(void);

uint8_t ft_ping_buff[40];
uint8_t ft_pong_buff[40];
volatile static uint8_t dma_irq_cnt = 0;
QueueHandle_t ft_queue_handle = NULL;
TaskHandle_t ft_queue_task_handle = NULL;

typedef struct {
    int8_t tx_dma_ch;
    int8_t rx_dma_ch;
    uint8_t rx_enable;
    EventGroupHandle_t spi_event_group;
} spi_dma_priv_t;

static void ft_spi_dma_handler_rx(void *arg, uint32_t flag)
{
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    hosal_spi_dev_t *dev = (hosal_spi_dev_t *)arg;
    if (NULL != dev) {
        if (dev->config.dma_enable) {
            spi_dma_priv_t *priv=  (spi_dma_priv_t *)dev->priv;
            bl_dma_int_clear(priv->rx_dma_ch);

            if (ft_queue_handle != NULL)
            {
                uint8_t *buff;

                if (dma_irq_cnt == 0)
                {
                    buff = ft_ping_buff;
                    dma_irq_cnt = 1;
                }
                else
                {
                    buff = ft_pong_buff;
                    dma_irq_cnt = 0;
                }

                xResult = xQueueSendFromISR( ft_queue_handle, buff, &xHigherPriorityTaskWoken );
                if (xResult != pdFAIL)
                {
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
            else
            {
                printf("\r\nft_queue_handle==null\r\n");
            }
        }
    } else {
        blog_error("hosal_spi_int_handler_rx no clear isr.\r\n");
    }
}

void ft_queue_data_process_task(void *param)
{
    uint8_t spidatabuff[40];
    uint8_t index = 0;

    for (;;)
    {
        xQueueReceive(ft_queue_handle, spidatabuff, portMAX_DELAY);

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
    }
}

int ft_create_queue_and_task(void)
{
    if (ft_queue_handle == NULL)
    {
        ft_queue_handle = xQueueCreate(FT_QUEUE_SIZE, FT_DATA_LEN);
        if (ft_queue_handle == NULL)
        {
            printf("creat ft queue failed\r\n");
            return -1;
        }
    }
    else
    {
        printf("reset ft queue\r\n");
        xQueueReset(ft_queue_handle);
    }
    
    if (ft_queue_task_handle == NULL)
    {
        BaseType_t err = xTaskCreate(ft_queue_data_process_task, "ft_queue_task", 512, NULL, DATA_PROCESS_TASK_PRIORITY, &ft_queue_task_handle);
        if (err != pdPASS)
        {
            printf("create ft_queue_data_process_task failed\r\n");
            return -1;
        }
    }

    return 0;
}

void ft_bsp_spi_slave_init(void)
{
    spi_slave_cfg();

    memset(g_dataRecvBuf[0], 0x00, sizeof(g_dataRecvBuf[0]));

    hosal_spi_dma_pp_recv(&spi, FT_DATA_LEN, ft_ping_buff, ft_pong_buff, ft_spi_dma_handler_rx);
}