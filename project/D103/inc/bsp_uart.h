#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#ifdef __cplusplus
 extern "C" {
#endif 

// #if defined(GD32F350)
// #include "gd32f3x0.h"
// #elif defined(GD32E230)
// #include "gd32e23x.h"
// #endif

//#define CONFIG_SUPPORT_UART1_DEBUG

#define USART0_BAUDRATE                    (256000U)
// #define USART0_DMA_CHANEL                  DMA_CH3
//#define USART1_BAUDRATE                    (256000U)

#define CMD_RECV_BUF_SIZE                  (64)
#define CMD_RECV_TIMEOUT                   (5) /*ms*/

typedef enum
{
    BUF_0        = 0,
    BUF_1        = 1,
    BUF_MAX
}BufEnum;

typedef struct CMD_RECV
{
    uint8_t buf[BUF_MAX][CMD_RECV_BUF_SIZE];
    volatile uint8_t bufRecv;
    volatile uint8_t bufProc;
    volatile uint8_t cmdReady;
    volatile uint8_t idleCnt;
    volatile uint16_t curIndex;
    volatile uint16_t bufLen;
}CMD_RECV_T;

typedef struct
{
    uint8_t buf[CMD_RECV_BUF_SIZE];
    uint8_t curIndex;
    uint8_t idleCnt;
    uint8_t cmdReady;
    uint8_t bufLen;
}UART_CMF_RECV_EXTI_T;

extern CMD_RECV_T g_cmdRecv;
extern UART_CMF_RECV_EXTI_T g_uartCmdRecvExit;
extern volatile uint8_t g_uartDmaSendingFlag;

// void UART_Init(void);
void UART0_DmaSend(uint8_t* buf, uint16_t size);
void UART_RecvTimeout(void);
void UART_Recv(void);

void clearCmdHeadFlag(void);
void setCmdHeadFlag(void);
uint8_t getCmdHeadFlag(void);
void Usart0RxPinInit(void);


void bsp_uart0_init(void);
void bsp_uart_init(int uart_id, int baudrate);
void axk_printf(char *fmt, ...);
void uart_dma_send(uint8_t *buff, uint16_t len);
void uart_recv_timeout_check_task(void *param);

#ifdef __cplusplus
}
#endif



#endif
