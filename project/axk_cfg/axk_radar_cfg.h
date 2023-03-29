#ifndef _AXK_RADAR_CFG_H_
#define _AXK_RADAR_CFG_H_

#define FUNC_QUEUE_SIZE                 (34)    // n*48 bytes   chirpNum: 34, dataLen: 48
#define FT_QUEUE_SIZE                   (512)   // n*40 bytes   chirpNum: 64, dataLen: 40

#define RAW_DATA_LOG_ENABLE             (0)     // show raw data
#define COMPLETE_DATA_LOG_ENABLE        (0)     // When data processing is complete, a prompt is displayed
#define RADAR_WORK_MODE_LOG_ENABLE      (0)     // show radar operating mode

#define UART_LOG_BAUDRATE               (2000000)


// #define DATA_PROCESS_TASK_PRIORITY      (27)
// #define CMD_PROCESS_TASK_PRIORITY       (26)
// #define UART_RECV_CHECK_TASK_PRIORITY   (25)

#define DATA_PROCESS_TASK_PRIORITY      (14)
#define CMD_PROCESS_TASK_PRIORITY       (13)
#define UART_RECV_CHECK_TASK_PRIORITY   (12)

#endif
