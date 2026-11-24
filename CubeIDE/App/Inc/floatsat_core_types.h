#ifndef FLOATSAT_CORE_TYPES_H
#define FLOATSAT_CORE_TYPES_H

#include "FreeRTOS.h"
#include "queue.h"

#include "floatsat_types.h"
#include "floatsat_err_log.h"

#include "data_bus.h"
#include "floatsat_telemetry.h"
#include "floatsat_control.h"
#include "floatsat_motor_controller.h"
#include "floatsat_time.h"
#include "fs_commands.h"
#include "floatsat_light_sensor.h"

/* @action_flags */
#define CORE_ACTION_UPDATE_DEBUG            (1U << 0)

#define CORE_ACTION_UPDATE_PHOTO_ALARM      (1U << 1)
#define CORE_ACTION_SET_PHOTO_ALARM         (1U << 2)
#define CORE_ACTION_TAKE_PHOTO              (1U << 3)
#define CORE_ACTION_PHOTO_CLEANUP           (1U << 4)
#define CORE_ACTION_RESET_PHOTOS            (1U << 5)
#define CORE_ACTION_CHANGE_MISSION          (1U << 6)
#define CORE_ACTION_BRIDGE_ACTIVE           (1U << 7)
#define CORE_ACTION_PHOTO_FAIL              (1U << 8)


#define CORE_MAX_PHOTO_INSTRUCTIONS         32
#define CORE_PHOTO_PREPARATION_TIME_S       10U // TODO: Change this to an actual measured value
#define CORE_PHOTO_TIMEOUT_MS               (10U * 1000U)

#define MISSION_STATE_PHOTO_IDLE            0x00
#define MISSION_STATE_PHOTO_MOVING          0x01
#define MISSION_STATE_PHOTO_BUSY            0x02

typedef struct floatsat_core_t {
    
    photo_info_t        *photo_arr;
    photo_info_t        next_photo;     /* Shadow copy of the next photo instruction to execute, 
                                        so that the list can be modified without interrupting the execution */
    uint8_t             next_photo_idx;
    uint8_t             photo_arr_len;
    bool                next_photo_is_async;   // Set when the commmand CMD_ID_PHOTO_TAKE is received 
    uint32_t            photo_deadline_tick; /* FreeRTOS tick that indicates the deadline to take the photo. 
                                                If the ESP32 does not invoke the finish/fail callback, 
                                                the fail callback will be invoked automatically  */
    
    uint8_t             current_mission;         // See @mission_id
    uint8_t             next_mission;

    uint32_t            action_flags;       /* These flags are set through commands and indicate that 
                                                a lazy action (one done after critical ones) is
                                                pending. For instance, changing the mission */
    uint8_t             debug_id;
    uint8_t             mission_state;      /*  States internal to the current mission. 
                                                Right now only for the photo mission*/

    TaskHandle_t        task_handle;
    floatsat_cmd_t      current_cmd;
    telemetry_handle_t  *telementry_handle;
    rtc_handle_t        *rtc;

    /* Shared resources */
    QueueHandle_t       cmd_queue;          // Receives parsed messages here
    control_config_t    *g_control_config;  // Control system config
    

    /* Handles */
    motor_handle_t      *motor_handle;
    IMU_handle_t        *imu_handle;
    light_sensor_handle_t *light_sensor;

    data_bus_reader_t   *data_bus_reader;

}floatsat_core_t;


typedef struct sys_cb_entry_t {
    uint32_t flag;
    char name[CMD_SYSCTL_MAX_CB_NAME_LEN + 1U];
}sys_cb_entry_t;

#endif