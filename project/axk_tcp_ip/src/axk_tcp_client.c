#include "axk_tcp_socket.h"
#include "FreeRTOS.h"
#include "task.h"

void axk_tcp_client_task(void *params)
{
    uint8_t send_buf[128] = {"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    uint8_t recv_buf[128] = {0};

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    printf("take notify!!!\r\n");
    vTaskDelay(10000);

    printf("==================tcp cient==================\n");

    int server_fd = axk_tcp_connect("192.168.169.2", 4321);
    if (server_fd < 0)
    {
        printf("tcp_connect error!\n");
        vTaskDelete(NULL);
    }

    for (;;)
    {
        int send_len = axk_tcp_send(server_fd, send_buf, strlen((const char *)send_buf));
        if (send_len <= 0)
        {
            printf("axk_tcp_send error!\n");
            axk_tcp_close(server_fd);
            vTaskDelete(NULL);  
        }
        else
        {
            // printf("send success! send: %s, send_len: %d\n", send_buf, send_len);
            printf("send_len: %d\r\n", send_len);
        }

        // bzero(recv_buf, sizeof(recv_buf));
        // int recv_len = axk_tcp_nonblocking_recv(server_fd, 
        //                                     recv_buf, 
        //                                     sizeof(recv_buf), 
        //                                     1, 
        //                                     0);
        // if (recv_len > 0)
        // {
        //     printf("recv : %s\n", recv_buf);
        // }

        vTaskDelay(100);
    }

    vTaskDelete(NULL);
}