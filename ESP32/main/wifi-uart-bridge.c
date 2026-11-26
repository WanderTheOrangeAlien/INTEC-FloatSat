#include <stdio.h>
#include <ctype.h>

#include "wifi_module.h"
#include "uart_module.h"
#include "websocket_module.h"
#include "project_defs.h"
#include "fs_packets.h"
#include "mdns.h"

#include "freertos/queue.h"

// #include "Arducam_OV2640.h"

// #define WIFI_DISABLE    1



#define ESP_WIFI_MODE WIFI_MODE_AP

extern const uint8_t img1_jpg_start[]    asm("_binary_img1_jpg_start");
extern const uint8_t img1_jpg_end[]      asm("_binary_img1_jpg_end");


static esp_err_t on_ws_rx(uint8_t *buffer, size_t len);
static void on_uart_rx(uint8_t *buffer, size_t len, uint8_t packet_type);
static void on_uart_info(uint8_t *info_buffer, size_t len);

static void handle_esp_command(esp_cmd_t *cmd);
static void Camera_Task(void *params);
static void on_camera_chunk_send(esp_err_t err, int socket, void *arg);

static void print_str_with_hex(char buffer[], size_t len);
static void print_hex(uint8_t *buffer, size_t len);

static const char LOG_TAG[] = "MAIN";

static volatile bool initialized = false;

