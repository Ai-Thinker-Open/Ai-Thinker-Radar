/**
  ******************************************************************************
  * @file           : banyan.c
  * @author         : iclm team
  * @brief          : banyan driver
  ******************************************************************************
  */
#include <stdio.h>
#include "global_conf.h"
#include "platform.h"
#include "banyan.h"
#include "config.h"
// #include "banyan_para.c"

#include <hosal_i2c.h>
#include "bsp_iic.h"
#include "bsp_nvs.h"
#include "mcu_sleep.h"

uint8_t g_ChannelCount = CHANNEL_MAX;
uint8_t g_maxChannelCount = CHANNEL_MAX;
uint8_t g_TxCount = (CHANNEL_MAX + 1) / 2;

#if 0
#if (USE_FULL_FRAME || USE_FULL_DUAL_FRAME)
static RADAR_REG_T InitRegList[MAX_REG_NUM] __attribute__((aligned (4))) =
{
    {0x40, 0x4207},
    {0x41, 0x0004},
    {0x01, 0x0000},
    {0x67, 0x0000},
    {0x72, 0x0650},
    {0x42, 0x0001},
    {0x43, 0xD4C0},
    // T3 6ms
    // {0x42, 0x0014},
    // {0x43, 0x1C70},
    {0x44, 0x0022},
    {0x45, 0x0000},
    {0x46, 0x0FA0},
    {0x47, 0x1001},
    {0x48, 0x57C0},
    {0x49, 0x2000},
    {0x4A, 0x55F0},
    {0x4B, 0x0000},
    {0x4C, 0x1770},
    // T3 6ms
    // {0x4B, 0x0012},
    // {0x4C, 0x4F80},
    {0x4D, 0x0000},
    {0x4E, 0x0001},
    {0x4F, 0x0001},
    {0x50, 0x86A0},
    // {0x51, 0x0003},
    // {0x52, 0x0D40},
    // T_NOP 100ms
    // {0x51, 0x0131},
    // {0x52, 0x2D00},
    // T_NOP 30ms
    {0x51, 0x005B},
    {0x52, 0x8D80},
    {0x53, 0x0A02},
    {0x54, 0xAAAB},
    {0x55, 0x0000},
    {0x56, 0x0011},
    {0x57, 0xFFFF},
    {0x58, 0xFFBC},
    {0x59, 0x0000},
    {0x5A, 0x0000},
    {0x5B, 0x0022},
    {0x5C, 0x0022},
    {0x5D, 0x1919},
    {0x5E, 0xFF00},
    {0x61, 0x02B5},
    {0x62, 0x02B5},
    {0x63, 0x02B5},
    {0x64, 0x02B5},
    {0x6E, 0x83FC},
    {0x66, 0x0A00},
    {0x6C, 0x9990},
    {0x6D, 0x9580},
    {0x70, 0x2EA0},
    {0x76, 0x0021},
    {0x02, 0x2064},
    {0x04, 0x020C},
    {0x09, 0x6901},
    {0x0A, 0xC200},
    {0x0B, 0xC064},
    {0x05, 0x000A},
    {0x06, 0x0123},
    {0x0D, 0x1E00},
    {0x0E, 0x2200},
    {0x14, 0x5A03},
    {0x15, 0x1708},
    {0x17, 0x0210},
    {0x20, 0x0000},
    {0x21, 0x0000},
    {0x22, 0x0000},
    {0x23, 0x1DDC},
    {0x24, 0x1D5E},
    {0x25, 0x1CE2},
    {0x26, 0x1C64},
    {0x27, 0x17D0},
    {0x28, 0x16D4},
    {0x29, 0x15DC},
    {0x2A, 0x15DC},
    {0x2B, 0x15DC},
    {0x2C, 0x15DC},
    {0x2D, 0x15DC},
    {0x2E, 0x15DC},
    {0x2F, 0x15DC},
    {0x72, 0x0793},
    {0x67, 0x1840},
    {0x01, 0x8E24},
    {0x41, 0xC844},
    {0x40, 0x0207},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
};
#else
static RADAR_REG_T InitRegList[MAX_REG_NUM] __attribute__((aligned (4))) =
{
    {0x40, 0x4207},
    {0x41, 0x0004},
    {0x01, 0x0000},
    {0x67, 0x0000},
    {0x72, 0x0650},
    // {0x42, 0x0001},
    // {0x43, 0xD4C0},
    // T3 6ms
    // {0x42, 0x0014},
    // {0x43, 0x1C70},
    // T3 7ms
    {0x42, 0x0017},
    {0x43, 0x29B0},
    // T3 100ms
    // {0x42, 0x0132},
    // {0x43, 0xF9F0},
    {0x44, 0x0022},
    {0x45, 0x0000},
    {0x46, 0x0FA0},
    {0x47, 0x1001},
    {0x48, 0x57C0},
    {0x49, 0x2000},
    {0x4A, 0x55F0},
    // {0x4B, 0x0000},
    // {0x4C, 0x1770},
    // T3 6ms
    // {0x4B, 0x0012},
    // {0x4C, 0x4F80},
   // T3 7ms
    {0x4B, 0x0015},
    {0x4C, 0x5CC0},
    // T3 100ms
    // {0x4B, 0x0131},
    // {0x4C, 0x2D00},
    {0x4D, 0x0000},
    {0x4E, 0x0001},
    {0x4F, 0x0001},
    {0x50, 0x86A0},
    // {0x51, 0x0003},
    // {0x52, 0x0D40},
    // T_NOP 100ms
    // {0x51, 0x0131},
    // {0x52, 0x2D00},
    // T_NOP 30ms
    // {0x51, 0x005B},
    // {0x52, 0x8D80},
    // T_NOP 10ms
    // {0x51, 0x001E},
    // {0x52, 0x8480},
    // T_NOP 6ms
    // {0x51, 0x0012},
    // {0x52, 0x4F80},
    // T_NOP 7ms
    {0x51, 0x0015},
    {0x52, 0x5CC0},
    // T_NOP 130ms
    // {0x51, 0x018C},
    // {0x52, 0xBA80},
    {0x53, 0x0A02},
    {0x54, 0xAAAB},
    {0x55, 0x0000},
    {0x56, 0x0011},
    {0x57, 0xFFFF},
    {0x58, 0xFFBC},
    {0x59, 0x0000},
    {0x5A, 0x0000},
    {0x5B, 0x0022},
    {0x5C, 0x0022},
    {0x5D, 0x1919},
    {0x5E, 0xFF00},
    {0x61, 0x02B5},
    {0x62, 0x02B5},
    {0x63, 0x02B5},
    {0x64, 0x02B5},
    {0x6E, 0x83FC},
    {0x66, 0x0A00},
    {0x6C, 0x9990},
    {0x6D, 0x9580},
    {0x70, 0x2EA0},
    {0x76, 0x0021},
    {0x02, 0x2064},
    {0x04, 0x020C},
    {0x09, 0x6901},
    {0x0A, 0xC200},
    {0x0B, 0xC064},
    {0x05, 0x000A},
    {0x06, 0x0123},
    {0x0D, 0x1E00},
    {0x0E, 0x2200},
    {0x14, 0x5A03},
    {0x15, 0x1708},
    {0x17, 0x0210},
    {0x20, 0x0000},
    {0x21, 0x0000},
    {0x22, 0x0000},
    {0x23, 0x1DDC},
    {0x24, 0x1D5E},
    {0x25, 0x1CE2},
    {0x26, 0x1C64},
    {0x27, 0x17D0},
    {0x28, 0x16D4},
    {0x29, 0x15DC},
    {0x2A, 0x15DC},
    {0x2B, 0x15DC},
    {0x2C, 0x15DC},
    {0x2D, 0x15DC},
    {0x2E, 0x15DC},
    {0x2F, 0x15DC},
    {0x72, 0x0793},
    {0x67, 0x1840},
    {0x01, 0x8E24},
    {0x41, 0xC844},
    {0x40, 0x0207},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
};
#endif
#endif

