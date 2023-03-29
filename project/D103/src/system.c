/**
  ******************************************************************************
  * @file           : system.c
  * @author         : iclm team
  * @brief          : system module
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>
#include "global_conf.h"
#include "utilities.h"
#include "system.h"
#include "platform.h"
#include "dataprocess.h"
#include "banyan.h"

#include "bsp_spi.h"

static SYS_PARA_T sysPara __ALIGN(4);

void* System_GetSysParaAddr(void)
{
    return (void*)&sysPara;
}

uint32_t System_GetSysParaLen(void)
{
    return sizeof(sysPara);
}

uint16_t System_GetSysMode(void)
{
    return sysPara.systemMode;
}

uint16_t System_GetUploadSampleRate(void)
{
    return sysPara.uploadSampleRate;
}

uint16_t System_GetDebugMode(void)
{
    return sysPara.debugMode;
}

void System_ParaInit(void)
{
    sysPara.uploadSampleRate = UPLOAD_SAMPLE_RATE;
    sysPara.debugMode = DEBUG_MODE_DEFAULT;
}

int8_t System_ParaUpdate(uint16_t type, int32_t val)
{
    switch (type)  
    {
#ifdef SUPPORT_DYNAMIC_SYS_MODE
        case SYS_SYSTEM_MODE:
            sysPara.systemMode = (int16_t)val;
            break;
#endif
			
#if defined(SUPPORT_DATA_PASSTHROUGH) || defined(SUPPORT_DYNAMIC_SYS_MODE)
        case SYS_UPLOAD_SP_RATE:
            sysPara.uploadSampleRate = (int16_t)val;
            DataProc_ResetRecvCnt();
            break;
#endif
		
       case SYS_DEBUG_MODE:
            sysPara.debugMode = (int16_t)val;
            break;
        
        default:
            return -1;
    }

    return 0;
}

int32_t System_ParaRead(uint16_t type)
{
    switch (type)  
    {
        case SYS_SYSTEM_MODE:
            return sysPara.systemMode;

        case SYS_UPLOAD_SP_RATE:
            return sysPara.uploadSampleRate;

       case SYS_DEBUG_MODE:
            return sysPara.debugMode;

        default:
            return 0x7fffffff; /*invalid value*/
    }
}

void System_Reset(void)
{
	// SPI_DeInit();
    bsp_spi_deinit();
#ifdef STM32_PLATFORM
	UsbTransfer_ResetUsbBuffer();
	DataProc_ResetRadarDataQueue();
#endif	
}

void System_Reconfig(void)
{
    bsp_spi_deinit();
    vTaskDelay(pdMS_TO_TICKS(1));

	DataProc_NeedReconfig();

    bsp_spi_slave_init();
}


