#ifndef _AXK_RADAR_EXPORT_H_
#define _AXK_RADAR_EXPORT_H_

#include <stdint.h>


extern void (*SendResultCallback)(uint8_t *buff, uint16_t len);


#endif
