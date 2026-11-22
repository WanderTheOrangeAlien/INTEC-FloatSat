#include "uart_module.h"

#if (UART_MODULE_VER == 2)

#define UART_PARSING_HEADER     0
#define UART_PARSING_BODY       1
#define UART_PARSING_ERROR      2

static void UART_Task(void *params);
static void handle_floatsat_telemetry(uart_module_handle_t *handle, size_t rx_len);
static inline bool is_packet_type(uint8_t byte);
static void on_packet_rx_complete(uart_module_handle_t *handle, size_t start, size_t end, uint8_t type);
static const char *uart_event_type_to_str(uart_event_type_t event_type);


static const char LOG_TAG[] = "UART";

static debug_buffer_t dbg_buffer = {
    .buffer_size = 32768U,
    .send_on_full = true,
    .active = false,
};

static const size_t packet_lengths[] = PACKET_LEN_ARR();

esp_err_t UART_Init(uart_module_handle_t *handle)
{
    (void)packet_lengths;

    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }
    if(!handle->config.on_rx_complete || !handle->config.on_info_complete){
        ESP_LOGE(LOG_TAG,"Error initializing. NULL callbacks given");
    }

    esp_err_t ret = DbgBuffer_Init(&dbg_buffer);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(LOG_TAG,"Initializing UART!");

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
        ESP_LOGE(LOG_TAG,"Error allocating rx buffer");
        return ESP_ERR_NO_MEM;
    }

    if(!handle->info_buffer){
        ESP_LOGE(LOG_TAG,"Error allocating info buffer");
        return ESP_ERR_NO_MEM;
    }

    handle->rx_idx = 0U;
    handle->rx_state = UART_PARSING_HEADER;

    xTaskCreate(UART_Task, "Task_UART", UART_TASK_STACK_DEPTH, handle, 
                UART_TASK_PRIORITY, &handle->task_handle);

    ESP_LOGI(LOG_TAG,"UART Initialized!");

 

    return ESP_OK;
}

