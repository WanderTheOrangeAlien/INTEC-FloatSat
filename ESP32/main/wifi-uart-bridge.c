#include <stdio.h>

#include "wifi_module.h"
#include "uart_module.h"
#include "websocket_module.h"
#include "project_defs.h"

static esp_err_t on_ws_rx(uint8_t *buffer, size_t len);
static void on_uart_rx(uint8_t *buffer, size_t len);
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
        .uart_port_num      = MAIN_UART_PORT_NUM,
        .pin_rx             = MAIN_UART_PIN_RX,
        .pin_tx             = MAIN_UART_PIN_TX,
        .uart_queue_size    = MAIN_UART_QUEUE_SIZE,

        .on_rx_complete     = on_uart_rx,

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
    printf("WS: %.*s\n",len, buffer);
    return ESP_OK;
}

static void on_uart_rx(uint8_t *buffer, size_t len)
{
    // printf("Sending UART data through websocket. (%zu) bytes!\n",len);
    // print_hex(buffer, len);
    // printf("\n");
    Websocket_Send(&ws_handle, buffer, len);
}

void app_main(void)
{
    UART_Init(&uart_handle);
    Wifi_Init(&wifi_config);
    Websocket_Init(&ws_handle);
}


static void print_hex(uint8_t *buffer, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ",buffer[i]);
    }
}