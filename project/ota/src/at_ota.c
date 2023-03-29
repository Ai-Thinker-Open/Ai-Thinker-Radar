// #include "axk_at.h"
// #include "axk_utils.h"
// #include "at_cmd.h"
// #include "at_ota.h"
// #include "axk_ota_import.h"

// #include <string.h>

// static struct
// {
//     uint8_t *buff;
//     uint16_t pos;
//     uint16_t length;
// } _ota_current = {0};

// int at_query_update_cmd(uint32_t argc, const char *argv[])
// {
//     axk_at_printf("+CIPUPDATE:%d\r\n", g_at_cmd_config.ota.update_state);

//     AT_RESPONSE_OK_WITH_ENTER;
//     return AT_ERR_SUCCESS;
// }

// int at_setup_update_cmd(uint32_t argc, const char *argv[])
// {
//     int mode;
//     int nonblocking = 0;
//     char version[32] = {'\0'};
//     char fwname[32] = {'\0'};

//     switch (argc)
//     {
//     case 4:
//         AT_CMD_PARSE_NUMBER_EX(3, &nonblocking, 0, 1);
//     case 3:
//         if (strlen(argv[2]) != 0) {
//             AT_CMD_PARSE_STRING(2, fwname, 0);
//         }
//     case 2:
//         if (strlen(argv[1]) != 0) {
//             AT_CMD_PARSE_STRING(1, version, 0);
//         }
//         break;
//     default:
//         break;
//     }
//     AT_CMD_PARSE_NUMBER_EX(0, &mode, 0, 1);

//     g_at_cmd_config.ota.nonblocking = nonblocking;
//     if (axk_hal_ota_update(mode, version, fwname) != 0) {
//         return AT_ERR_FAILURE;
//     }

//     if (nonblocking == 0) {
//         return AT_ERR_BUSY;
//     }

//     AT_RESPONSE_OK;
//     return AT_ERR_SUCCESS;
// }

// int at_exe_update_cmd(uint32_t argc, const char **argv)
// {
//     g_at_cmd_config.ota.nonblocking = 0;
//     if (axk_hal_ota_update(AT_OTA_MODE_ANY, NULL, NULL) != 0) {
//         return AT_ERR_FAILURE;
//     }

//     return AT_ERR_BUSY;
// }

// int axk_at_ota_export(at_ota_update_state_t state)
// {
//     axk_at_printf("+CIPUPDATE:%d\r\n", state);
//     g_at_cmd_config.ota.update_state = state;

//     if (g_at_cmd_config.ota.nonblocking == 0) {
//         if (state == AT_OTA_STATE_FINISH) {
//             AT_RESPONSE_OK_WITH_ENTER;
//             g_at_ctrl.at_state = AT_IDLE;
//         } else if (state == AT_OTA_STATE_FAIL) {
//             AT_RESPONSE_ERROR_WITH_ENTER;
//             g_at_ctrl.at_state = AT_IDLE;
//         }
//     }

//     return 0;
// }

// static void at_user_ota_callback(char data)
// {
//     int ret;

//     if (_ota_current.pos < _ota_current.length)
//     {
//         _ota_current.buff[_ota_current.pos] = data;
//         _ota_current.pos++;
//     }
//     if (_ota_current.pos < _ota_current.length)
//     {
//         return;
//     }

//     AT_ENTER;
//     axk_at_printf("Recv %d bytes\r\n", _ota_current.length);
//     AT_ENTER;

//     _ota_current.buff[_ota_current.length] = '\0';
//     ret = axk_hal_user_ota_update((char *)_ota_current.buff, _ota_current.length);
//     if (ret != 0) {
//         AT_RESPONSE_ERROR;
//     }
    
//     free(_ota_current.buff);
//     memset(&_ota_current, 0, sizeof(_ota_current));
//     at_port_exit_specific();
// }

// int at_setup_userota_cmd(uint32_t argc, const char *argv[])
// {
//     int length;

//     AT_CMD_PARSE_NUMBER_EX(0, &length, 0, 1024);

//     _ota_current.pos = 0;
//     _ota_current.length = length;
//     if ((_ota_current.buff = malloc(length + 1)) == NULL)
//     {
//         return AT_ERR_FAILURE;
//     }
//     if (0 != at_port_enter_specific(at_user_ota_callback))
//     {
//         return AT_ERR_FAILURE;
//     }

//     AT_RESPONSE_OK;
//     AT_ENTER;
//     axk_at_printf(">");

//     return 0;
// }
