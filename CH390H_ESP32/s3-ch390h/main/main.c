/*
 * SPDX-FileCopyrightText: 2024 Sergey Kharenko
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileContributor: 2024 Sergey Kharenko
 * SPDX-FileContributor: 2024 Espressif Systems (Shanghai) CO LTD
 */
#include <string.h>
#include "sys/socket.h" // for INADDR_ANY
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_bit_defs.h"
#include "esp_mac.h"
#include "argtable3/argtable3.h"
#include "nvs_flash.h"

#include "cmd_system.h"
#include "ping_cmd.h"
#include "iperf.h"
#include "iperf_cmd.h"
#include "wifi_cmd.h"

#include "basic.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "tcp_server.h"

static const char *TAG = "main";

static esp_eth_handle_t eth_handle;
static esp_netif_t *eth_netif;

static bool started = false;
static EventGroupHandle_t eth_event_group;
static const int GOTIP_BIT = BIT0;

// 测试WIFI信息
#define ESP_WIFI_SSID "YOUR_SSID"
#define ESP_WIFI_PASS "YOUR_PASSWORD"
#define ESP_MAXIMUM_RETRY 3

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED)
    {
        if (esp_netif_dhcpc_stop(eth_netif) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to stop dhcp client");
        }
        esp_netif_ip_info_t ip_t = {0};
        ip_t.ip.addr = ipaddr_addr("192.168.1.10");
        ip_t.netmask.addr = ipaddr_addr("255.255.255.0");
        ip_t.gw.addr = ipaddr_addr("192.168.1.1");
        if (esp_netif_set_ip_info(eth_netif, &ip_t) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set ip info");
        }
        ESP_LOGI(TAG, "ETHERNET_EVENT_CONNECTED");
    }
    else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(TAG, "ETHERNET_EVENT_DISCONNECTED");
    }
    else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_START)
    {
        started = true;
    }
    else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_STOP)
    {
        xEventGroupClearBits(eth_event_group, GOTIP_BIT);
        started = false;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP)
    {
        xEventGroupSetBits(eth_event_group, GOTIP_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void eth_init(void)
{
    eth_event_group = xEventGroupCreate();

    basic_init(&eth_handle);

    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
    // default esp-netif configuration parameters.
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

/* "ethernet" command */
static struct
{
    struct arg_str *control;
    struct arg_end *end;
} eth_control_args;

static int eth_cmd_control(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&eth_control_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, eth_control_args.end, argv[0]);
        return 1;
    }

    if (!strncmp(eth_control_args.control->sval[0], "info", 4))
    {
        uint8_t mac_addr[6];
        esp_netif_ip_info_t ip;
        printf("%s:\r\n", esp_netif_get_desc(eth_netif));
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        printf("  HW ADDR: " MACSTR "\r\n", MAC2STR(mac_addr));
        esp_netif_get_ip_info(eth_netif, &ip);
        printf("  ETHIP: " IPSTR "\r\n", IP2STR(&ip.ip));
        printf("  ETHMASK: " IPSTR "\r\n", IP2STR(&ip.netmask));
        printf("  ETHGW: " IPSTR "\r\n", IP2STR(&ip.gw));
    }
    return 0;
}

void register_ethernet_commands(void)
{
    eth_control_args.control = arg_str1(NULL, NULL, "<info>", "Get info of Ethernet");
    eth_control_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
        .command = "ethernet",
        .help = "Ethernet interface IO control",
        .hint = NULL,
        .func = eth_cmd_control,
        .argtable = &eth_control_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* "wifi_set" command */
static struct
{
    struct arg_str *ssid;
    struct arg_str *pass;
    struct arg_end *end;
} wifi_set_args;

static int wifi_cmd_set(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&wifi_set_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, wifi_set_args.end, argv[0]);
        return 1;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, wifi_set_args.ssid->sval[0], 32);
    strncpy((char*)wifi_config.sta.password, wifi_set_args.pass->sval[0], 64);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    return 0;
}

