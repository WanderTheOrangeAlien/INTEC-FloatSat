#include "floatsat_telemetry.h"



void Task_LogFn(void *params)
{
    // Low priority task that sends any pending messages through telemetry info packets
    uint8_t *info_buffer = NULL;
    floatsat_err_t ret = ERR_OK;

    const telemetry_handle_t *telemetry_handle = (const telemetry_handle_t*)params;
    const ringbuff_t *ringbuff = Log_GetRingbuff();
    size_t idx = 0U; // Info buffer index
    size_t msg_len = 0U;

    while(1){
        // Try to pop a message. If the returned value is not 0, then there were none
        idx = 0U;
        ret = Log_PopNextMsg(&info_buffer[idx], &msg_len);
        if(ret != 0){
            continue;
        }
        
        // Try to acquire the buffer
        ret = Telemetry_TakeInfoBuffer((telemetry_handle_t*)telemetry_handle, &info_buffer);
        if(ret != ERR_OK){
            continue;
        }

        while(ret == 0){
            idx += msg_len;
            ret = Log_PopNextMsg(&info_buffer[idx], &msg_len);
        }

        ret = Telemetry_GiveInfoBuffer((telemetry_handle_t*)telemetry_handle, &info_buffer);

    }

}