#if 0
static RADAR_REG_T InitRegList[MAX_REG_NUM] __attribute__((aligned (4))) =
{
    {0x40, 0x4207},
    {0x41, 0x0004},
    {0x01, 0x0000},
    {0x67, 0x0000},
    {0x72, 0x0650},
    {0x42, 0x0001},
    {0x43, 0xD4C0},
    {0x44, 0x0022},
    {0x45, 0x0000},
    {0x46, 0x0FA0},
    {0x47, 0x1001},
    {0x48, 0x57C0},
    {0x49, 0x2000},
    {0x4A, 0x55F0},
    {0x4B, 0x0000},
    {0x4C, 0x1770},
    {0x4D, 0x0000},
    {0x4E, 0x0001},
    {0x4F, 0x0001},
    {0x50, 0x86A0},
    {0x51, 0x0003},
    {0x52, 0x0D40},
    {0x53, 0x0A02},
    {0x54, 0xAAAB},
    {0x55, 0x0000},
    {0x56, 0x0011},
    {0x57, 0xFFFF},
    {0x58, 0xFFBC},
    {0x59, 0x0000},
    {0x5A, 0x0000},
    {0x5B, 0x0022},
    {0x5C, 0x0022},
    {0x5D, 0x1919},
    {0x5E, 0xFF00},
    {0x61, 0x02B5},
    {0x62, 0x02B5},
    {0x63, 0x02B5},
    {0x64, 0x02B5},
    {0x6E, 0x83FC},
    {0x66, 0x0A00},
    {0x6C, 0x9990},
    {0x6D, 0x9580},
    {0x70, 0x2EA0},
    {0x76, 0x0021},
    {0x02, 0x2064},
    {0x04, 0x020C},
    {0x09, 0x6901},
    {0x0A, 0xC200},
    {0x0B, 0xC064},
    {0x05, 0x000A},
    {0x06, 0x11FE},
    {0x0D, 0x1E00},
    {0x0E, 0x2200},
    {0x14, 0x5A03},
    {0x15, 0x1708},
    {0x17, 0x0210},
    {0x20, 0x0000},
    {0x21, 0x0000},
    {0x22, 0x0000},
    {0x23, 0x1DDC},
    {0x24, 0x1D5E},
    {0x25, 0x1CE2},
    {0x26, 0x1C64},
    {0x27, 0x17D0},
    {0x28, 0x16D4},
    {0x29, 0x15DC},
    {0x2A, 0x15DC},
    {0x2B, 0x15DC},
    {0x2C, 0x15DC},
    {0x2D, 0x15DC},
    {0x2E, 0x15DC},
    {0x2F, 0x15DC},
    {0x72, 0x0793},
    {0x67, 0x1840},
    {0x01, 0x8E24},
    {0x41, 0xC844},
    {0x40, 0x0207},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
}; //1.538Mhz
#endif

