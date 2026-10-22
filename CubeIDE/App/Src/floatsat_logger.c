#include "floatsat_telemetry.h"

#include "task.h"
#include "floatsat_logger.h"

__attribute__((noreturn))
void Task_LogFn(void *params)
{
    // Low priority task that sends any pending messages through telemetry info packets
    uint8_t *info_buffer = NULL;
    floatsat_err_t ret = ERR_OK;

    Log_SetLoggerTaskHandle(xTaskGetCurrentTaskHandle());
    telemetry_handle_t *telemetry_handle = (telemetry_handle_t*)params;
    size_t idx = 0U; // Info buffer index
    size_t msg_len = 0U;
    uint32_t n_pending_msg = 0U;

    while(1){
        // pdFALSE makes it so the ulTaskNotifyTake function decrements the notification value
        // before exit. That way, the notification value is equivalent to the number of pending messages
    	n_pending_msg = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        // Try to acquire the buffer
        ret = Telemetry_TakeInfoBuffer(telemetry_handle, &info_buffer);
        if(ret != ERR_OK){
            continue;
        }
        idx = 0U;
        msg_len = 0U;

        // TODO: This could halt. Consider a hard upper bound
        // Pop all pending messages into the info buffer
        while(ret == 0){
            idx += msg_len;
            ret = Log_PopNextMsg(&info_buffer[idx], &msg_len);
            n_pending_msg--;

            if(n_pending_msg == 0){
            	// Reset the notification value
            	ulTaskNotifyValueClear(NULL, 0xFFFFFFFF);

            	if(ret == 0){
            		// This should not happen. This means that there are more messages in the buffer than the
            		// amount of pending messages provided by the notification value
            		assert(false);
            	}

            }

        }

        ret = Telemetry_GiveInfoBuffer(telemetry_handle, &info_buffer);

    }

}