esp_err_t UART_SendFloatsatCmd(uart_module_handle_t *handle, const char *command)
{
    if(!handle || !command){
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[255] = {0};

    floatsat_msg_header_t header = {
        .sync = CMD_MSG_SYNC,
        .len = strlen(command),
        .crc = 0U
    };

    memcpy(buffer, &header, sizeof(floatsat_msg_header_t));
    memcpy(buffer + sizeof(floatsat_msg_header_t), command, header.len);
    uart_write_bytes(handle->config.uart_port_num, buffer, sizeof(floatsat_msg_header_t) + header.len);

    return ESP_OK;

}

static void UART_Task(void *params)
{
    uart_module_handle_t *handle = (uart_module_handle_t*)params;
    uart_event_t event;
    
    ESP_LOGI(LOG_TAG,"UART task start");
    while(1){

        if(xQueueReceive(handle->uart_queue, &event, portMAX_DELAY)){
            DbgBuffer_Printf(&dbg_buffer, "=======QUEUE EVENT (%s)=======\n",uart_event_type_to_str(event.type));
            // ESP_LOGI(LOG_TAG,"UART event. Type = %s(%u). Size = %u",uart_event_type_to_str(event.type),event.type, event.size);

            switch (event.type)
            {
            case UART_DATA:
                if(handle->received_break && event.size == 1U){
                    DbgBuffer_Printf(&dbg_buffer, " =====> Discarding BREAK byte\n");
                    uint8_t dummy;
                    uart_read_bytes(handle->config.uart_port_num, &dummy, 1U, portMAX_DELAY);
                    handle->received_break = false;
                    break;
                }


                handle_floatsat_telemetry(handle, event.size);
                break;

            case UART_FIFO_OVF:
                uart_flush_input(handle->config.uart_port_num);
                handle->rx_state = UART_PARSING_HEADER;    
                ESP_LOGE(LOG_TAG,"Buffer overflow!");
                break;

            case UART_BREAK:
                handle->received_break = true;
                break;

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
    static size_t idx = 0U, packet_start = 0U, packet_end = 0U;
    uint16_t end_marker = 0x0000;

    uart_read_bytes(handle->config.uart_port_num, &handle->rx_buffer[handle->rx_idx], rx_len, portMAX_DELAY);
    handle->rx_idx += rx_len;
    DbgBuffer_Printf(&dbg_buffer, "================= Received %u bytes =================\n",rx_len);

    while(idx < handle->rx_idx){

        switch (handle->rx_state)
        {
        case UART_PARSING_HEADER:
            handle->current_packet_type = handle->rx_buffer[idx];
            if(!is_packet_type(handle->current_packet_type)){
                DbgBuffer_Printf(&dbg_buffer, "(Header) Invalid packet type 0x%02x at idx = %u\nDump: ",handle->current_packet_type, idx);
                
                size_t dbg_idx = (idx > 10U) ? idx - 10U : 0U;
                DbgBuffer_PrintHex(&dbg_buffer, &handle->rx_buffer[dbg_idx], (uint16_t)20U);
                
                
                DbgBuffer_Printf(&dbg_buffer, "\n");
                
                handle->rx_state = UART_PARSING_ERROR;
                break;
            }

            handle->current_packet_bytes = packet_lengths[handle->current_packet_type];
            DbgBuffer_Printf(&dbg_buffer, "Got packet type: 0x%02x at idx = %u. Trying ot read %u bytes\n",
                handle->current_packet_type, idx, handle->current_packet_bytes);

            
            handle->rx_state = UART_PARSING_BODY;
            packet_start = idx;
            __attribute__((fallthrough));
        
        case UART_PARSING_BODY:
            // Try to advance handle->current_packet_bytes
            if(idx + handle->current_packet_bytes <= handle->rx_idx){
                DbgBuffer_Printf(&dbg_buffer, "\tRead %u bytes from idx = %u (%u bytes in buffer)\n",
                    handle->current_packet_bytes, idx, rx_len);
                packet_end = idx + handle->current_packet_bytes;
                
                // All packets have a 2-byte end marker. Check for these last two bytes
                memcpy(&end_marker, &handle->rx_buffer[packet_end - PACKET_END_MARKER_LEN], PACKET_END_MARKER_LEN);

                if(end_marker != PACKET_END_MARKER_VAL){
                    // Invalid packet
                    DbgBuffer_Printf(&dbg_buffer, "\tERROR: Invalid end marker\n");
                    handle->rx_state = UART_PARSING_ERROR;
                    break;
                }
                DbgBuffer_Printf(&dbg_buffer, "\tEnd mark read successfully\n");
                on_packet_rx_complete(handle, packet_start, packet_end, handle->current_packet_type);

                /*
                    TODO NEXT:
                    - Implement on_packet_rx_complete
                    - Implemente recovery
                    - Add debug printfs with the debug buffer
                */

                idx += handle->current_packet_bytes;
                handle->rx_state = UART_PARSING_HEADER;

                if(idx == handle->rx_idx){
                    // No more to read. We can safely reset the buffer
                    DbgBuffer_Printf(&dbg_buffer, "\tResetting buffer\n");
                    memset(handle->rx_buffer, 0, handle->rx_idx);
                    handle->rx_idx = 0U;
                    idx = 0U;
                }

            }else{
                DbgBuffer_Printf(&dbg_buffer, "\tWaiting for more bytes (%u bytes in buffer)\n", rx_len);
                // we have to receive more bytes 
                return;
            }

            break;

        case UART_PARSING_ERROR:
            // Manually advance until a valid packet is found or we must receive more bytes
            DbgBuffer_Printf(&dbg_buffer, "\t--------------Entered Recovery--------------\n");
            DbgBuffer_Printf(&dbg_buffer, "\t\tStarted idx = %u\n", idx);
            
            do{
                handle->current_packet_type = handle->rx_buffer[++idx];
            }while(!is_packet_type(handle->current_packet_type) && idx < handle->rx_idx);

            if(idx >= handle->rx_idx){
                DbgBuffer_Printf(&dbg_buffer, "\t\tWaiting fore more bytes\n");
                // Receive more bytes
                return;
            }

            // Found a valid packet byte. Check the end marker
            DbgBuffer_Printf(&dbg_buffer, "\t\tFound valid packet type 0x%02x at idx = %u\n", handle->current_packet_type, idx);
            
            handle->current_packet_bytes = packet_lengths[handle->current_packet_type];
            uint16_t marker_val = 0x0000;
            if(idx + handle->current_packet_bytes <= handle->rx_idx){
                
                memcpy(&marker_val, 
                    &handle->rx_buffer[idx + handle->current_packet_bytes - PACKET_END_MARKER_LEN],
                    PACKET_END_MARKER_LEN );
                if(marker_val == PACKET_END_MARKER_VAL){
                    // Found a valid packet. 
                    DbgBuffer_Printf(&dbg_buffer, "\t\tValid end marker: 0x%04x\n", marker_val);
                    handle->rx_state = UART_PARSING_BODY;
                    packet_start = idx;
                    DbgBuffer_Printf(&dbg_buffer, "\t--------------Exited Recovery--------------\n");
                    break;

                }else{
                    DbgBuffer_Printf(&dbg_buffer, "\t\tInvalid end marker: 0x%04x\n", marker_val);
                    idx++;
                }


            }else{
                return;
            }
            break;



        default:
            break;
        }
    }   

}



static void on_packet_rx_complete(uart_module_handle_t *handle, size_t start, size_t end, uint8_t type)
{
    // printf("\tWith idx=%u. Got data: ",handle->rx_idx);
    // print_str_with_hex((char*)handle->rx_buffer, handle->rx_idx);
    // printf("\n");


    switch (handle->current_packet_type)
    {
    case PACKET_TYPE_FAST:
    case PACKET_TYPE_EXTENDED:
    case PACKET_TYPE_DEBUG:
    case PACKET_TYPE_CMD:
        if(handle->config.on_rx_complete){
            handle->config.on_rx_complete(&handle->rx_buffer[start], end - start, type);
        }
        break;
    
    case PACKET_TYPE_INFO_HEADER:{
        // Update the handle state to start receiving info packet chunks
        info_packet_header_t *header = (info_packet_header_t*)&handle->rx_buffer[start];
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
                &handle->rx_buffer[start + 2U], // Packet is: [packet_type, n_chunk, data...]
                copy_len);

        // Info complete
        if(is_last_chumk && handle->config.on_info_complete){
            handle->config.on_info_complete(
                handle->info_buffer, 
                handle->info_chunk_idx * INFO_CHUNK_DATA_SIZE + handle->last_chunk_size);            
        }

        handle->info_chunk_idx++;
        break;
    }

    default:
        break;
    }


}

static inline bool is_packet_type(uint8_t byte)
{
    return byte < (sizeof(packet_lengths) / sizeof(packet_lengths[0]));
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


#endif