static RADAR_REG_T InitRegList[MAX_REG_NUM] __attribute__((aligned (4))) =
{
    {0x40, 0x4207},
    {0x41, 0x0004},
    {0x01, 0x0000},
    {0x67, 0x0000},
    {0x72, 0x0650},
    {0x42, 0x0001},
    {0x43, 0xD4C0},
    {0x44, 0x0022},
    {0x45, 0x0000},
    {0x46, 0x0FA0},
    {0x47, 0x1001},
    {0x48, 0x57C0},
    {0x49, 0x2000},
    {0x4A, 0x55F0},
    {0x4B, 0x0000},
    {0x4C, 0x1770},
    {0x4D, 0x0000},
    {0x4E, 0x0001},
    {0x4F, 0x0001},
    {0x50, 0x86A0},
    {0x51, 0x0003},
    {0x52, 0x0D40},
    {0x53, 0x0A02},
    {0x54, 0xAAAB},
    {0x55, 0x0000},
    {0x56, 0x0011},
    {0x57, 0xFFFF},
    {0x58, 0xFFBC},
    {0x59, 0x0000},
    {0x5A, 0x0000},
    {0x5B, 0x0022},
    {0x5C, 0x0022},
    {0x5D, 0x1919},
    {0x5E, 0xFF00},
    {0x61, 0x02B5},
    {0x62, 0x02B5},
    {0x63, 0x02B5},
    {0x64, 0x02B5},
    {0x6E, 0x83FC},
    {0x66, 0x0A00},
    {0x6C, 0x9990},
    {0x6D, 0x9580},
    {0x70, 0x2EA0},
    {0x76, 0x0021},
    {0x02, 0x2064},
    {0x04, 0x020C},
    {0x09, 0x6901},
    {0x0A, 0xC200},
    {0x0B, 0xC064},
    {0x05, 0x000A},
    // {0x06, 0x11FE}, // 1.538Mhz  可以收
    // {0x06, 0x11B7}, // 1.987MHz  可以收
    // {0x06, 0x1192}, // 2.510MHz  收不完整
    // {0x06, 0x017F}, // 2.980MHz  收不完整
    // {0x06, 0x015B}, // 3.974MHz  收不完整
    // {0x06, 0x0137}, // 5.960MHz  收不完整
    // {0x06, 0x0125}, // 7.947MHz  收不到
    {0x06, 0x0123}, // 12MHz     收不到
    {0x0D, 0x1E00},
    {0x0E, 0x2200},
    {0x14, 0x5A03},
    {0x15, 0x1708},
    {0x17, 0x0210},
    {0x20, 0x0000},
    {0x21, 0x0000},
    {0x22, 0x0000},
    {0x23, 0x1DDC},
    {0x24, 0x1D5E},
    {0x25, 0x1CE2},
    {0x26, 0x1C64},
    {0x27, 0x17D0},
    {0x28, 0x16D4},
    {0x29, 0x15DC},
    {0x2A, 0x15DC},
    {0x2B, 0x15DC},
    {0x2C, 0x15DC},
    {0x2D, 0x15DC},
    {0x2E, 0x15DC},
    {0x2F, 0x15DC},
    {0x72, 0x0793},
    {0x67, 0x1840},
    {0x01, 0x8E24},
    {0x41, 0xC844},
    {0x40, 0x0207},
    {0x00, 0x0000} /*must be last, do not delete!!!*/
};

