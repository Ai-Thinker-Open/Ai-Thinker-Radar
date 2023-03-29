/**
  ******************************************************************************
  * @file           : banyan.h
  * @author         : iclm team
  * @brief          : banyan driver header file
  ******************************************************************************
  */
#ifndef __BANYAN_H__
#define __BANYAN_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>

typedef enum  
{
	RAW_POINT_64 = 64,
    RAW_POINT_128 = 128,
    RAW_POINT_256 = 256,
    RAW_POINT_512 = 512,
    RAW_POINT_1024 = 1024,
}rawPointEnum;

typedef struct RADAR_REG
{
    uint16_t addr;
    uint16_t val;
}RADAR_REG_T;

#define I2C_ADDR_BanYan_Chip0		    (0x40)
#define I2C_ADDR_BanYan_Chip1		    (0x44)
#define MAX_REG_NUM                 (100)
#define REG_FIX_NUM                 (5)
#define RAW_MAP_NUM                 (5)

#define BANYAN_DIG_FUN_SWITCH       (0x01)
#define BANYAN_DIG_RAW_PEAK_NUM     (0x04)
#define BANYAN_DIG_FFT_NUM          (0x05)
#define BANYAN_DIG_FFT_SETTING      (0x0A)
#define BANYAN_DIG_DFFT_DATA_NUM    (0x0D)
#define BANYAN_DIG_DFFT_CHIRP_NUM   (0x0E)
#define BANYAN_PAT_CHIRP_NUM        (0x44)

#define BANYAN_DFFT_DATA_NUM_POS    (8)
#define BANYAN_DFFT_CHIRP_NUM_POS   (8)
#define BANYAN_RAW_POS              (8)

#define BANYAN_PEAK_MASK            (0x1F)
#define BANYAN_RAW_MASK             (0x07)

#define BANYAN_FFT_DATA             BIT(2)
#define BANYAN_DFFT_DATA            BIT(12)
#define BANYAN_DFFT_PEAK_DATA       BIT(4)
#define BANYAN_DSRAW_DATA           BIT(1)

extern uint8_t g_ChannelCount;
extern uint8_t g_TxCount;
extern uint8_t g_maxChannelCount;

void Radar_Init(void);
void WriteSocAllReg(void);
uint16_t Radar_GetFftPoint(void);
uint16_t Radar_GetRawPoint(void);
uint16_t Radar_GetOneFrameChirpNum(void);
uint8_t Radar_GetDataType(void);
uint16_t Radar_GetDfftDataNum(void);
uint16_t Radar_GetDfftChirpNum(void);
uint16_t Radar_GetDfftPeakSize(void);
void Radar_Enter_LowPWR(uint16_t devAddr);
void Radar_Exit_LowPWR(uint16_t devAddr);

void Radar_UpdateReg(uint16_t addr, uint16_t val);
void* Radar_GetRadarParaAddr(void);
uint32_t Radar_GetRadarParaLen(void);

//Algorithm param
void Algo_SaveParameter(uint32_t* data,uint16_t len);
void Algo_ReadParameter(uint32_t* buf, uint32_t bufLen);
uint32_t Algo_ReadParameterExistFlag(void);

#if defined(SUPPORT_DYNAMIC_SYS_MODE)
void* Radar_GetRegListMTTAddr(void);
uint32_t Radar_GetRegListMTTLen(void);
void* Radar_GetRegListVSAddr(void);
uint32_t Radar_GetRegListVSLen(void);
void* Radar_GetRegListGRAddr(void);
uint32_t Radar_GetRegListGRLen(void);
#endif	
#ifdef __cplusplus
}
#endif

#endif


