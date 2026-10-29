#include <stdio.h>
#include <ctype.h>

#include "wifi_module.h"
#include "uart_module.h"
#include "websocket_module.h"
#include "project_defs.h"

// #define WIFI_DISABLE    1

static esp_err_t on_ws_rx(uint8_t *buffer, size_t len);
static void on_uart_rx(uint8_t *buffer, size_t len, uint8_t packet_type);
static void on_uart_info(uint8_t *info_buffer, size_t len);
static void print_str_with_hex(char buffer[], size_t len);

static void print_hex(uint8_t *buffer, size_t len);


static wifi_module_config_t wifi_config = {
    .ssid       = "La Cueva del Dragon",
    .password   = "ElVerdeTalisman29",
    // .ssid       = "Miguel iPhone",
    // .password   = "Maup1234",
    .wifi_mode  = WIFI_MODE_STA
};

static uart_module_handle_t uart_handle = {
    .config = {
        .rx_buff_size       = MAIN_UART_RX_BUFFER_SIZE,
        .tx_buff_size       = MAIN_UART_TX_BUFFER_SIZE,
        .info_buffer_len    = MAIN_UART_INFO_BUFFER_SIZE,
        .uart_port_num      = MAIN_UART_PORT_NUM,
        .pin_rx             = MAIN_UART_PIN_RX,
        .pin_tx             = MAIN_UART_PIN_TX,
        .uart_queue_size    = MAIN_UART_QUEUE_SIZE,

        .on_rx_complete     = on_uart_rx,
        .on_info_complete   = on_uart_info,

        .uart_config = {
            .baud_rate      = MAIN_UART_BAUDRATE,
            .data_bits      = UART_DATA_8_BITS,
            .parity         = UART_PARITY_DISABLE,
            .stop_bits      = UART_STOP_BITS_1,
            .flow_ctrl      = UART_HW_FLOWCTRL_DISABLE,
        }
    }
};

static ws_handle_t ws_handle = {
    .on_rx = on_ws_rx
};

static esp_err_t on_ws_rx(uint8_t *buffer, size_t len)
{
    printf("WS: ");
    print_str_with_hex((char*)buffer,len);
    printf("\n");

    uart_write_bytes(uart_handle.config.uart_port_num, buffer, len);

    return ESP_OK;
}

static void on_uart_rx(uint8_t *buffer, size_t len, uint8_t packet_type)
{
    // printf("Sending UART data through websocket. (%zu) bytes!\n",len);
    // print_hex(buffer, len);
    // printf("\n");

    if(packet_type == PACKET_TYPE_INFO_CHUNK){
        // printf("Chunk contents: \n%.*s\n",len, (char*)buffer);
        return;
    }

#ifndef WIFI_DISABLE
    Websocket_Send(&ws_handle, buffer, len, HTTPD_WS_TYPE_BINARY);
#endif
}

static void on_uart_info(uint8_t *info_buffer, size_t len)
{
    // printf("[%s] Got info buffer with %zu bytes\n",__func__, len);
    Websocket_Send(&ws_handle, info_buffer, len, HTTPD_WS_TYPE_TEXT);
}

void app_main(void)
{
    ESP_ERROR_CHECK(UART_Init(&uart_handle));
#ifndef WIFI_DISABLE
    ESP_ERROR_CHECK(Wifi_Init(&wifi_config));
    ESP_ERROR_CHECK(Websocket_Init(&ws_handle));
#endif
}


static void print_hex(uint8_t *buffer, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ",buffer[i]);
    }
}

static void print_str_with_hex(char buffer[], size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        
        if(isprint(buffer[i])){
            printf("%c",buffer[i]);
        }else{
            printf("{%02X}",buffer[i]);
        }

    }
    
}