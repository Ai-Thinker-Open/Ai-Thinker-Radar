#include "bsp_nvs.h"
#include <stdio.h>

elem_key_t elem_key = 
{
    .magic = "magic",
    .elem_len[0] = "elemLen1",
    .elem_len[1] = "elemLen2",
    .elem_member[0] = "elem_mb1",
    .elem_member[1] = "elem_mb2",
    .bodysensing = "bodyss",
    .parameter = "param",
};


int bsp_nv_write(char *key, char *value, uint16_t val_len)
{
    int err;

    err = ef_set_env_blob(key, (void *)value, val_len);
    if (err != EF_NO_ERR)
    {
        printf("[NV] write key[%s] err\r\n", key);
    }

    return err;
}

int bsp_nv_read(char *key, char *value, uint16_t val_len)
{
    size_t len;

    ef_get_env_blob(key, (void *)value, val_len, &len);
    if (len < val_len)
    {
        printf("[NV] get fail:%d \r\n", len);
        return -1;
    }

    return 0;
}