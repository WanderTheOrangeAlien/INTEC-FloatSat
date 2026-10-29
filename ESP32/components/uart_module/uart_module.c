#include <stdio.h>
#include <ctype.h>
#include "driver/gpio.h"
#include "uart_module.h"

static void handle_floatsat_telemetry(uart_module_handle_t *handle, size_t rx_len);
static void on_packet_rx_complete(uart_module_handle_t *handle);
static const char *uart_event_type_to_str(uart_event_type_t event_type);

static void print_str_with_hex(char buffer[], size_t len);

static const char *TAG = "UART";

static const size_t packet_lengths[] = {
    [PACKET_TYPE_FAST]              25U,
    [PACKET_TYPE_EXTENDED]          33U,
    [PACKET_TYPE_INFO_HEADER]       13U,
    [PACKET_TYPE_INFO_CHUNK]        INFO_CHUNK_DATA_SIZE + 2U
};

void Task_UART(void *params)
{
    uart_module_handle_t *handle = (uart_module_handle_t*)params;
    uart_event_t event;
    size_t rx_size;
    
    ESP_LOGI(TAG,"UART task start");
    while(1){

        if(xQueueReceive(handle->uart_queue, &event, portMAX_DELAY)){
            // ESP_LOGI(TAG,"UART event. Type = %s(%u). Size = %u",uart_event_type_to_str(event.type),event.type, event.size);

            switch (event.type)
            {
            case UART_DATA:
                handle_floatsat_telemetry(handle, event.size);
                break;

            case UART_FIFO_OVF:
                uart_flush_input(handle->config.uart_port_num);
            default:
                break;
            }

        }

    }
    free(handle->rx_buffer);
    handle->rx_buffer = NULL;
    vTaskDelete(NULL);
}

static void handle_floatsat_telemetry(uart_module_handle_t *handle, size_t rx_len)
{
    // ESP_LOGI(TAG, "UART DATA event. rx_len=%u",rx_len);
    bool packet_pending = true; // Whether there are more bytes to read after receiving the current packet
    
    while(packet_pending)
    {
        switch (handle->rx_state)
        {
        case UART_RX_STATE_WAIT_HEADER:
            memset(handle->rx_buffer, 0, handle->config.rx_buff_size);
            handle->rx_idx = 0U;

            // Receive one byte, this will tell the packet type
            uart_read_bytes(handle->config.uart_port_num, handle->rx_buffer, 1U, portMAX_DELAY);
            handle->current_packet_type = handle->rx_buffer[0];

            // Validate packet type
            if(handle->current_packet_type >= sizeof(packet_lengths) / sizeof(packet_lengths[0])){
                ESP_LOGE(TAG,"Invalid packet type: %u. rx_len = %lu",
                     handle->current_packet_type, rx_len);
                memset(handle->rx_buffer,0,handle->config.rx_buff_size);
                uart_flush_input(handle->config.uart_port_num);
                return;
            }
            
            handle->current_packet_bytes = packet_lengths[handle->current_packet_type] - 1U; //Subtract 1 because we already got the first byte
            handle->rx_state = UART_RX_STATE_WAIT_BODY;
            handle->rx_idx = 1U;
            // printf("[TYPE = %u]\n",handle->current_packet_type);

            // If the uart event length is 1, there are no more bytes to read. 
            // Otherwise, fall through to the UART_RX_STATE_WAIT_BODY case
            if(rx_len == 1){
                // The STM32 sends a single 0x00 byte after UARTT initialization.
                // This should be the ONLY instance where a single 0x00 byte is sent
                // Interpret this case as the init glitch and ignore

                if(handle->current_packet_type == 0x00){
                    printf("NULL init byte detected. Rearming...\n");
                    handle->rx_state = UART_RX_STATE_WAIT_HEADER;
                }

                packet_pending = false;
                break;
            }
            rx_len--; 
            __attribute__((fallthrough));

        case UART_RX_STATE_WAIT_BODY:{
            // printf("Receiving packet body for type " LOG_COLOR_I "%u. " LOG_RESET_COLOR " rx_len = %zu\n",handle->current_packet_type, rx_len);
            
            int remaining = handle->current_packet_bytes - handle->rx_idx + 1U;

            // the amount of bytes to wait is different during the last info chunk
            if(handle->current_packet_type == PACKET_TYPE_INFO_CHUNK && 
                handle->info_chunk_idx + 1U >= handle->info_chunks_to_wait)
            {
                remaining = handle->last_chunk_size + 1U; // Add 1 to include the packet type byte
                // printf("[Last chunk] Waiting %u bytes\n",remaining);
            }
            
            if(remaining < 0){
                ESP_LOGE(TAG,"Error, invalid parser state!");
                return;
            }

            if(rx_len <= remaining){
                // printf("rx_len <= remaining. (%u < %u). idx=%u\n",rx_len, remaining, handle->rx_idx);
                // Receiive bytes normally
                uart_read_bytes(handle->config.uart_port_num, 
                    &handle->rx_buffer[handle->rx_idx], rx_len, portMAX_DELAY);
                    
                handle->rx_idx += rx_len;
                
                // Is this right? Should we handle this in the else? It's almost the same code
                if(remaining == rx_len){
                    // All bytes received
                    handle->rx_state = UART_RX_STATE_WAIT_HEADER;
                    packet_pending = false;
                    on_packet_rx_complete(handle);
                }
                return; // Exit the function to wait for more bytes
                    
            }else{
                // printf("\trx_len > remaining. (%u > %u). idx=%u\n",rx_len, remaining, handle->rx_idx);
                // Another packet arrived immediately
                // Process the current packet and continue 
                

                uart_read_bytes(handle->config.uart_port_num,
                    &handle->rx_buffer[handle->rx_idx], remaining, portMAX_DELAY);
                rx_len -= remaining;
                handle->rx_idx += remaining;

                on_packet_rx_complete(handle);

                handle->rx_state = UART_RX_STATE_WAIT_HEADER;
                packet_pending = true;
                
            }                   
            break;
        }

        
        default:
            break;
        }
}

}

