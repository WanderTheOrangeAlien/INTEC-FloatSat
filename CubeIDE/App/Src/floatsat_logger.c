#include "floatsat_telemetry.h"

#include "task.h"
#include "floatsat_logger.h"

#ifdef TRACE
static TraceStringHandle_t xStrLogger;
#endif

__attribute__((noreturn))
void Task_LogFn(void *params)
{
#ifdef TRACE
    xTraceStringRegister("Logger",&xStrLogger);
#endif

    // Low priority task that sends any pending messages through telemetry info packets
    uint8_t *info_buffer = NULL;
    floatsat_err_t ret = ERR_OK;

    Log_SetLoggerTaskHandle(xTaskGetCurrentTaskHandle());
    telemetry_handle_t *telemetry_handle = (telemetry_handle_t*)params;
    size_t idx = 0U; // Index of the info buffer at which the new message will start. Corresponds to the position of the END OF TEXT byte 
    size_t available_bytes;
    size_t msg_len = 0U;
    uint32_t n_pending_msg = 0U;

    while(1){
        // pdFALSE makes it so the ulTaskNotifyTake function decrements the notification value
        // before exit. That way, the notification value is equivalent to the number of pending messages
    	n_pending_msg = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);


        // If buffer has not been acquired, try to take it
        if(!info_buffer){
            ret = Telemetry_TakeInfoBuffer(telemetry_handle, &info_buffer);
        }else{
            ret = ERR_OK;
        }

        // Try to acquire the buffer
        if(ret != ERR_OK){
            // Send notification to itself to increase the pending msg count that was decremented in ulTaskNotifyTake
            xTaskNotify(xTaskGetCurrentTaskHandle(),1U,eIncrement);
            vTaskDelay(INFO_BUFFER_WAIT_DELAY_TICKS);
            continue;
        }

        uint8_t *p = (uint8_t*)memchr((char*)info_buffer, ASCII_ETX, TELEMETRY_INFO_BUFFER_SIZE);
        if(!p){
            ret = ERR_BUFFER_OVERFLOW;
        }else{
            idx = (size_t)(p - info_buffer);
            available_bytes = TELEMETRY_INFO_BUFFER_SIZE - idx;
            ret = Log_PopNextMsg(&info_buffer[idx], &msg_len, available_bytes);
        }

        

        if(ret == ERR_BUFFER_OVERFLOW){
            // The buffer is full, wait for the telemetry task to consume it. Release it so telemetry can use it
#ifdef TRACE
            xTracePrint(xStrLogger, "Buffer full");
#endif
            xTaskNotify(xTaskGetCurrentTaskHandle(), 1U, eIncrement);
            Telemetry_GiveInfoBuffer(telemetry_handle, &info_buffer);
            vTaskDelay(INFO_BUFFER_WAIT_DELAY_TICKS);
            continue;
            // Report the buffer full error
        }

        if(n_pending_msg == 0){
            // No more messages
            ulTaskNotifyValueClear(NULL, 0xFFFFFFFF); // TODO: Is this necesary?
            idx = 0U;
            msg_len = 0U;

            if(ret == 0){
                // This should not happen. This means that there are more messages in the buffer than the
                // amount of pending messages provided by the notification value
                assert(false);
            }

        }

        ret = Telemetry_GiveInfoBuffer(telemetry_handle, &info_buffer);

    }

}
