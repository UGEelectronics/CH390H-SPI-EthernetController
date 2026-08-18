#include "tcp_server.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/uart.h"

static const char *TAG = "socket";

// An example of echo test with hardware flow control on UART
void echo_task(void *arg)
{
    uart_task_para_t *uart_para = (uart_task_para_t *)arg;
    int uart_num = uart_para->uart_num;
    uint8_t data[128];
    
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_LOGI(TAG, "Start RS485 application test and configure UART.");

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 512 * 2, 512 * 2, 0, NULL, 0));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings
    ESP_ERROR_CHECK(uart_set_pin(uart_num, uart_para->txd_io, uart_para->rxd_io, uart_para->rts_io, UART_PIN_NO_CHANGE));

    if (uart_para->uart_num != UART_NUM_2)
    {
        // Set RS485 half duplex mode
        ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));
    }

    // // Set read timeout of UART TOUT feature
    // ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, 3));

    ESP_LOGI(TAG, "UART %d start receive loop.\r", uart_para->uart_num);
    // echo_send(uart_num, "Start RS485 UART test.\r\n", 24);

    while (1)
    {
        // Read data from UART
        int len = uart_read_bytes(uart_num, data, 128, pdMS_TO_TICKS(10));

        // Write data back to UART
        if (len <= 0)
        {
            continue;
        }
        // ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
        // ESP_LOGI(TAG, "Received %u bytes (uart %d):", len, uart_num);
        if (uart_para->sock == -1)
        {
            continue;
        }
        // send() can return less bytes than supplied length.
        // Walk-around for robust implementation.
        int to_write = len;
        while (to_write > 0)
        {
            int written = send(uart_para->sock, data + (len - to_write), to_write, 0);
            if (written < 0)
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                // Failed to retransmit, giving up
                break;
            }
            to_write -= written;
        }
    }
    vTaskDelete(NULL);
}

static void do_retransmit(const int sock, int uart_num)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            // ESP_LOGI(TAG, "Received %d bytes", len);
            // ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_INFO);
            uart_write_bytes(uart_num, rx_buffer, len);
        }
    } while (len > 0);
}

void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    uart_task_para_t* uart_para = (uart_task_para_t*)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(uart_para->port);
    ip_protocol = IPPROTO_IP;

    int listen_sock = socket(uart_para->addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", uart_para->addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", uart_para->port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        uart_para->sock = sock;

        // Convert ip address to string
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock, uart_para->uart_num);

        shutdown(sock, 0);
        close(sock);
        uart_para->sock = -1;
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}