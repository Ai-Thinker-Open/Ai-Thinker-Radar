#ifndef __BODYSENSING_DSP_H__
#define __BODYSENSING_DSP_H__
#include "bodysensing_types.h"


extern COMPLEX16_T chirp_data_clean[ONEFFT_POINT];
extern COMPLEX16_T Yone[ONEFFT_POINT];
extern COMPLEX32_T Wone[ONEFFT_POINT];

void cluster_remove(COMPLEX16_T *range_chirp);
void OneRecursiveCanceller(COMPLEX16_T *range_chirp, uint8_t start_pos, uint8_t shift);

uint32_t abs_32(int32_t src);
uint32_t Euclidean_32(uint32_t x, uint32_t y);
uint16_t abs_16(int16_t src);
uint16_t Euclidean_16(uint16_t x, uint16_t y);

#endif
