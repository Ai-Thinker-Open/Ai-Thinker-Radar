#include "ft_system.h"
#include "bsp_spi.h"
#include "ft_banyan.h"
#include "ft_dataprocess.h"

#include <FreeRTOS.h>
#include <task.h>

void power_off(void);
void power_on(void);
void cs_interrupt_init(void);
void create_spi_init_task(void (*spi_slave_init_type)(void));
void ft_bsp_spi_slave_init(void);

void Ft_System_Reconfig(void)
{
    static uint8_t reconfig_cnt = 0;

    if (reconfig_cnt != 0)  // No reconfiguration in production mode
        return;

    reconfig_cnt = 1;

    bsp_spi_deinit();
    power_off();
    // cs_interrupt_init();
    vTaskDelay(5);
    power_on();

    Ft_Radar_Init();            // 写产测iic
    Ft_DataProc_Init();         // 初始化数据

    create_spi_init_task(ft_bsp_spi_slave_init);
}