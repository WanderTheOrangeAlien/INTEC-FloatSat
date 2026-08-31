#include "floatsat_telemetry.h"


static floatsat_err_t Telemetry_SendPacket(telemetry_handle_t *handle, 
                                           telemetry_packet_t *packet);


static const char *LOG_TAG = "TELEMETRY";

floatsat_err_t Telemetry_Init(telemetry_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->g_orientation){
        return ERR_INVALID_ARG;
    }

    handle->queue = xQueueCreate(TELEMETRY_QUEUE_SIZE, sizeof(telemetry_packet_t*));

    if(handle->queue == 0U){
        LOGE(LOG_TAG,"Queue could not be created");
        return ERR_INVALID_STATE;
    }



}




void telemetry_task(void *args)
{
    telemetry_handle_t *handle = (telemetry_handle_t*)args;
    telemetry_packet_t packet;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Access shared resource atomically
        portENTER_CRITICAL();
        packet.orientation = handle->g_orientation;
        portEXIT_CRITICAL(); 

        // TODO: Figure out when to send the extended packets (could be a counter)

        // TODO: Send the packet
    }

}

static floatsat_err_t Telemetry_SendPacket(telemetry_handle_t *handle, telemetry_packet_t *packet)
{
    if(!handle || !packet){
        return ERR_INVALID_ARG;
    }

    HAL_StatusTypeDef status = HAL_OK;
    uint8_t tx_len = 0U;


    if(packet->type == PACKET_TYPE_ORIENTATION){
        // Send only the first 2 fields
        tx_len = offsetof(telemetry_packet_t, temp);
    }else if(packet->type == PACKET_TYPE_EXTENDED){
        tx_len = sizeof(telemetry_packet_t);
    }else{
        LOGE(LOG_TAG,"Invalid packet type. Type: %d",packet->type);
        return ERR_INVALID_ARG;
    }

    status = HAL_UART_Transmit(handle->uart, (uint8_t*)packet, tx_len,  
                               TELEMETRY_UART_TIMEOUT_MS);

    if(status != HAL_OK){
        LOGE(LOG_TAG, "Error in transmission. Status: %d", status);
        return ERR_UART_TX_FAIL;
    }


    return ERR_OK;
}