#include "floatsat_telemetry.h"
#include <string.h>
#include <stdbool.h>

#include "floatsat_utils.h"

static floatsat_err_t Telemetry_SendCurrentPacket(telemetry_handle_t *handle);
STATIC floatsat_err_t Telemetry_HandleInfo(telemetry_handle_t *handle);
static floatsat_err_t Telemetry_AddToRegistry(telemetry_handle_t *handle);




// Global array of handle pointers so that HAL interrup callbacks can get
// the handles from the UART instance
static telemetry_handle_t *telemetry_handle_registry[FLOATSAT_UART_NUM] = {0};

static const char *LOG_TAG = "TELEMETRY";

static uint8_t info_buffer[TELEMETRY_INFO_BUFFER_SIZE] = {0};



floatsat_err_t Telemetry_Init(telemetry_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    // TODO: The handle to the task is obtain at the start of the task. Consider removing the guard
    if(!handle->uart || !handle->task_handle){
        return ERR_INVALID_ARG;
    }

    handle->queue = xQueueCreate(TELEMETRY_QUEUE_SIZE, sizeof(telemetry_packet_t*));
    handle->info_semphr = xSemaphoreCreateMutex();

    if(handle->queue == 0U){
        LOGE(LOG_TAG,"Queue could not be created");
        return ERR_INVALID_STATE;
    }

    if(handle->info_semphr == NULL){
        LOGE(LOG_TAG,"Semaphore could not be created");
        return ERR_INVALID_STATE;
    }

    Telemetry_AddToRegistry(handle);

    return ERR_OK;

}




void Task_TelemetryFn(void *args)
{
    telemetry_handle_t *handle = (telemetry_handle_t*)args;
    floatsat_err_t ret = ERR_OK;

    handle->task_handle = xTaskGetCurrentTaskHandle();

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Get start and end times
        handle->task_start_time_us = Util_GetTimeUs();
        handle->task_end_time_us = handle->task_start_time_us 
            + (portTICK_PERIOD_MS * 1000U) - INFO_CHUNK_TIME_MARGIN_US;

        // Start by checking for error conditions
        if(!(handle->packet_type_mask & PACKET_MASK_FAST)){
            LOGE(LOG_TAG,"Task started without fast packet. Invalid state");
            ret = ERR_INVALID_STATE;
            goto err;
        }

        if(handle->packet_type_mask & PACKET_MASK_SLOW){
            // Fast + Slow telemetry, whole packet
            handle->current_packet->type = PACKET_TYPE_EXTENDED;

        }else{
            // Only fast telemetry, first part of packet
            handle->current_packet->type = PACKET_TYPE_FAST;
        }
        GOTO_ON_ERR(Telemetry_SendCurrentPacket(handle), err, ret);
        // clear fast and slow flags
        handle->packet_type_mask &= ~(PACKET_MASK_FAST | PACKET_MASK_SLOW);
        
        // Handle info packet
        if(handle->packet_type_mask & PACKET_MASK_INFO){
            GOTO_ON_ERR(Telemetry_HandleInfo(handle), err, ret);
        }


        continue;
err:
        // Abort transmission. Clear all flags
        LOGE(LOG_TAG, "Error in telemetry task. Transmission was aborted. Error code: 0x%04x", ret);
        handle->packet_type_mask = 0U;

    }

}

static floatsat_err_t Telemetry_SendCurrentPacket(telemetry_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    HAL_StatusTypeDef status = HAL_OK;
    uint8_t tx_len = 0U;


    if(handle->current_packet->type == PACKET_TYPE_FAST){
        // Send only the first 2 fields
        tx_len = offsetof(telemetry_packet_t, slow);
    }else if(handle->current_packet->type == PACKET_TYPE_EXTENDED){
        tx_len = sizeof(telemetry_packet_t);
    }else{
        LOGE(LOG_TAG,"Invalid packet type. Type: %d",handle->current_packet->type);
        return ERR_INVALID_ARG;
    }

    handle->tx_busy = true;
    status = HAL_UART_Transmit_DMA(handle->uart, (uint8_t*)handle->current_packet, tx_len);
    while(handle->tx_busy){;}

    if(status != HAL_OK){
        LOGE(LOG_TAG, "Error in transmission. Status: %d", status);
        return ERR_UART_TX_FAIL;
    }


    return ERR_OK;
}


