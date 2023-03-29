#ifndef __AT_OTA_H__
#define __AT_OTA_H__

#include <stdint.h>

typedef enum
{
    AT_OTA_STATE_FAIL = -1,
    AT_OTA_STATE_NONE,
    AT_OTA_STATE_FOUND_SERVER,
    AT_OTA_STATE_CONNECTED,
    AT_OTA_STATE_GET_VERSION,
    AT_OTA_STATE_FINISH,
} at_ota_update_state_t;

typedef enum
{
    AT_OTA_MODE_HTTP,
    AT_OTA_MODE_HTTPS,
    AT_OTA_MODE_ANY,
} at_ota_update_mode_t;

typedef struct
{
    uint32_t update_state;
    uint32_t nonblocking;
} at_ota_config_t;

// int at_query_update_cmd(uint32_t argc, const char *argv[]);
// int at_setup_update_cmd(uint32_t argc, const char *argv[]);
// int at_exe_update_cmd(uint32_t argc, const char **argv);
// int at_setup_userota_cmd(uint32_t argc, const char *argv[]);

#endif
