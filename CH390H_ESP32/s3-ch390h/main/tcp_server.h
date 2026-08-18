#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

typedef struct
{
    int uart_num;
    int sock;
    int txd_io;
    int rxd_io;
    int rts_io;

    int addr_family;
    int port;
} uart_task_para_t;

void echo_task(void *arg);
void tcp_server_task(void *pvParameters);

#endif // __TCP_SERVER_H