static uint16_t rawPointMap[RAW_MAP_NUM] = 
{
	RAW_POINT_64,
    RAW_POINT_128,
    RAW_POINT_256,
    RAW_POINT_512,
    RAW_POINT_1024
};

static const RADAR_REG_T RegList_LowPWR[] =
{
	{0x70, 0x1020},
	{0x6C, 0x8880},
	{0x6D, 0x8800},
	{0x72, 0x0650},
	{0x67, 0x0000},
	{0x66, 0xF0F0},
	{0x6E, 0x03FC},
	{0x41, 0x4804},
	{0x00, 0x0000} 
};

static const RADAR_REG_T RegList_NormalPWR[] =
{
	{0x41, 0xc864},
	{0x72, 0x0653},
	{0x6C, 0x9990},
	{0x6D, 0x9940},
	{0x70, 0x32a0},
	{0x6E, 0xabFC},
	{0x66, 0x0a00},
	{0x67, 0x1840},
	{0x00, 0x0000} 
};
    
uint16_t Radar_GetFftPoint(void)
{
	uint16_t regVal = 64;
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_FFT_NUM, &regVal);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_FFT_NUM, &regVal);
	return regVal;
}

uint16_t Radar_GetRawPoint(void)
{
    uint16_t val = 256;
    uint16_t rawVal = 0;
    
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_RAW_PEAK_NUM, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_RAW_PEAK_NUM, &val);
    rawVal = (val >> BANYAN_RAW_POS) & BANYAN_RAW_MASK;

    if (rawVal < RAW_MAP_NUM)
    {
        return rawPointMap[rawVal];
    }
    else
    {
        return RAW_POINT_64;
    }
}

uint16_t Radar_GetOneFrameChirpNum(void)
{
	uint16_t val = 32;
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_PAT_CHIRP_NUM, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_PAT_CHIRP_NUM, &val);
	return val;
}

uint16_t Radar_GetDfftDataNum(void)
{
    uint16_t val = 32;
    
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_DFFT_DATA_NUM, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_DFFT_DATA_NUM, &val);

    return (val >> BANYAN_DFFT_DATA_NUM_POS);
}

uint16_t Radar_GetDfftPeakSize(void)
{
    uint16_t val = 32;
    
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_RAW_PEAK_NUM, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_RAW_PEAK_NUM, &val);

    return ((val & BANYAN_PEAK_MASK) * 4); /*4--word length*/
}

uint16_t Radar_GetDfftChirpNum(void)
{
    uint16_t val = 32;
    
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_DFFT_CHIRP_NUM, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_DFFT_CHIRP_NUM, &val);

    return (val >> BANYAN_DFFT_CHIRP_NUM_POS);
}

