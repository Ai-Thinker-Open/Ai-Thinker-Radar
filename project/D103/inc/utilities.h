/**
  ******************************************************************************
  * @file    utilities.h
  * @author  ting.gao@iclegend.com
  * @brief   utilities header file
  ******************************************************************************
  */
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>

#define OK		0
#define FAIL    -1

#define LE16_TO_BIG(x) \
            ((uint16_t)( \
                (((uint16_t)(x) & (uint16_t)0x00ffU) << 8) | \
                (((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))


#define BIG16_TO_LE LE16_TO_BIG

#define LE32_TO_BIG(x) \
	((uint32_t)( \
		(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
		(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
		(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
		(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))
#define BIG32_TO_LE LE32_TO_BIG

#define __ALIGN(n)      __attribute__((aligned (n)))
#define __SRAM4_DATA    __attribute__((section(".Sram4Data")))

#define ARRAY_SIZE(X)    (sizeof(X)/sizeof(X[0]))

void RunFailed(uint8_t *file, uint32_t line);

#ifdef __cplusplus
}
#endif

#endif

