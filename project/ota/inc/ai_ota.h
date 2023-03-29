#ifndef _AI_OTA_H_
#define _AI_OTA_H_

#include "ota_parse.h"


ota_parame ai_ota_parame_init(char *host, int port, char *resource);
void ai_https_update_ota(void *param);
void ai_http_update_ota(void *param);


#endif