uint8_t Radar_GetDataType(void)
{
    uint8_t dataType = DATA_TYPE_FFT;
    uint16_t val = BANYAN_FFT_DATA;
    
    // I2C_Read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_FUN_SWITCH, &val);
    bsp_iic_read(I2C_ADDR_BanYan_Chip0, BANYAN_DIG_FUN_SWITCH, &val);

    if (val & BANYAN_DFFT_DATA)
    {
        dataType = DATA_TYPE_DFFT;
    }
    else if (val & BANYAN_FFT_DATA)
    {
        dataType = DATA_TYPE_FFT;
    }
    else if (val & BANYAN_DFFT_PEAK_DATA)
    {
        dataType = DATA_TYPE_DFFT_PEAK;
    }
	else if (val & BANYAN_DSRAW_DATA)
    {
        dataType = DATA_TYPE_DSRAW;
    }
    else
    {
        dataType = DATA_TYPE_MAX;
    } 

    return dataType;
}

void Radar_Init(void)
{
    uint16_t loop = 0;
    uint16_t data;

    while(InitRegList[loop].addr) 
    {
        bsp_iic_write(I2C_ADDR_BanYan_Chip0, (uint8_t)(InitRegList[loop].addr), InitRegList[loop].val);
        loop++;
    }

    loop = 0; 
    while(InitRegList[loop].addr) 
    {
        data = 0;
        bsp_iic_read(I2C_ADDR_BanYan_Chip0, (uint8_t)(InitRegList[loop].addr), &data);
        printf("InitRegList[%d].addr=0x%02x, data=0x%04x\r\n", loop, InitRegList[loop].addr, data);
        loop++;
    }

    Delay(22);
    rext_on();  //REXT接地
    // gpio_bit_set(GPIOB, GPIO_PIN_7);
    Delay(1);
    /* 延时22ms，等待SPI稳定，此时间一般大于frame中雷达上扫时间 */
    Delay(22);
}

void Radar_UpdateReg(uint16_t addr, uint16_t val)/*currently only update existing reg*/
{
    uint16_t loop = 0;

    while(InitRegList[loop].addr) 
    {   
        if (loop < REG_FIX_NUM)
        {
            loop++;
            continue;
        }
        
        if (InitRegList[loop].addr == addr)
        {
            InitRegList[loop].val = val;
            Config_NeedFlashWrite();
            return;
        }
        loop++;
    }
}

void* Radar_GetRadarParaAddr(void)
{
    printf("InitRegList addr=%p\r\n", &InitRegList);
    return (void*)&InitRegList;
}

uint32_t Radar_GetRadarParaLen(void)
{
    return sizeof(InitRegList);
}

#if defined(SUPPORT_DYNAMIC_SYS_MODE)
void* Radar_GetRegListMTTAddr(void)
{
    return (void*)&InitRegList_MTT;
}

uint32_t Radar_GetRegListMTTLen(void)
{
    return sizeof(InitRegList_MTT);
}

void* Radar_GetRegListVSAddr(void)
{
    return (void*)&InitRegList_VS;
}

uint32_t Radar_GetRegListVSLen(void)
{
    return sizeof(InitRegList_VS);
}

void* Radar_GetRegListGRAddr(void)
{
    return (void*)&InitRegList_GR;
}

uint32_t Radar_GetRegListGRLen(void)
{
    return sizeof(InitRegList_GR);
}
#endif

void Algo_SaveParameter(uint32_t*data,uint16_t len)
{
    Config_WritebodysensingFlash((uint32_t*)data, len);
}

void Algo_ReadParameter(uint32_t *buf, uint32_t bufLen)
{
    // uint32_t index = 0;
    // for (index = 0; index < bufLen; index++)
    // {
    //     buf[index] = REG32(BODYSENSING_START_ADDR + 4 * (index + 1));
    // }

    bsp_nv_read(elem_key.parameter, (char *)buf, (uint16_t)bufLen);
}

uint32_t Algo_ReadParameterExistFlag(void)
{
    uint32_t bParamExistFlag = Config_ReadAlgorithmParamLen();
    return bParamExistFlag;
}

void WriteSocAllReg(void)
{
    uint16_t loop = 0;
    
    while(InitRegList[loop].addr) 
    {
        bsp_iic_write(I2C_ADDR_BanYan_Chip0, (uint8_t)(InitRegList[loop].addr), InitRegList[loop].val);
        loop++;
    }
}
