#ifndef __AXK_IP_IMPORT_H__
#define __AXK_IP_IMPORT_H__

#include <stdint.h>

#include "at_ota.h"

/**
 * @brief   ota update
 *
 * @return  0:success other:fail
 */
int axk_hal_ota_update(at_ota_update_mode_t mode, const char *version, const char *fwname);
/**
 * @brief   user ota update
 *
 * @return  0:success other:fail
 */
int axk_hal_user_ota_update(const char *url, uint16_t length);

#endif
