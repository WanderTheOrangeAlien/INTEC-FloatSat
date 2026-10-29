#ifndef WEBSOCKET_MODULE_H
#define WEBSOCKET_MODULE_H

#include "esp_err.h"

#include "esp_http_server.h"
#include "esp_log.h"

#define WEBSOCKET_MAX_CLIENTS   4


typedef esp_err_t (*ws_on_rx_callback_t)(uint8_t *buffer, size_t len); 

typedef struct ws_client_t {
    httpd_handle_t hd;
    int fd;
    bool connected;
}ws_client_t;

typedef struct websocket_config_t{
    
}websocket_config_t;

typedef struct ws_handle_t {
    httpd_handle_t server_handle;

    ws_client_t clients[WEBSOCKET_MAX_CLIENTS];
    uint8_t n_clients;

    ws_on_rx_callback_t on_rx; 

}ws_handle_t;



esp_err_t Websocket_Init(ws_handle_t *handle);
esp_err_t Websocket_Send(ws_handle_t *handle, uint8_t *buffer, size_t len, httpd_ws_type_t type);

#endif