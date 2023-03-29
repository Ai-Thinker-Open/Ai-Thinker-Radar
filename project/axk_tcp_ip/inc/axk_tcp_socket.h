#ifndef _AXK_TCP_SOCKET_H_
#define _AXK_TCP_SOCKET_H_

#include <stdio.h>
#include <stdint.h>
#include <lwip/sockets.h>


#define MAX_CONNECT_NUM     (5)

int axk_tcp_init(const char* ip, int port);
int axk_tcp_accept(int sfd);
int axk_tcp_connect(const char* ip, int port);
int axk_tcp_nonblocking_recv(int conn_sockfd, 
                         void *rx_buf, 
                         int buf_len, 
                         int timeval_sec, 
                         int timeval_usec);
int axk_tcp_blocking_recv(int conn_sockfd, void *rx_buf, uint16_t buf_len);
int axk_tcp_send(int conn_sockfd, uint8_t *tx_buf, uint16_t buf_len);
void axk_tcp_close(int sockfd);


#endif // _AXK_TCP_SOCKET_H_
