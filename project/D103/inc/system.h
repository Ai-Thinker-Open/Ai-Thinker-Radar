/**
  ******************************************************************************
  * @file           : system.h
  * @author         : iclm team
  * @brief          : system header file
  ******************************************************************************
  */
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
 extern "C" {
#endif 

typedef enum  
{
	SYS_SYSTEM_MODE = 0,
    SYS_UPLOAD_SP_RATE,
    SYS_DEBUG_MODE
}sysParaEnum;

typedef struct SYS_PARA
{
    int16_t systemMode;
    int16_t uploadSampleRate;
    int16_t debugMode;
    int16_t pad;
}SYS_PARA_T;

uint16_t System_GetSysMode(void);
uint16_t System_GetUploadSampleRate(void);
uint16_t System_GetDebugMode(void);
void* System_GetSysParaAddr(void);
uint32_t System_GetSysParaLen(void);
void System_ParaInit(void);
int8_t System_ParaUpdate(uint16_t type, int32_t val);
int32_t System_ParaRead(uint16_t type);
void System_Reset(void);
void System_Reconfig(void);

#ifdef __cplusplus
}
#endif

#endif


