#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <hal_boot2.h>

#include "at_ota.h"
#include "ai_ota.h"

// #include "axk_ota_import.h"
// #include "axk_at.h"

// #include "ai_ota.h"
// #include "axk_ota_export.h"

#include "bl_sys.h"
#include "FreeRTOS.h"
#include "task.h"
#include "http_parser.h"

static ota_parame ota_param;
static int ota_type;
static char ota_host[256];
static char ota_resource[256];

static void _ota_task(void *params) 
{
    vTaskDelay(pdMS_TO_TICKS(500));
    if (ota_type == AT_OTA_MODE_HTTP) {
        ai_http_update_ota(&ota_param);
    } else {
        ai_https_update_ota(&ota_param);
    }

    vTaskDelete(NULL);
}

// OTA 入口函数
int axk_hal_ota_update(at_ota_update_mode_t mode, const char *version, const char *fwname)
{
    char *resource = NULL;
    HALPartition_Entry_Config otaEntry;

    if (mode == AT_OTA_MODE_HTTP) {
        return -1;
    }

    if (hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &otaEntry)) {
        printf("[OTA] get otaEntry fail\r\n");
        return -1;
    }

    /* 800KB */
    if (otaEntry.maxLen[1] < 0xC8000) {
        // resource = "/ota.bin.xz";
        resource = "/ota/ota.bin.xz";
        printf("[OTA] get xz fw \r\n");
    } else {
        // resource = "/ota.bin";
        resource = "/ota/ota.bin";
        printf("[OTA] get normal fw \r\n");
    }

    ota_type = AT_OTA_MODE_HTTPS;
    // ota_param = ai_ota_parame_init("192.168.2.141", 443, resource);
    ota_param = ai_ota_parame_init("aithinker-static.oss-cn-shenzhen.aliyuncs.com", 443, resource);

    int ret = xTaskCreate(_ota_task, "ota", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        printf("[OTA] task create fail: %d\r\n", ret);
        return -1;
    }

    return 0;
}

int axk_hal_user_ota_update(const char *url, uint16_t length)
{
    int port;
    char schema[8] = {0};
    struct http_parser_url purl;
    HALPartition_Entry_Config otaEntry;

    if (hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &otaEntry)) {
        printf("[OTA] get otaEntry fail\r\n");
        return -1;
    }

    if (url == NULL) {
        return -1;
    }

    http_parser_url_init(&purl);

    int parser_status = http_parser_parse_url(url, length, 0, &purl);

    if (parser_status != 0) {
        printf("[OTA] Error parse url:%s\r\n", url);
        return -1;
    }

    memset(ota_host, 0, sizeof(ota_host));
    memset(ota_resource, 0, sizeof(ota_resource));

    if (purl.field_data[UF_SCHEMA].len > 8) {
        printf("[OTA] schema ovfl \r\n");
        return -1;
    }
    memcpy(schema, url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
    if (strcasecmp(schema, "http") == 0) {
        ota_type = AT_OTA_MODE_HTTP;
        port = 80;
    } else if (strcasecmp(schema, "https") == 0) {
        ota_type = AT_OTA_MODE_HTTPS;
        port = 443;
    } else {
        printf("[OTA] undef schema\r\n");
        return -1;
    }
    port = purl.port ? purl.port : port;
    memcpy(ota_host, url + purl.field_data[UF_HOST].off, purl.field_data[UF_HOST].len);
    memcpy(ota_resource, url + purl.field_data[UF_PATH].off, purl.field_data[UF_PATH].len);

    printf("[OTA] port:%d host:%s path:%s\r\n", port, ota_host, ota_resource);

    ota_param = ai_ota_parame_init(ota_host, port, ota_resource);

    int ret = xTaskCreate(_ota_task, "ota", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        printf("[OTA] task create fail: %d\r\n", ret);
        return -1;
    }

    return 0;
}
