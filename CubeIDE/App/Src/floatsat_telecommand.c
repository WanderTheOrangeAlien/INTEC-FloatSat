#include "floatsat_telecommand.h"

#include <string.h>

#include "stm32f4xx_hal_uart.h"
#include "stm32f407xx.h"


static floatsat_err_t CmdManager_AddToRegistry(cmd_manager_handle_t *handle);
static int UART2Index(USART_TypeDef *uart_base);


static const char *LOG_TAG = "CMD_MANAGER";

// Global handle registry. This is required because the UART interrupt callbacks
// don't have access to any user-defined context struct 
static cmd_manager_handle_t *cmd_manager_registry[FLOATSAT_UART_NUM] = {0};



floatsat_err_t CmdManager_Init(cmd_manager_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->uart || !handle->dma_buffer || !handle->msg_buffer || handle->task_handle == 0U){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret;

    handle->msg_buffer_empty = true;

    GOTO_ON_ERR(CmdManager_AddToRegistry(handle),err,ret);

    


    // Start reception

err:

    return ERR_OK;
}


void CmdManager_Task(void *args)
{
    cmd_manager_handle_t *handle = (cmd_manager_handle_t*)args;
    uint32_t notification_value;


    while(1){
        notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);



    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 
     * Since command length is variable, the UART reception is implemented as a 
     * very simple state machine. It first waits for a header of known length
     * which contains the length of the command and the CRC. Then, it 
     * waits for the stated amount of bytes, validates the CRC, and notifies the 
     * cmd manager task
    */

    // TODO: What happens if the length if wrong, and it ends up a little higher than
    // the actual message? The microcntorller will keep waiting for data that will not come
    // A far stretch, indeed, but should be account for this?

    int index = UART2Index(huart->Instance);
    if(index < 0){
        LOGE(LOG_TAG,"HOW?!");
        return;
    }

    cmd_manager_handle_t *handle = cmd_manager_registry[index];
    BaseType_t higher_task_woken = pdFALSE;


    if(handle == NULL){
        LOGE(LOG_TAG,"Rx INTERRUPT: Invalid UART instance");
        return;
    }

    switch (handle->state) 
    {
    case MSG_RX_STATE_WAIT_HEADER:{
        floatsat_msg_header_t header = {0};
        memcpy(&header,(uint8_t*)handle->dma_buffer,sizeof(floatsat_msg_header_t));
        
        if(header.sync != CMD_MSG_SYNC){
            LOGE(LOG_TAG,"Invalid Sync word");
            goto header_err;
        }

        if(header.len > CMD_DMA_BUFFER_LEN - sizeof(floatsat_msg_header_t)){
            LOGE(LOG_TAG,"Invalid msg length: %u",header.len);
            goto header_err;
        }

        handle->xfer_crc = header.crc;
        handle->xfer_len = header.len;

        handle->state = MSG_RX_STATE_WAIT_MSG;
        HAL_UART_Receive_DMA(huart, (uint8_t*)handle->dma_buffer + sizeof(floatsat_msg_header_t), header.len);

        break;
    }        
    
    case MSG_RX_STATE_WAIT_MSG:{
        // Compute CRC
        #warning "CMD Manager CRC not implemented!"


        if(handle->msg_buffer_empty){
            memcpy((uint8_t*)handle->msg_buffer, (uint8_t*)handle->dma_buffer, handle->xfer_len + sizeof(floatsat_msg_header_t));
            handle->msg_buffer_empty = false;

            vTaskNotifyGiveFromISR(handle->task_handle, &higher_task_woken);
        }else{
            // A new msg was received but the previous one wasn't serviced yet

            // TODO: Set the MSG_DROPPED bit in the system status struct 
        }

        handle->state = MSG_RX_STATE_WAIT_HEADER;
        memset((uint8_t*)handle->dma_buffer, 0, handle->xfer_len); // TODO: This might  not be necesary
        HAL_UART_Receive_DMA(huart,(uint8_t*)handle->dma_buffer,sizeof(floatsat_msg_header_t));


        if(higher_task_woken == pdTRUE){
            portYIELD_FROM_ISR(higher_task_woken); // TODO: Is this redundant? Could we skip the if?
        }
        break;
    
    }

    default:
        LOGE(LOG_TAG,"Invalid state: %u",handle->state);
        break;
    }

    return;

header_err:
    handle->state = MSG_RX_STATE_WAIT_HEADER;
    memset((uint8_t*)handle->dma_buffer, 0, sizeof(floatsat_msg_header_t));
    HAL_UART_Receive_DMA(huart,(uint8_t*)handle->dma_buffer,sizeof(floatsat_msg_header_t));

}


static int UART2Index(USART_TypeDef *uart_base)
{
    if(!uart_base){
        return -1;
    }

    static const USART_TypeDef *const UART_MAP[] = {
        USART1, USART2, USART3, UART4, UART5
    };

    for (size_t i = 0; i < sizeof(UART_MAP) / sizeof(USART_TypeDef*); i++){
        if(uart_base == UART_MAP[i]){
            return i;
        }
    }
    
    return -1;
    
}

static floatsat_err_t CmdManager_AddToRegistry(cmd_manager_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    int index = UART2Index(handle->uart->Instance);
    if(index < 0){
        LOGE(LOG_TAG,"Invalid UART instance");
        return ERR_INVALID_ARG;
    }

    if(cmd_manager_registry[index] != NULL){
        LOGE(LOG_TAG,"There was already a handle at the CMD Manager registry at index %d",index);
        return ERR_INVALID_STATE;
    }

    cmd_manager_registry[index] = handle;

    return ERR_OK;
}