floatsat_err_t Telemetry_AddFast(telemetry_handle_t *handle, telemetry_packet_fast_t *packet)
{
    if(!handle || !packet){
        return ERR_INVALID_ARG;
    }
    bool last_packet_dropped = false;

    /* TODO: Critical section might not be necesary, but I left it here to state that this block of code 
        must not be interrupted
    */
    taskENTER_CRITICAL(); 
    if(handle->packet_type_mask & PACKET_MASK_FAST){
        // Last fast telemetry was not dispatched. Indicate error but continue  
        // operation
        last_packet_dropped = true;
    }else{
         // Update the mask
        handle->packet_type_mask |= PACKET_MASK_FAST;
    }

    handle->current_packet->timestamp = HAL_GetTick();
    handle->current_packet->fast = *packet;
    taskEXIT_CRITICAL();


    if(last_packet_dropped){
        LOGW(LOG_TAG, "Warning: Last fast telemetry packet was not dispatched. "
            "Last packet will be dropped and will continue with the current one");
    }

    return ERR_OK;
}

floatsat_err_t Telemetry_AddSlow(telemetry_handle_t *handle, telemetry_packet_slow_t *packet)
{
    if(!handle || !packet){
        return ERR_INVALID_ARG;
    }

    taskENTER_CRITICAL();

    if(handle->packet_type_mask & PACKET_MASK_SLOW){
        LOGW(LOG_TAG, "Warning: Last slow telemetry packet was not dispatched. "
            "Last packet will be dropped and will continue with the current one");
    }

    handle->packet_type_mask |= PACKET_MASK_SLOW;

    // The slow packet uses the same timestamp as the preceeding fast packet, so 
    // it is not updated
    handle->current_packet->slow = *packet;
    taskEXIT_CRITICAL();

    return ERR_OK;
}

/// @brief Acquire the info buffer
/// @param handle Pointer to telemetry handle
/// @param out_ptr The pointer to the info buffer will be written here.
/// @return `ERR_OK` if the buffer was acquired successfully. `ERR_RESOURCE_BUSY`
/// if the buffer was already acquired. `ERR_RESOURCE_NOT_AVAILABLE` if the buffer has not
/// been previously emptied by the telemetry task. `ERR_INVALID_ARG` if NULL arguments were passed
floatsat_err_t Telemetry_TakeInfoBuffer(telemetry_handle_t *handle, uint8_t **out_ptr)
{
    if(!handle || !out_ptr){
        return ERR_INVALID_ARG;
    }

    if(handle->info_buffer_not_empty){
        return ERR_RESOURCE_NOT_AVAILABLE;
    }

    if(xSemaphoreTake(handle->info_semphr, 0U)){
        *out_ptr = info_buffer;
        return ERR_OK;
    }else{
        *out_ptr = NULL;
        return ERR_RESOURCE_BUSY;
    }
}

/// @brief Release the info buffer. The pointer given by `Telemetry_TakeInfoBuffer` 
/// will be set to NULL to avoid using it later
/// @param handle 
/// @param ptr Address to the pointer given by  `Telemetry_TakeInfoBuffer`
/// @return `ERR_OK` if the buffer was released successfully, `ERR_INVALID_STATE` 
/// if there was an error releasing the semaphore. `ERR_INVALID_ARG` if NULL arguments were passed
floatsat_err_t Telemetry_GiveInfoBuffer(telemetry_handle_t *handle, uint8_t **ptr)
{
    if(!handle || !ptr){
        return ERR_INVALID_ARG;
    }

    if(*ptr != info_buffer){
        LOGE(LOG_TAG,"Wrong pointer given. Did you modify it after calling Telemetry_TakeInfoBuffer");
        return ERR_INVALID_ARG;
    }

    BaseType_t ret = xSemaphoreGive(handle->info_semphr);

    if(ret == pdTRUE){
        *ptr = NULL;
        // Indicate that an info packet is pending for transmission by raising the flag
        handle->packet_type_mask |= PACKET_MASK_INFO;

        // Indicate that the buffer is not empty
        handle->info_buffer_not_empty = true;

        return ERR_OK;
    }else{
        LOGE(LOG_TAG,"Error releasing info buffer semaphore");
        return ERR_INVALID_STATE;
    }

}

