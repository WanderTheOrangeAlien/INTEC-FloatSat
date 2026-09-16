#ifndef INC_FLOATSAT_INIT_H
#define INC_FLOATSAT_INIT_H

#include "stm32f4xx_hal.h"

#include "floatsat_error.h"

#include "LSM9DS1_stm32.h"
#include "madgwick_filter.h"
#include "floatsat_telecommand.h"
#include "floatsat_telemetry.h"
#include "floatsat_control.h"
#include "floatsat_core.h"

// Structure to enclose all peripheral handles. This is required to cleanly
// pass all the handles from main.c to floatsat_init.c
typedef struct floatsat_periph_t {
    I2C_HandleTypeDef       *imu_i2c;
    UART_HandleTypeDef      *main_uart;

}floatsat_periph_t;

// Structure to group all task handles. This is required to cleanly pass all 
/// handles from main.c to floatsat_init.c
typedef struct floatsat_tasks_t {
    TaskHandle_t    cmd_manager_task;
    TaskHandle_t    telemetry_task;
}floatsat_tasks_t;

// Struct to group all initialized handles so they can be passed as parameters to
// the FreeRTOS tasks
typedef struct floatsat_handles_t {
    const cmd_manager_handle_t    *cmd_manager_handle;
    const telemetry_handle_t      *telemetry_handle;
    const control_handle_t        *control_handle;
    const madgwick_filter_t       *madgwick_filter;
    const floatsat_core_t         *core_handle;
}floatsat_handles_t;


floatsat_err_t FloatSat_Init(const floatsat_periph_t *peripherals);
const floatsat_handles_t*  FloatSat_GetHandles(void);


#endif