static void on_packet_rx_complete(uart_module_handle_t *handle)
{
    // printf("\tWith idx=%u. Got data: ",handle->rx_idx);
    // print_str_with_hex((char*)handle->rx_buffer, handle->rx_idx);
    // printf("\n");
    switch (handle->current_packet_type)
    {
    case PACKET_TYPE_FAST:
        if(handle->config.on_rx_complete){
            handle->config.on_rx_complete(handle->rx_buffer, handle->rx_idx, PACKET_TYPE_FAST);
        }
        break;
    case PACKET_TYPE_EXTENDED:
        if(handle->config.on_rx_complete){
            handle->config.on_rx_complete(handle->rx_buffer, handle->rx_idx, PACKET_TYPE_EXTENDED);
        }
        break;
    
    case PACKET_TYPE_INFO_HEADER:{
        // Update the handle state to start receiving info packet chunks
        info_packet_header_t *header = (info_packet_header_t*)handle->rx_buffer;
        handle->info_chunks_to_wait = header->nchunks;
        handle->last_chunk_size = header->last_chunk_size;
        handle->info_chunk_idx = 0U;
        memset(handle->info_buffer, 0, handle->config.info_buffer_len);
        // printf("========== Got info header. Chunks: %u. Last chunk Size: %u\n",header->nchunks,header->last_chunk_size);
        break;
    }

    case PACKET_TYPE_INFO_CHUNK: { 
        // Add to the info buffer
        bool is_last_chumk = (handle->info_chunk_idx + 1U >= handle->info_chunks_to_wait); 

        size_t copy_len = (is_last_chumk) ? 
            handle->last_chunk_size : INFO_CHUNK_DATA_SIZE;

        // printf("==== Info chunk #%u! ====\n\n",handle->info_chunk_idx);

        memcpy(&handle->info_buffer[INFO_CHUNK_DATA_SIZE * handle->info_chunk_idx],
                &handle->rx_buffer[2], // Packet is: [packet_type, n_chunk, data...]
                copy_len);

        if(is_last_chumk && handle->config.on_rx_complete){
            // printf(" ================= Info packet complete! =================\n");
            handle->config.on_info_complete(
                handle->info_buffer, 
                handle->info_chunk_idx * INFO_CHUNK_DATA_SIZE + handle->last_chunk_size);
            
        }

        handle->info_chunk_idx++;
    }

    default:
        break;
    }


}

esp_err_t UART_Init(uart_module_handle_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }
    if(!handle->config.on_rx_complete || !handle->config.on_info_complete){
        ESP_LOGE(TAG,"Error initializing. NULL callbacks given");
    }


    ESP_LOGI(TAG,"Initializing UART!");

    uart_module_config_t *config = &handle->config; 

    handle->uart_queue = xQueueCreate(config->uart_queue_size, sizeof(uart_event_t));

    ESP_ERROR_CHECK(uart_driver_install( config->uart_port_num, config->rx_buff_size, 
                         config->tx_buff_size, config->uart_queue_size, 
                         &handle->uart_queue, 0));
    
    ESP_ERROR_CHECK(uart_param_config(config->uart_port_num, &config->uart_config));

    ESP_ERROR_CHECK(uart_set_pin(config->uart_port_num, 5, 4,
                        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ));
    gpio_pullup_en(config->pin_rx);

    handle->rx_buffer = (uint8_t*)calloc(1,handle->config.rx_buff_size);
    handle->info_buffer = (uint8_t*)calloc(1, handle->config.info_buffer_len);

    if(!handle->rx_buffer){
        ESP_LOGE(TAG,"Error allocating rx buffer");
        return ESP_ERR_NO_MEM;
    }

    if(!handle->info_buffer){
        ESP_LOGE(TAG,"Error allocating info buffer");
        return ESP_ERR_NO_MEM;
    }

    handle->rx_idx = 0U;
    handle->rx_state = UART_RX_STATE_WAIT_HEADER;

    xTaskCreate(Task_UART, "Task_UART", UART_TASK_STACK_DEPTH, handle, 
                UART_TASK_PRIORITY, &handle->task_handle);

    ESP_LOGI(TAG,"UART Initialized!");

    return ESP_OK;
}

static const char *uart_event_type_to_str(uart_event_type_t event_type)
{
    static const char *str[UART_EVENT_MAX + 1] = {
        "UART_DATA",
        "UART_BREAK",
        "UART_BUFFER_FULL",
        "UART_FIFO_OVF",
        "UART_FRAME_ERR",
        "UART_PARITY_ERR",
        "UART_DATA_BREAK",
        "UART_PATTERN_DET",
#if SOC_UART_SUPPORT_WAKEUP_INT
        "UART_WAKEUP",
#endif
        "UART_EVENT_MAX",
    };

    if(event_type >= 0 && event_type < UART_EVENT_MAX){
        return str[event_type];
    }else{
        return "INVALID";
    }

}


#pragma region misc


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

#pragma endregion