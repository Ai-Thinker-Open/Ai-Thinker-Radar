#ifndef __BODYSENSING_TYPES_H__
#define __BODYSENSING_TYPES_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>


#define     ONEFFT_POINT        (10)
#define     FRAME_DEPTH         (34)
#define     ACC_DEPTH           ((FRAME_DEPTH-2) >> 2)

#define     ROI_MIN             (2)
#define     NOCOHERENTLEN       (40)

#define     RADAR_RANGE_RES     (75)/* æ‡¿Î∑÷±Ê¬ £®cm£© */

#pragma pack(4) 
typedef struct COMPLEXFLOAT
{
    float real;
    float imag;
}COMPLEXFLOAT_T;
#pragma pack() 

typedef struct COMPLEX16
{
    int16_t real;
    int16_t imag;
}COMPLEX16_T;

typedef struct COMPLEX32
{
    int32_t real;
    int32_t imag;
}COMPLEX32_T;

typedef struct COMPLEX64
{
    int64_t real;
    int64_t imag;
}COMPLEX64_T;

#endif
