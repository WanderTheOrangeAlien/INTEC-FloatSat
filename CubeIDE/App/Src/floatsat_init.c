/**
  ******************************************************************************
  * @file    floatsat_init.c
  * @author  Miguel Urena
  * @brief   Initialization of floatsat
  * 
  * @verbatim The function `FloatSat_Init()` calls all the initialization 
  * functions of the different FloatSat modules. The handles and param structures 
  * for each one are created as static variables
  *
  ******************************************************************************
**/

#include "floatsat_init.h"
static const char *LOG_TAG = "INIT";

/* ============================= Module handles ============================= */

/* -------------- IMU -------------- */
static const LSM9DS1_params_t IMU_params = {
    .gyro_odr           =   GYRO_ODR_119HZ,     
    .acc_odr            =   ACC_ODR_119HZ,      
    .mag_odr            =   MAG_ODR_80HZ,

    .gyro_scale         =   GYRO_FS_500DPS,      
    .acc_scale          =   ACC_FS_2G,           
    .mag_scale          =   MAG_FS_4GAUSS,
    
    .gyro_axes          =   LSM9DS1_GYRO_XEN_G | LSM9DS1_GYRO_YEN_G | LSM9DS1_GYRO_ZEN_G,
    .acc_axes           =   LSM9DS1_ACC_XEN_XL | LSM9DS1_ACC_YEN_XL | LSM9DS1_ACC_ZEN_XL, 
    
    .mag_xy_mode        =   MAG_XY_MODE_ULTRA_HIGH,
    .mag_op_mode        =   MAG_OP_MODE_CONTINUOUS,
    
    .mag_en_temp_comp   =   LSM9DS1_MAG_TEMP_COMP_EN
};

static IMU_handle_t IMU_handle = {
    .params = &IMU_params,
};

/* -------------- Telecommand -------------- */
// TODO: These can be static variables insice the floatsat_telecommand.c file
uint8_t cmd_dma_buffer[CMD_DMA_BUFFER_LEN] = {0};
uint8_t cmd_msg_buffer[CMD_MSG_BUFFER_LEN] = {0};

static parser_ctx_t parser_ctx = {0};

static cmd_manager_handle_t cmd_manager = {
    .dma_buffer              = cmd_dma_buffer,
    .msg_buffer              = cmd_msg_buffer,
    .parser_ctx              = &parser_ctx
};


/* -------------- Telemetry -------------- */
static telemetry_handle_t telemetry_handle = {0};

/* -------------- Madgwick filter -------------- */
static madgwick_filter_t madgwick_filter = {0};

/* -------------- Core -------------- */
static photo_info_t photo_info_arr[CORE_MAX_PHOTO_INSTRUCTIONS] = {0}; 
static control_config_t g_control_config = {0};

static floatsat_core_t core_handle = {
    .photo_info_arr = photo_info_arr,
    .telementry_handle = &telemetry_handle,
    .g_control_config = &g_control_config,
};


/* -------------- Control -------------- */
static control_handle_t control_handle = {
    .imu                = &IMU_handle,
    .madgwick           = &madgwick_filter,
    .telemetry_handle   = &telemetry_handle,
    .g_control_config   = &g_control_config
};


/* -------------- All handles struct -------------- */
floatsat_handles_t floatsat_handles = {
    .cmd_manager_handle     =   &cmd_manager,
    .control_handle         =   &control_handle,
    .telemetry_handle       =   &telemetry_handle,
    .madgwick_filter        =   &madgwick_filter
};

floatsat_err_t FloatSat_Init(const floatsat_periph_t *periph)
{
    if(!periph){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    // Create all required FreeRTOS primitives
    core_handle.cmd_queue = xQueueCreate(CORE_CMD_QUEUE_LEN,sizeof(floatsat_cmd_t));
    


    // Assign the peripheral to each corresponding handle and then call each init function
    
    /* ------------------------------ IMU ------------------------------ */
    IMU_handle.i2c_handle = periph->imu_i2c;
    GOTO_ON_ERR_LOG(IMU_Init(&IMU_handle), err, ret,
        LOG_TAG, "Error initializaing IMU. Error code: 0x%04x",ret);
    
    IMU_CheckParams(&IMU_handle); // Test for proper initialization

    /* ------------------------------ Madwick ------------------------------ */
    GOTO_ON_ERR_LOG(Madgwick_Init(&madgwick_filter),err ,ret,
        LOG_TAG, "Errorr initializing Madgwick filter. Error code: 0x%04x",ret);

    /* ----------------------------- Control ----------------------------- */
    GOTO_ON_ERR_LOG(Control_Init(&control_handle), err, ret,
        LOG_TAG, "Error initializing control. Error code: 0x%04x", ret);

    /* ----------------------------- Telecommand ----------------------------- */
    cmd_manager.uart = periph->main_uart;
    cmd_manager.core_cmd_queue = core_handle.cmd_queue;
    GOTO_ON_ERR_LOG(CmdManager_Init(&cmd_manager), err, ret,
        LOG_TAG, "Error initializing telecomand manager. Error code: 0x%04x", ret);

    /* ----------------------------- Telemetry ----------------------------- */
    telemetry_handle.uart = periph->main_uart;
    GOTO_ON_ERR_LOG(Telemetry_Init(&telemetry_handle), err, ret, 
        LOG_TAG, "Error initializing telemetry. Error code: 0x%04x", ret);

    /* ----------------------------- Telemetry ----------------------------- */
    GOTO_ON_ERR_LOG(Core_Init(&core_handle), err, ret, 
        LOG_TAG, "Error initializing Supervisor (aka Core). Error code: 0x%04x", ret);



    return ERR_OK;

err:
    return ret;

}

const floatsat_handles_t* FloatSat_GetHandles(void)
{
    return &floatsat_handles;
}