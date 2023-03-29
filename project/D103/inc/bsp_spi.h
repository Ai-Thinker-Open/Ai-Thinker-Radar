
#ifndef __BSP_SPI_H__
#define __BSP_SPI_H__

#ifdef __cplusplus
 extern "C" {
#endif 

// #if defined(GD32F350)
// #include "gd32f3x0.h"
// #elif defined(GD32E230)
// #include "gd32e23x.h"
// #endif
#include "global_conf.h"
#include <hosal_spi.h>
#include <queue.h>

#define USE_EVENT_GROUP     (0)
#define DATA_PROCESS_BIT    (1 << 0)
#define CMD_PROCESS_BIT     (1 << 1)

typedef enum
{
    DMA_RECV_FLAG_MEM_0        = 0,
    DMA_RECV_FLAG_MEM_1        = 1,  //XXX改为1缓存
    DMA_RECV_FLAG_MAX
}dmaRecvFlagEnum;

#define SPI_FRAME_DLEN_MAX      RADAR_DATA_MAX_LEN
#define SPI_FRAME_HLEN          (4)
#define SPI_FRAME_TLEN          (4)
#define SPI_FRAME_LEN_MAX       (SPI_FRAME_DLEN_MAX + SPI_FRAME_HLEN + SPI_FRAME_TLEN)

#define DATA_RECV_BUF_SIZE           (SPI_FRAME_LEN_MAX * 2) /*ping-pong buffer*/

#define USE_FULL_FRAME          (0) // 一次接收全部数据使能

#define USE_FULL_DUAL_FRAME     (1) // 一次接收双倍缓存

extern uint8_t g_dataRecvBuf[CHANNEL_MAX][DATA_RECV_BUF_SIZE];
extern volatile uint8_t g_dataRecvFlag[CHANNEL_MAX][DMA_RECV_FLAG_MAX];
extern volatile uint8_t g_frame_flag;
extern uint8_t g_total_data_buff[];

extern QueueHandle_t SpiDataQueue;

void bsp_spi_slave_init(void);
void bsp_spi_deinit(void);
void show_task_state_task(void *param);
void vSpiTimeoutTimerCallback( TimerHandle_t xTimer );

#ifdef __cplusplus
}
#endif

#endif

