#ifndef _FT_CONFIG_H_
#define _FT_CONFIG_H_

#include <stdio.h>
#include <stdint.h>






uint32_t Config_ReadRadarParaLen(void);
void Config_WriteRadarPara2Flash(uint32_t* addr, uint16_t len);
void Config_ReadRadarParaData(uint32_t* addr, uint16_t len);

#endif // _FT_CONFIG_H_




