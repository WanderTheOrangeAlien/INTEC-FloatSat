#include <stdio.h>
#include "websocket_module.h"

static esp_err_t websocket_handler(httpd_req_t *req);
static esp_err_t ws_on_connect(httpd_req_t *req);

static const char *TAG = "WEBSOCKET";

static httpd_uri_t uri_websocket = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = websocket_handler,
    .user_ctx   = NULL,
    .is_websocket = true,
    .ws_post_handshake_cb = ws_on_connect
};




static esp_err_t websocket_handler(httpd_req_t *req)
{
    ws_handle_t *handle = (ws_handle_t*)req->user_ctx;

    httpd_ws_frame_t ws_pkt = {0};
    uint8_t *rx_buffer = NULL;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;

    ESP_LOGI(TAG,"Got request with type 0x%02x", req->method);

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0U); // max_size set to 0 to get the length
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with 0x%04x", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if(ws_pkt.len){
        rx_buffer = calloc(1, ws_pkt.len + 1); // +1 For the NULL terminator (Probably not needed becuase this is a binary)
    
        if(!rx_buffer){
            ESP_LOGE(TAG, "Failed to allocate memory for the rx_buffer");
            return ESP_ERR_NO_MEM;
        }

        ws_pkt.payload = rx_buffer;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if(ret != ESP_OK){
            ESP_LOGE(TAG, "Error receiving the websocket frame: 0x%04x",ret);
            free(rx_buffer);
            return ret;
        }
        if(handle->on_rx){
            handle->on_rx(rx_buffer, ws_pkt.len);
        }
    
    }

    free(rx_buffer);
    return ESP_OK;
}

static esp_err_t ws_on_connect(httpd_req_t *req)
{
    ws_handle_t *handle = (ws_handle_t*)req->user_ctx;

    printf("%s: New client connected!\n",__func__);

    if(handle->n_clients < WEBSOCKET_MAX_CLIENTS){
        handle->clients[handle->n_clients].fd = httpd_req_to_sockfd(req);
        handle->clients[handle->n_clients].hd = req->handle;
        handle->clients[handle->n_clients].connected = true;
        // printf("req handle = 0x%8p. Handle's server handle: %8p\n",req->handle, handle->server_handle);
        printf("Added new client with flie descriptor: %d\n",handle->clients[handle->n_clients].fd);

    }else{
        ESP_LOGE(TAG,"%s: max clients reached. Current connection will be closed");
    }

    return ESP_OK;
}

esp_err_t Websocket_Init(ws_handle_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }
 
    // Start the web server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    esp_err_t ret = httpd_start(&handle->server_handle, &config);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Error in httpd_start()");
        return ret;
    }

    // Register uri handlers
    uri_websocket.user_ctx = (void*)handle;
    httpd_register_uri_handler(handle->server_handle, &uri_websocket);


    return ESP_OK;
}

/// @brief Sends data to the first client
/// @param handle Websocket module handle
/// @param buffer message to send
/// @param len Length of the message
/// @return 
esp_err_t Websocket_Send(ws_handle_t *handle, uint8_t *buffer, size_t len)
{
    if(!handle || ! buffer || len == 0U){
        return ESP_ERR_INVALID_ARG;
    }

    if(!handle->clients[0].connected){
        ESP_LOGE(TAG,"Tried to send, but no client connected");
        return ESP_FAIL;
    }

    httpd_ws_frame_t frame = {
        .len = len,
        .payload = buffer,
        .type = HTTPD_WS_TYPE_BINARY
    };

    httpd_ws_send_data_async(handle->server_handle, handle->clients[0].fd, 
        &frame, NULL, NULL);

    

    return ESP_OK;
}