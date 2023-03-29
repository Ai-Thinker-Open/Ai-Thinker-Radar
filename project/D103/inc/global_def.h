/**
  ******************************************************************************
  * @file    global_def.h
  * @author  iclm team
  * @brief   global define for whole project
  ******************************************************************************
  */
#ifndef __GLOBAL_DEF_H__
#define __GLOBAL_DEF_H__

#ifdef __cplusplus
 extern "C" {
#endif

#define XenD103     //自己添加的

#define RADAR_PROTOCOL_VERSION	  2

#define RADAR_DEV_FIRMWARE_TYPE   0
#ifdef EVBSN01
#define RADAR_DEV_MODEL_TYPE_SN	  (0x8800)
#define RADAR_DEV_TYPE (0x8000)
#endif
#if defined(DEVMB) || defined(DEVMB_PLUS)
#define RADAR_DEV_MODEL_TYPE_SN	  (0x8801)
#define RADAR_DEV_TYPE (0x8001)
#endif
#ifdef ISK1101
#define RADAR_DEV_MODEL_TYPE_SN	  (0x8802)
#define RADAR_DEV_TYPE (0x8002)
#endif
#ifdef XenD103
#define RADAR_DEV_MODEL_TYPE_SN	  (0x8802)
#define RADAR_DEV_TYPE (0x8002)
#endif

typedef enum
{
    CHANNEL_0        = 0, /*LNA1-TSPI_DA0*/
#ifdef STM32_PLATFORM
    CHANNEL_1        = 1, /*LNA2-TSPI_DA1*/
#endif
#if defined(DEVMB) || defined(DEVMB_PLUS)
    CHANNEL_2        = 2,
	CHANNEL_3        = 3,
#endif
    CHANNEL_MAX
}channelEnum;

typedef enum
{
    DATA_TYPE_FFT         = 0,
    DATA_TYPE_DFFT        = 1,
    DATA_TYPE_DFFT_PEAK   = 2,
	DATA_TYPE_DSRAW		  = 3,
    DATA_TYPE_MAX
}dataTypeEnum;

typedef enum
{
    SYS_MODE_PASSTHROUGH  = 0,
    SYS_MODE_MTT          = 1, /*motion target tracking, please copy XXX_MTT.txt to XXX.txt*/
    SYS_MODE_VS           = 2, /*vital sign, please copy XXX_VS.txt to XXX.txt*/
	SYS_MODE_GR           = 3, /*Gesture Recognition, please copy XXX_GR.txt to XXX.txt*/
    SYS_MODE_MAX
}sysModeEnum;

#define DEBUG_MODE_ON     1
#define DEBUG_MODE_OFF    0

#ifdef __cplusplus
}
#endif

#endif


