#include <FreeRTOS.h>
#include <task.h>
#include "axk_tcp_socket.h"


static void process_client_data(void *arg)
{
    int client_fd = *(int*)arg;

    while (1)
    {
        uint8_t buf[128] = {0};
        
        int recv_len = axk_tcp_nonblocking_recv(client_fd, 
                                            buf, 
                                            sizeof(buf), 
                                            1, 
                                            0);
        if (recv_len <= 0)
        {
            continue;
        }
        else
        {
            printf("client_fd = %d, recv : %s\n", client_fd, buf);
            int send_len = axk_tcp_send(client_fd, buf, strlen((const char *)buf));
            if (send_len <= 0)
            {
                printf("send error!\n");
                axk_tcp_close(client_fd);
                vTaskDelete(NULL);
            }
            else
            {
                printf("send success! send: %s, send_len: %d\n", buf, send_len);
            }
        }
        
        vTaskDelay(10);
    }
}

void axk_tcp_server_task(void *params)
{
    printf("==================tcp server==================\n");
    int server_fd = axk_tcp_init(NULL, 4321);

    for (;;)
    {
        int new_fd = axk_tcp_accept(server_fd);

        // 创建客户端数据处理线程
        BaseType_t ret = xTaskCreate(process_client_data, "axktcp", 1024, (void *)&new_fd, 10, NULL);
        if(ret != 0)
        {
            perror("pthread_create");
            break;
        }
    }

    axk_tcp_close(server_fd);
    vTaskDelete(NULL);
}