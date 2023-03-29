/**
  ******************************************************************************
  * @file           : config.h
  * @author         : ting.gao@iclegend.com
  * @brief          : config header file
  ******************************************************************************
  */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include "global_conf.h"
#include "platform.h"
#include "bsp_flash.h"

#define FLASH_PAGE_SIZE         FMC_PAGE_SIZE
#define FLASH_DATA_PAGE_ADDR    FMC_SW_SETTING_ADDR
#define FLASH_SN_ADDR           FMC_HW_SETTING_ADDR

#define FLASH_MAGIC_NUM         (0x12345678)
#define FLASH_WORD_LEN          (4)

/* bit operations */
#define REG32(addr)                  (*(volatile uint32_t *)(uint32_t)(addr))
#define REG16(addr)                  (*(volatile uint16_t *)(uint32_t)(addr))
#define REG8(addr)                   (*(volatile uint8_t *)(uint32_t)(addr))
#define BIT(x)                       ((uint32_t)((uint32_t)0x01U<<(x)))
#define BITS(start, end)             ((0xFFFFFFFFUL << (start)) & (0xFFFFFFFFUL >> (31U - (uint32_t)(end)))) 
#define GET_BITS(regval, start, end) (((regval) & BITS((start),(end))) >> (start))


typedef enum  
{
	FLASH_ELEM_RADAR = 0,
    FLASH_ELEM_SYS,
#if defined(SUPPORT_MOTION_TARGET_TRACKING) || defined(SUPPORT_DYNAMIC_SYS_MODE)
    FLASH_ELEM_MTT,
#endif
    FLASH_ELEM_MAX
}flashElemEnum;

typedef struct FLASH_ELEMENT
{
    uint32_t elemLen;
    void *elemAddr; 
}FLASH_ELEMENT_T;

#pragma pack(4)
typedef struct FLASH_DATA
{
    uint32_t magic;
    FLASH_ELEMENT_T elem[FLASH_ELEM_MAX];
}FLASH_DATA_T;
#pragma pack()

void Config_ReloadAlgoRegister(uint16_t systemMode);
void Config_WriteData2Flash(void);
void Config_RetrieveFlashData(void);
void Config_EarseFlashData(void);
void Config_NeedFlashWrite(void);
void Config_SavePara2Flash(void);
uint32_t Config_GetSN(void);
void Config_Init(void);
void Config_WritebodysensingFlash(uint32_t* addr, uint16_t len);
uint32_t Config_ReadAlgorithmParamLen(void);
uint8_t getFTMode(void);

#ifdef __cplusplus
}
#endif

#endif


