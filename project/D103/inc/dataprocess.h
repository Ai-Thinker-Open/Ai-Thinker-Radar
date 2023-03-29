/**
  ******************************************************************************
  * @file           : dataprocess.h
  * @author         : iclm team
  * @brief          : data process header file
  ******************************************************************************
  */
#ifndef __DATAPROCESS_H__
#define __DATAPROCESS_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include "platform.h"

typedef enum  
{
	DATA_STATE_HEAD = 0,
    DATA_STATE_ID,
    DATA_STATE_INDEX1,
    DATA_STATE_INDEX2,
    DATA_STATE_DATA,
    DATA_STATE_TAIL0,
    DATA_STATE_TAIL1,
    DATA_STATE_TAIL2,
    DATA_STATE_TAIL3
}dataStateEnum;

typedef enum
{
    FT_MODE_ENABLE  = 0,
    FT_MODE_RUNNING = 1,
    FT_MODE_DISALBE = 2,
    NO_FT_MODE      = 3,
    ERROR_FT_MODE
}FactoryTestStatus_t;

#define DATA_HEAD		          (0xAA)
#define DATA_TAIL		          (0x55)
#define ID_MASK                   (0xF0)

#define FFT0_ID                   (0x30)
#define FFT1_ID                   (0x70)

#define DSRAW0_ID                   (0x20)
#define DSRAW1_ID                   (0x60)

#define CHIRP_INDEX_POS0          (5)
#define CHIRP_INDEX_POS1          (3)
#define CHIRP_INDEX_MASK          (0x0F)
#define SKIP_NUM                  (4)

#define DFFT0_ID                  (0xB0)
#define DFFT1_ID                  (0xF0)
#define FRAME_CNT_POS             (8)

#define DFFT_PEAK_ID              (0x40)

#define DATA_PROC_STACK_SIZE      (512)
#define RADAR_DATA_QUEUE_SIZE     (2)

#define BUF_LEN     (264)

typedef struct RADAR_DATA_PARSE
{
    uint8_t  buf[BUF_LEN];
    uint8_t  state;
    uint8_t  channelId;
    uint8_t  chirpIndex;
    uint16_t frameCnt;
    uint16_t curIndex;
    uint16_t needCopyLen;
    uint16_t dfft_peak_data_len;
}RADAR_DATA_PARSE_T;

typedef struct RADAR_PARA
{
    uint8_t dataType;
    uint16_t dataLen;
    uint16_t chirpNum;
}RADAR_PARA_T;

extern RADAR_PARA_T RadarPara;
extern RADAR_DATA_PARSE_T RadarDataParse[1];

void DataProc_Recv(void);
void DataProc_Init(void);
uint8_t DataProc_GetRadarDataType(void);
uint16_t DataProc_GetRadarDataLen(void);
uint8_t DataProc_NeedReconfig(void);
void DataProc_ResetRecvCnt(void);

uint8_t isFrameLastChirpDataFinished(void);
void setFrameLsatChirpDataFinished(void);
void clearFrameLsatChirpDataFinished(void);
uint8_t CmdProc_IsInDebugMode(void);
uint8_t GetLowPowerType(void);

void DataProcess(uint8_t channel, uint8_t dmaFlag, uint8_t *recvBuf, uint16_t bufLen);

#ifdef STM32_PLATFORM
void DataProc_TaskInit(void);
void DataProc_Send2RadarDataQueue(uint8_t channel, void *radarData);
void DataProc_ResetRadarDataQueue(void);
#endif

#ifdef __cplusplus
}
#endif

#endif


