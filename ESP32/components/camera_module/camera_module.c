#include <stdio.h>
#include "camera_module.h"

#include "esp_clk_tree.h"

static void Camera_Task(void *params);

esp_err_t Camera_Init(camera_handle_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    xTaskCreate(Camera_Task, "Camera", CAMERA_TASK_STACK_SIZE, (void*)handle, 
        CAMERA_TASK_PRIORITY, &handle->task_handle);
    return ESP_OK;
}


static void Camera_Task(void *params)
{
    uint32_t freq = 0U;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &freq);
    size_t loop_count = (size_t)( (freq / 5.0f) * 10e-3 );
    uint64_t n = 0;
    
    TickType_t last_wake = 0U;

    printf( "==================================\n"
            "freq = %lu MHz\n"
            "loop_count = %zu\n"
            "1 tick = %lu ms\n"
            "==================================", 
            freq / 1000000U, loop_count, portTICK_PERIOD_MS);

    

    while(1){

        // printf("Camera woke up at %lu ms\n",xTaskGetTickCount() / portTICK_PERIOD_MS);

        vTaskDelayUntil(&last_wake,pdMS_TO_TICKS(10));
    }
}