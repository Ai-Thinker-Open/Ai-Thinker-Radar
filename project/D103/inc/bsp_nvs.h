#ifndef _BSP_NVS_H_
#define _BSP_NVS_H_

#include <stdint.h>
#include <easyflash.h>
#include "config.h"


typedef struct 
{
    char *magic;
    char *elem_len[FLASH_ELEM_MAX];
    char *elem_member[FLASH_ELEM_MAX];
    char *bodysensing;
    char *parameter;
} elem_key_t;


extern elem_key_t elem_key;

int bsp_nv_write(char *key, char *value, uint16_t val_len);
int bsp_nv_read(char *key, char *value, uint16_t val_len);

#endif
