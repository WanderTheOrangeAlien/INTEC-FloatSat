#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define CAMERA_TASK_STACK_SIZE      4096
#define CAMERA_TASK_PRIORITY        3

typedef struct camera_handle_t {
    uint8_t *frame_buffer;
    TaskHandle_t task_handle;

} camera_handle_t;


esp_err_t Camera_Init(camera_handle_t *handle);

void func(void);

#endif