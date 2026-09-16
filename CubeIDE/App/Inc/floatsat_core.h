#ifndef FLOATSAT_CORE_H
#define FLOATSAT_CORE_H

#include "FreeRTOS.h"
#include "queue.h"

#include "floatsat_types.h"
#include "floatsat_error.h"

#include "floatsat_telemetry.h"

#define CORE_CMD_QUEUE_LEN              4
#define CORE_MAX_PHOTO_INSTRUCTIONS     32

/* @action_flags */
#define CORE_ACTION_FLAG_RESET_PHOTOS       (1 << 0)
#define CORE_ACTION_FLAG_CHANGE_MISSION     (1 << 1)

typedef struct floatsat_core_t {
    uint8_t             mission_id; // See @mission_id

    uint8_t             control_type;                       /* @control_type in floatsat_types.h*/
    float               control_params[CONTROL_MAX_PARAMS];

    photo_info_t        *photo_info_arr;
    uint8_t             curr_photo_index;       // Current photo index
    uint8_t             last_photo_index;  

    uint8_t             action_flags;       /* These flags are set through commands and indicate that 
                                                a lazy action (one done after critical ones) is
                                                pending. For instance, changing the mission */

    QueueHandle_t       cmd_queue;          // Receives parsed messages here
    TaskHandle_t        task_handle;
    floatsat_cmd_t      current_cmd;
    const telemetry_handle_t  *telementry_handle;

    /* Shared resources */
    control_config_t    *g_control_config;  // Control system config

}floatsat_core_t;

floatsat_err_t Core_Init(floatsat_core_t *handle);




#endif