static wifi_module_config_t wifi_config = {
#if defined(CONFIG_WIFI_MODULE_MODE_STA)
    .ssid      = CONFIG_STA_WIFI_SSID,
    .password  = CONFIG_STA_WIFI_PASSWORD,
    .wifi_mode = WIFI_MODE_STA,
#elif defined(CONFIG_WIFI_MODULE_MODE_AP)
    .ssid      = CONFIG_AP_WIFI_SSID,
    .password  = CONFIG_AP_WIFI_PASSWORD,
    .wifi_mode = WIFI_MODE_AP,
#else
#error "No WiFi mode selected"
#endif
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


#ifdef ARDUCAM_OV2640_H
ov2640_handle_t camera = {
    .i2c_config = {
        .clk_freq_hz        =   CAMERA_I2C_CLK_FREQ,
        .i2c_port           =   CAMERA_I2C_PORT,
        .pin_scl            =   CAMERA_I2C_PIN_SCL,
        .pin_sda            =   CAMERA_I2C_PIN_SDA, 
        .timeout_ms         =   CAMERA_I2C_TIMEOUT_MS,
    },
    .spi_config = {
        .pin_miso           =   CAMERA_SPI_PIN_MISO,
        .pin_mosi           =   CAMERA_SPI_PIN_MOSI,
        .pin_sclk           =   CAMERA_SPI_PIN_SCLK,
        .pin_cs             =   CAMERA_SPI_PIN_CS,
        .max_transfer_size  =   CAMERA_CHUNK_SIZE,
        .queue_size         =   CAMERA_SPI_QUEUE_SIZE,
        .host               =   CAMERA_SPI_HOST
    }
};
#endif
static uint32_t photo_id                = 0U;
QueueHandle_t camera_cmd_queue          = NULL;
TaskHandle_t camera_task_handle         = NULL;
SemaphoreHandle_t camera_read_semphr    = NULL;

static ws_handle_t ws_handle = {
    .on_rx = on_ws_rx
};



static esp_err_t on_ws_rx(uint8_t *buffer, size_t len)
{
    printf("WS: ");
    print_str_with_hex((char*)buffer,len);
    printf("\n");

    if(strncmp((char*)&buffer[6], "esp-restart", len) == 0){
        ESP_LOGI("System","Resetting ESP...");
        esp_restart();
        return ESP_OK;
    }

    uart_write_bytes(uart_handle.config.uart_port_num, buffer, len);

    return ESP_OK;
}   

static uint32_t dbg_count = 0U; 

static void on_uart_rx(uint8_t *buffer, size_t len, uint8_t packet_type)
{
    // printf("Sending UART data through websocket. (%zu) bytes!\n",len);
    // print_hex(buffer, len);
    // printf("\n");
    if(!initialized){
        return;
    }

    if(packet_type == PACKET_TYPE_CMD){
        handle_esp_command((esp_cmd_t*)&buffer[1]);
        return;
    }




#ifndef WIFI_DISABLE
    if(packet_type != PACKET_TYPE_INFO_CHUNK && packet_type != PACKET_TYPE_INFO_HEADER){
        Websocket_Send(&ws_handle, buffer, len, HTTPD_WS_TYPE_BINARY);
    }
#endif
}

static void on_uart_info(uint8_t *info_buffer, size_t len)
{

    if(initialized){
        Websocket_Send(&ws_handle, info_buffer, len, HTTPD_WS_TYPE_TEXT);
    }
}

static void handle_esp_command(esp_cmd_t *cmd)
{
    ESP_LOGI(LOG_TAG,"Received command ID 0x%02x.",
        cmd->cmd_id);

    if(!initialized){
        ESP_LOGE(LOG_TAG,"Error: Bridge not initialized");
        return;
    }

    if(cmd->cmd_id == EXT_CMD_TAKE_PHOTO){
        ESP_LOGI(LOG_TAG,"Taking %u photos...",cmd->photo_args.photo_amount);

        xQueueSend(camera_cmd_queue, cmd, 100U);
        // xTaskNotify(camera_task_handle, cmd->args.photo_amount, eSetValueWithOverwrite);
    }

}

static void Camera_Task(void *params)
{
    (void)params;
    uint8_t *chunk_buffer = NULL;
    size_t chunk_idx = 0U, remaining = 0U, xfer_len = 0U, photo_idx = 0U;
    size_t loop_iters = 0U;
    uint32_t notification_val = 0U;
    esp_cmd_t current_cmd = {0};

    esp_err_t ret = ESP_OK;

    chunk_buffer = heap_caps_aligned_calloc(sizeof(uint32_t), 
                                            1U, 
                                            CAMERA_CHUNK_SIZE + offsetof(camera_packet_chunk_t, data), 
                                            MALLOC_CAP_DMA);
    if(!chunk_buffer){
        ESP_LOGE(LOG_TAG, "Error allocating camera chunk buffer!");
        while(1){
            vTaskDelay(10);
        }
    }

    while(1){
        // notification_val = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        xQueueReceive(camera_cmd_queue, &current_cmd, portMAX_DELAY);
        photo_idx = 0;

        ESP_LOGI(LOG_TAG, 
            "Received photo command: {\n"
                "\tyaw = %f\n"
                "\ttimestamp = %02u:%02u:%02u\n"
                "\tphoto_amount = %u\n"
            "}",
            current_cmd.photo_args.yaw, 
            current_cmd.photo_args.timestamp.hours, current_cmd.photo_args.timestamp.minutes, current_cmd.photo_args.timestamp.seconds, 
            current_cmd.photo_args.photo_amount    
        );


        while(photo_idx++ < current_cmd.photo_args.photo_amount)
        {    
            ESP_LOGI(LOG_TAG, "taking photo #%u", notification_val);
            
#ifdef ARDUCAM_OV2640_H
            ret = OV2640_Capture(&camera, &remaining);
            if(ret != ESP_OK){
                ESP_LOGE(LOG_TAG,"(Camera Task) Camera capture failed");
                goto err;
            }
#else
            remaining = (size_t)(img1_jpg_end - img1_jpg_start);
#endif

            chunk_idx = 0U;
            loop_iters = 0U;

            // Send the header
            camera_packet_header_t header = {
                .type       = PACKET_TYPE_CAMERA_HEADER,
                .nchunks    = (remaining % CAMERA_CHUNK_SIZE == 0) ?
                    remaining / CAMERA_CHUNK_SIZE:
                    remaining / CAMERA_CHUNK_SIZE + 1,            
                .timeout_ms = CAMERA_CHUNK_TIMEOUT_MS,

                .photo_id   = photo_id++,
                #ifdef ARDUCAM_OV2640_H
                .resolution = camera.resolution,
                #endif
                .timestamp  = current_cmd.photo_args.timestamp,
                .yaw        = current_cmd.photo_args.yaw,
            };
        
            Websocket_Send(&ws_handle, (uint8_t*)&header, sizeof(header), HTTPD_WS_TYPE_BINARY);
        
            while(remaining > 0U && ++loop_iters <= CAMERA_READ_MAX_LOOP_ITERS){

                xfer_len = (remaining >= CAMERA_CHUNK_SIZE) ? CAMERA_CHUNK_SIZE : remaining;
                
                #ifdef ARDUCAM_OV2640_H
                // Read image data and store it after the chunk header
                ret = OV2640_ReadChunk( &camera, 
                    &chunk_buffer[offsetof(camera_packet_chunk_t, data)], 
                    xfer_len, &remaining);
                    if(ret != ESP_OK){
                        ESP_LOGE(LOG_TAG,"(Camera Task) Error in chunk #%u", chunk_idx);
                    goto err;
                }
                #else
                memcpy( &chunk_buffer[offsetof(camera_packet_chunk_t, data)], 
                &img1_jpg_start[CAMERA_CHUNK_SIZE * chunk_idx],
                        xfer_len);
                        remaining -= xfer_len;     
                        #endif            
                        // Create the chunk header
                camera_packet_chunk_t chunk_header = {
                    .type   = PACKET_TYPE_CAMERA_CHUNK,
                    .index  = chunk_idx,
                    .len    = xfer_len,
                };
                // Copy the header at the start of the buffer
                memcpy(chunk_buffer, &chunk_header, offsetof(camera_packet_chunk_t, data));
                
                // The callback releases the semaphore
                ret = Websocket_SendNoCopy(&ws_handle, 
                    chunk_buffer, xfer_len + offsetof(camera_packet_chunk_t, data), 
                    HTTPD_WS_TYPE_BINARY,
                    on_camera_chunk_send, camera_read_semphr);
                    
                if(ret != ESP_OK){
                    ESP_LOGE(LOG_TAG,"(Camera Task) Error sending chunk #%u", chunk_idx);
                    goto err;
                }
                
                
                // Wait for the sempahore to be available
                if(xSemaphoreTake(camera_read_semphr, pdMS_TO_TICKS(CAMERA_CHUNK_TIMEOUT_MS))){
                    ++chunk_idx;
                }else{
                    ESP_LOGE(LOG_TAG,"(Camera Task) Error sending chunk #%u  (Sempahore Timeout)", chunk_idx);
                    goto err;
                }    
            }
        }

        if(loop_iters >= CAMERA_READ_MAX_LOOP_ITERS){
            ESP_LOGE(LOG_TAG,"(Camera Task) Max loop iterations reached!");
            goto err;
        }

        if(notification_val <= 1U){
            UART_SendFloatsatCmd(&uart_handle, "sys --invoke-cb on_photo_finish");
            ESP_LOGI(LOG_TAG,"Photos finished!");
        }

        continue;
    err:
        chunk_idx = 0U;
        remaining = 0U;
        ulTaskNotifyValueClear(NULL, UINT_MAX);
        UART_SendFloatsatCmd(&uart_handle, "sys --invoke-cb on_photo_fail");
        ESP_LOGE(LOG_TAG,"Photo failed!");
    }

}

static void on_camera_chunk_send(esp_err_t err, int socket, void *arg)
{
    (void)err;
    (void)socket;
    SemaphoreHandle_t semphr = (SemaphoreHandle_t)arg;

    xSemaphoreGive(semphr);
}

void start_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("floatsat"));
    ESP_ERROR_CHECK(mdns_instance_name_set("INTEC FloatSat"));

}

void app_main(void)
{
    // Check that the camera chunk packet has a header which size if a multiple of 4
    // So the buffer is 4-byte aligned
    static_assert(  (offsetof(camera_packet_chunk_t, data) % 4U) == 0 );

    ESP_ERROR_CHECK(UART_Init(&uart_handle));

#ifdef ARDUCAM_OV2640_H
    ESP_ERROR_CHECK(OV2640_Init(&camera));
#endif
    camera_read_semphr = xSemaphoreCreateBinary();
    camera_cmd_queue = xQueueCreate(5U, sizeof(esp_cmd_t));
    xTaskCreate(Camera_Task, "CAMERA", CAMERA_TASK_STACK_SIZE, NULL, CAMERA_TASK_PRIO, &camera_task_handle);

#ifndef WIFI_DISABLE
    ESP_ERROR_CHECK(Wifi_Init(&wifi_config));
    if(wifi_config.wifi_mode == WIFI_MODE_AP){
        start_mdns();
    }

    ESP_ERROR_CHECK(Websocket_Init(&ws_handle));

    Websocket_WaitForClient(&ws_handle, portMAX_DELAY);

    initialized = true;
    UART_SendFloatsatCmd(&uart_handle, "sys --invoke-cb on_bridge_active");
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