/**
  ******************************************************************************
  * @file           : config.c
  * @author         : ting.gao@iclegend.com
  * @brief          : config module
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "utilities.h"
#include "banyan.h"
#include "dataprocess.h"
#include "system.h"

#include "bsp_nvs.h"

static FLASH_DATA_T flashData;
static uint8_t needFlashWrite = 0;

void Config_ReloadAlgoRegister(uint16_t systemMode)
{
#ifdef SUPPORT_DYNAMIC_SYS_MODE
	uint8_t reload = 1;
    uint16_t loop = 0;
    uint16_t readNum = 0;
    uint32_t *readAddr = NULL;
    uint32_t flashAddr = FLASH_DATA_PAGE_ADDR;
    switch(systemMode)
    {
        case SYS_MODE_MTT:
			flashAddr = (uint32_t)Radar_GetRegListMTTAddr();
			readNum = Radar_GetRegListMTTLen()/FLASH_WORD_LEN;
            break;
            
        case SYS_MODE_VS:
			flashAddr = (uint32_t)Radar_GetRegListVSAddr();
			readNum = Radar_GetRegListVSLen()/FLASH_WORD_LEN;
            break;

		case SYS_MODE_GR:
			flashAddr = (uint32_t)Radar_GetRegListGRAddr();
			readNum = Radar_GetRegListGRLen()/FLASH_WORD_LEN;
            break;
			
        default:
			reload = 0;
            break;
    }
	
	if(reload)
	{
		if(Radar_GetRadarParaLen() < readNum)
		{
			return;
		}
		
		readAddr = (uint32_t *)Radar_GetRadarParaAddr();
		for (loop = 0; loop < readNum; loop++)
		{
			readAddr[loop] = REG32(flashAddr);
			flashAddr += FLASH_WORD_LEN;
		}		
	}
#endif	
}

void Config_WriteData2Flash(void)
{
    uint16_t idx = 0;
    int err;

    flashData.magic = FLASH_MAGIC_NUM;

    err = bsp_nv_write(elem_key.magic, (char *)&flashData.magic, sizeof(flashData.magic));
    if (err != EF_NO_ERR)
    {
        printf("save magic failed\r\n");
    }

    for (idx = 0; idx < FLASH_ELEM_MAX; idx++)
    {
        bsp_nv_write(elem_key.elem_len[idx], (char *)&flashData.elem[idx].elemLen, sizeof(flashData.elem[idx].elemLen));
        bsp_nv_write(elem_key.elem_member[idx], (char *)flashData.elem[idx].elemAddr, flashData.elem[idx].elemLen/FLASH_WORD_LEN);
    }
}

void Config_RetrieveFlashData(void)
{
    uint16_t idx = 0;

    bsp_nv_read(elem_key.magic, (char *)&flashData.magic, sizeof(flashData.magic));
    printf("flashData.magic=%#x\r\n", flashData.magic);
    if (FLASH_MAGIC_NUM != flashData.magic)
    {
        printf("flash setting is empty!\r\n");
        Config_WriteData2Flash();               // 往flash写入数据
    }

    flashData.magic = 0;
    bsp_nv_read(elem_key.magic, (char *)&flashData.magic, sizeof(flashData.magic));
    if (FLASH_MAGIC_NUM != flashData.magic) // 写入失败擦除参数，死循环
    {
        printf("Error: flash work abnormal!\r\n");
    }
    
    for (idx = 0; idx < FLASH_ELEM_MAX; idx++)
    {
        bsp_nv_read(elem_key.elem_len[idx], (char *)&flashData.elem[idx].elemLen, sizeof(flashData.elem[idx].elemLen));
        printf("flashData.elem[%d].elemLen=%d\r\n", idx, flashData.elem[idx].elemLen);
        printf("flashData.elem[%d].elemAddr=%p\r\n", idx, flashData.elem[idx].elemAddr);
        bsp_nv_read(elem_key.elem_member[idx], (char *)flashData.elem[idx].elemAddr, flashData.elem[idx].elemLen/FLASH_WORD_LEN);
    }
	Config_ReloadAlgoRegister(System_GetSysMode());             
}

void Config_SavePara2Flash(void)
{    
    if (!needFlashWrite)
    {
        return;
    }

    Config_WriteData2Flash();
    needFlashWrite = 0;
}


void Config_EarseFlashData(void)
{    
    // Flash_ErasePage(FLASH_DATA_PAGE_ADDR);   //TODO暂时注释
}

void Config_NeedFlashWrite(void)
{
    needFlashWrite = 1;
}

uint32_t Config_GetSN(void)
{
#if 0
    uint32_t flashAddr = FLASH_SN_ADDR;

#ifdef DEVMB
    flashAddr += QSPI_MEM_START;
#endif
    return REG32(flashAddr);
#endif
    return 0xffffffff;
}

void Config_Init(void)
{
    uint16_t totalLen = 0;

    totalLen += sizeof(flashData.magic);
    
    flashData.elem[FLASH_ELEM_RADAR].elemAddr = Radar_GetRadarParaAddr();   // 获取雷达参数结构体数组地址
    flashData.elem[FLASH_ELEM_RADAR].elemLen = Radar_GetRadarParaLen();     // 获取结构体数组大小
    totalLen += flashData.elem[FLASH_ELEM_RADAR].elemLen + sizeof(flashData.elem[FLASH_ELEM_RADAR].elemLen);
    printf("flashData.elem[FLASH_ELEM_RADAR].elemAddr=%p\r\n", flashData.elem[FLASH_ELEM_RADAR].elemAddr);
    printf("flashData.elem[FLASH_ELEM_RADAR].elemLen=%d\r\n", flashData.elem[FLASH_ELEM_RADAR].elemLen);

    System_ParaInit();
    flashData.elem[FLASH_ELEM_SYS].elemAddr = System_GetSysParaAddr(); 
    flashData.elem[FLASH_ELEM_SYS].elemLen = System_GetSysParaLen();
    totalLen += flashData.elem[FLASH_ELEM_SYS].elemLen + sizeof(flashData.elem[FLASH_ELEM_SYS].elemLen);
    printf("flashData.elem[FLASH_ELEM_SYS].elemAddr=%p\r\n", flashData.elem[FLASH_ELEM_SYS].elemAddr);
    printf("flashData.elem[FLASH_ELEM_SYS].elemLen=%d\r\n", flashData.elem[FLASH_ELEM_SYS].elemLen);

    if (totalLen > FLASH_PAGE_SIZE)     // 1K
    {
        printf("Error: flashDataLen is more than FLASH_PAGE_SIZE!\r\n");
    }
	Config_RetrieveFlashData();         // flash有参数则读取参数，无参数则写入参数
}

void Config_WritebodysensingFlash(uint32_t* addr, uint16_t len)
{
    uint32_t dataLen = len;

    bsp_nv_write(elem_key.bodysensing, (char *)&dataLen, sizeof(dataLen));
    bsp_nv_write(elem_key.parameter, (char *)addr, len);
}

uint32_t Config_ReadAlgorithmParamLen(void)
{
    int ret;
    uint32_t dataLen = 0xffffffff;

    ret = bsp_nv_read(elem_key.bodysensing, (char *)&dataLen, sizeof(dataLen));
    if (ret != 0)
    {
        dataLen = 0xffffffff;
        printf("read dataLen failed\r\n");
    }
    
    return dataLen;   
}

uint8_t getFTMode(void)
{
    extern uint8_t g_FtcmdFlag;
    return g_FtcmdFlag;
}

