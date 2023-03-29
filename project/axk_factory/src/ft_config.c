#include "ft_config.h"
#include "bsp_nvs.h"




/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PARA_LEN_KEY    "datalen"
#define PARA_DATA_KEY   "paradata"


uint32_t Config_ReadRadarParaLen(void)
{
    uint32_t dataLen = 0;

    bsp_nv_read(PARA_LEN_KEY, (char *)&dataLen, sizeof(dataLen));

    return dataLen;
}


void Config_WriteRadarPara2Flash(uint32_t* addr, uint16_t len)
{
    uint32_t dataLen = len;
    
    bsp_nv_write(PARA_LEN_KEY, (char *)&dataLen, sizeof(dataLen));
    bsp_nv_write(PARA_DATA_KEY, (char *)addr, len);
}

void Config_ReadRadarParaData(uint32_t* addr, uint16_t len)
{
    bsp_nv_read(PARA_DATA_KEY, (char *)addr, len);
}