STATIC floatsat_err_t Telemetry_HandleInfo(telemetry_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret;
    // Acquire buffer
    ret = Telemetry_TakeInfoBuffer(handle, &handle->info_bufer);

    if(ret != ERR_OK){
        LOGE(LOG_TAG,"Info packet bit was set, but the info buffer was not released. This is an invalid state!");
        return ERR_INVALID_STATE;
    }

    switch (handle->info_state)
    {
    case INFO_STATE_START:{

        // Create the header
        size_t len = strnlen((char*)handle->info_bufer, TELEMETRY_INFO_BUFFER_SIZE);
        uint8_t last_chunk_size = len % INFO_CHUNK_DATA_SIZE;

        info_packet_header_t header = {
            .type               = PACKET_TYPE_INFO_HEADER,
            .timestamp          = HAL_GetTick(),
            .nchunks            = (last_chunk_size == 0U) ? len / INFO_CHUNK_DATA_SIZE : len / INFO_CHUNK_DATA_SIZE + 1U,
            .last_chunk_size    = last_chunk_size
        };

        //TODO: Calculate CRC

        // Send the header, blocking 
        handle->tx_busy = true;
        

        GOTO_ON_HAL_ERR(HAL_UART_Transmit_DMA(handle->uart, (uint8_t*)&header, sizeof(info_packet_header_t)),
            err, ret);
        while(handle->tx_busy){;}

        handle->chunk_index = 0U;
        handle->nchunks = header.nchunks;
        handle->last_chunk_size = header.last_chunk_size;
        
        handle->info_state = INFO_STATE_TX;
        // Intended fall through. We want to start sending chunks immediately
    }

    case INFO_STATE_TX:{
        bool can_send = (handle->task_end_time_us - Util_GetTimeUs()) > INFO_CHUNK_RX_TIME_US;
        while(can_send){

            info_packet_chunk_t chunk = {
                .type = PACKET_TYPE_INFO_CHUNK,
                .chunk_number = handle->chunk_index,
                .data = &handle->info_bufer[handle->chunk_index * INFO_CHUNK_DATA_SIZE]
            };
            
            // Send the header
            handle->tx_busy = true;
            GOTO_ON_HAL_ERR(
                HAL_UART_Transmit_DMA(handle->uart, (uint8_t*)&chunk, offsetof(info_packet_chunk_t, data)), 
                err, ret);
            while(handle->tx_busy){;}

            // Send the body
            uint16_t tx_len = (handle->chunk_index < handle->nchunks - 1) ? INFO_CHUNK_DATA_SIZE: handle->last_chunk_size;
            handle->tx_busy = true;
            GOTO_ON_HAL_ERR(
                HAL_UART_Transmit_DMA(handle->uart, chunk.data, tx_len), 
                err, ret);
            while(handle->tx_busy){;}

            handle->chunk_index++;

            if(handle->chunk_index == handle->nchunks){
                goto tx_complete;
            }

            can_send = (handle->task_end_time_us - Util_GetTimeUs()) > INFO_CHUNK_RX_TIME_US;
        }
        break;
    }
    
    default:
        break;
    }
    return ERR_OK;

tx_complete:
    handle->info_state = INFO_STATE_START;
    handle->packet_type_mask &= ~PACKET_MASK_INFO;
    handle->info_buffer_not_empty = false;
    return Telemetry_GiveInfoBuffer(handle, &handle->info_bufer);

err: 
    handle->info_state = INFO_STATE_START;
    handle->packet_type_mask &= ~PACKET_MASK_INFO;
    Telemetry_GiveInfoBuffer(handle, &handle->info_bufer);
    return ret;
}

static floatsat_err_t Telemetry_AddToRegistry(telemetry_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    int index = Util_UART2Index(handle->uart->Instance);
    if(index < 0){
        LOGE(LOG_TAG,"Invalid UART instance");
        return ERR_INVALID_ARG;
    }

    return ERR_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    int index = Util_UART2Index(huart->Instance);
    if(index < 0){
        LOGE(LOG_TAG,"HOW?!");
        return;
    }

    telemetry_handle_t *handle = telemetry_handle_registry[index];

    handle->tx_busy = false;
    
}