void register_wifi_set_commands(void)
{
    wifi_set_args.ssid = arg_str1(NULL, NULL, "<SSID>", "WIFI SSID");
    wifi_set_args.pass = arg_str1(NULL, NULL, "<PASS>", "WIFI PASS");
    wifi_set_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
        .command = "wifi_set",
        .help = "Set Wifi info",
        .hint = NULL,
        .func = wifi_cmd_set,
        .argtable = &wifi_set_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void boot_task(void *pvParameter)
{
    gpio_set_level(GPIO_NUM_18, 1);
    gpio_set_level(GPIO_NUM_8, 1);

    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL << 39) | (1ULL << 40)) | ((1ULL << 18) | (1ULL << 8));
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // self test
    gpio_set_level(GPIO_NUM_39, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(GPIO_NUM_40, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(GPIO_NUM_39, 0);
    gpio_set_level(GPIO_NUM_40, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));
    // gpio_set_level(GPIO_NUM_18, 0);
    gpio_set_level(GPIO_NUM_8, 0);
    vTaskDelete(NULL);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = ESP_WIFI_SSID,
    //         .password = ESP_WIFI_PASS,
    //     },
    // };
    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void trace_task(void *pvParameters)
{
    char buf[1024];
    while (1)
    {
        vTaskList(buf);
        printf("%s", buf);
        printf("==================================\n");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

static uart_task_para_t uart0_para;
static uart_task_para_t uart1_para;
static uart_task_para_t uart2_para;

void app_main(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "iperf>";

    // init console REPL environment
#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usbjtag_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config, &repl_config, &repl));
#endif

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 开机自检
    xTaskCreate(boot_task, "boot_task", 4096, NULL, 2, NULL);

    register_system_common();
    register_ethernet_commands();
    register_wifi_set_commands();
    
    // app_register_all_wifi_commands();
    app_register_ping_commands();
    app_register_iperf_commands();

    eth_init();
    wifi_init_sta();

    // 串口参数设定
    uart0_para.uart_num = UART_NUM_0;
    uart0_para.txd_io = 1;
    uart0_para.rxd_io = 43;
    uart0_para.rts_io = 2;

    uart1_para.uart_num = UART_NUM_1;
    uart1_para.txd_io = 44;
    uart1_para.rxd_io = 41;
    uart1_para.rts_io = 42;

    uart2_para.uart_num = UART_NUM_2;
    uart2_para.txd_io = 16;
    uart2_para.rxd_io = 17;
    uart2_para.rts_io = -1;

    uart0_para.addr_family = AF_INET;
    uart0_para.port = 8000;
    uart0_para.sock = -1;

    uart1_para.addr_family = AF_INET;
    uart1_para.port = 8001;
    uart1_para.sock = -1;

    uart2_para.addr_family = AF_INET;
    uart2_para.port = 8002;
    uart2_para.sock = -1;

    // TCP接收->串口发送线程
    xTaskCreate(tcp_server_task, "tcp0_server", 3072, (void *)&uart0_para, 5, NULL);
    xTaskCreate(tcp_server_task, "tcp1_server", 3072, (void *)&uart1_para, 5, NULL);
    xTaskCreate(tcp_server_task, "tcp2_server", 3072, (void *)&uart2_para, 5, NULL);

    // 串口接收->TCP发送线程
    xTaskCreate(echo_task, "echo_task", 3072, &uart0_para, 3, NULL);
    xTaskCreate(echo_task, "echo_task1", 3072, &uart1_para, 3, NULL);
    xTaskCreate(echo_task, "echo_task2", 3072, &uart2_para, 3, NULL);

    // 内存分析
    // xTaskCreate(trace_task, "trace_task", 4096, NULL, 6, NULL);

    printf("\n =======================================================\n");
    printf(" |       Steps to Test Ethernet Bandwidth              |\n");
    printf(" |                                                     |\n");
    printf(" |  1. Enter 'help', check all supported commands      |\n");
    printf(" |  2. Wait ESP32 to get IP from DHCP                  |\n");
    printf(" |  3. Enter 'ethernet info', optional                 |\n");
    printf(" |  4. Server: 'iperf -u -s -i 3'                      |\n");
    printf(" |  5. Client: 'iperf -u -c SERVER_IP -t 60 -i 3'      |\n");
    printf(" |                                                     |\n");
    printf(" =======================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}