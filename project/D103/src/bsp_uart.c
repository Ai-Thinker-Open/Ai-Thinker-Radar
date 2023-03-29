#include <hosal_uart.h>
#include <bl_uart.h>
#include "bsp_uart.h"

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
// #include "gd32_uart.h"
#include "global_conf.h"
// #include "gd32_exit.h"
#include "mcu_sleep.h"
#include "body_presence.h"
#include "ICLM_KS5_LowPower.h"
#include <bl_irq.h>
#include "bsp_spi.h"
#include "axk_radar_cfg.h"

#define UART_TRAN_NUM   (1)

// event group
extern EventGroupHandle_t CmdProcessEventGroup;

CMD_RECV_T g_cmdRecv;
UART_CMF_RECV_EXTI_T g_uartCmdRecvExit;

volatile uint8_t g_uartDmaSendingFlag = 0;

static uint8_t get_cmd_head_flag = 0;

// 串口结构体初始化
HOSAL_UART_DEV_DECL(uart_dev_int, UART_TRAN_NUM, 16, 7, 256000);

HOSAL_UART_DEV_DECL(uart_dev0, 0, 4, 0xff, UART_LOG_BAUDRATE);


void uart_dma_send(uint8_t *buff, uint16_t len)
{
#if 1
    hosal_uart_dma_cfg_t txdam_cfg = {
        .dma_buf = buff,
        .dma_buf_size = len,
    };

    /* Start a UART TX DMA transfer */
    hosal_uart_ioctl(&uart_dev_int, HOSAL_UART_DMA_TX_START, &txdam_cfg);
#else
    hosal_uart_send(&uart_dev_int, buff, len);
    g_uartDmaSendingFlag = 0;
#endif
}

/**
 * 中断回调
 */
static void __uart_rx_callback(void *p_arg)
{
	int cnt;
    hosal_uart_dev_t *p_dev = (hosal_uart_dev_t *)p_arg;

    cnt = hosal_uart_receive(p_dev, &g_cmdRecv.buf[g_cmdRecv.bufRecv][g_cmdRecv.curIndex], CMD_RECV_BUF_SIZE);

    g_cmdRecv.curIndex += cnt;
    g_cmdRecv.idleCnt = 0;
    if (g_cmdRecv.curIndex >= CMD_RECV_BUF_SIZE)
    {
        g_cmdRecv.cmdReady = 1;
        g_cmdRecv.bufProc = g_cmdRecv.bufRecv;
        g_cmdRecv.bufRecv = (++g_cmdRecv.bufRecv) % BUF_MAX;
        g_cmdRecv.bufLen = g_cmdRecv.curIndex;
        g_cmdRecv.curIndex = 0;
		setUartRecv_Flag();


        BaseType_t xHigherPriorityTaskWoken, xResult;

        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(CmdProcessEventGroup, CMD_PROCESS_BIT, &xHigherPriorityTaskWoken );

        /* Was the message posted successfully? */
        if( xResult != pdFAIL )
        {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}

/**
 * 串口DMA发送完成回调
 */
static int __uart_tx_dma_callback(void *p_arg)
{
    /* If TX transmission is completed */
    g_uartDmaSendingFlag = 0;
    return 0;
}

// log口
void bsp_uart0_init(void)
{
    hosal_uart_init(&uart_dev0);
}

// 通讯口
void bsp_uart_init(int uart_id, int baudrate)
{
    // uint8_t tempbuf[] = "radar test\r\n";
    // hosal_uart_dma_cfg_t txdma_cfg = {
    //     .dma_buf = tempbuf,
    //     .dma_buf_size = sizeof(tempbuf) - 1,
    // };

    uart_dev_int.config.uart_id = uart_id;
    uart_dev_int.config.baud_rate = baudrate;
    /* Uart init device */
    hosal_uart_init(&uart_dev_int);

    /* Set DMA TX RX transmission complete interrupt callback */
    hosal_uart_callback_set(&uart_dev_int, HOSAL_UART_TX_DMA_CALLBACK,
                          __uart_tx_dma_callback, &uart_dev_int);

    bl_uart_int_rx_notify_register(uart_dev_int.port, __uart_rx_callback, &uart_dev_int);

    // hosal_uart_ioctl(&uart_dev_int, HOSAL_UART_DMA_TX_START, &txdma_cfg);

    bl_uart_int_rx_enable(uart_dev_int.port);
    bl_irq_register(UART1_IRQn, UART1_IRQHandler);
    bl_irq_enable(UART1_IRQn);
}


int HAL_at_uart_send(char *buf, uint16_t size)
{
    uint16_t cnt = 0;

    while (cnt < size) {
        bl_uart_data_send(1, ((uint8_t*)buf)[cnt]);
        cnt++;
    }

    return 0;
}


void axk_printf(char *fmt, ...)
{
    char string[256] = {0};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(string, 256, fmt, ap);
    va_end(ap);
    HAL_at_uart_send(string,strlen(string));
}

void UART_RecvTimeout(void)
{
    if (g_cmdRecv.curIndex > 0)
    {
        g_cmdRecv.idleCnt++;
        if (g_cmdRecv.idleCnt >= CMD_RECV_TIMEOUT)
        {
            g_cmdRecv.cmdReady = 1;
            g_cmdRecv.bufProc = g_cmdRecv.bufRecv;
            g_cmdRecv.bufRecv = (++g_cmdRecv.bufRecv) % BUF_MAX;
            g_cmdRecv.bufLen = g_cmdRecv.curIndex;
            g_cmdRecv.curIndex = 0;
            g_cmdRecv.idleCnt = 0;
			setUartRecv_Flag();

            xEventGroupSetBits(CmdProcessEventGroup, CMD_PROCESS_BIT);
        }
    }
}

void uart_recv_timeout_check_task(void *param)
{
    for (;;)
    {
        UART_RecvTimeout();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void clearCmdHeadFlag(void)
{
    get_cmd_head_flag = 0;
}

void setCmdHeadFlag(void)
{
    get_cmd_head_flag = 1;
}

uint8_t getCmdHeadFlag(void)
{
    return get_cmd_head_flag;
}












