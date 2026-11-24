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
#include "data_bus_priv.h"

static void update_init_flag(floatsat_err_t err, uint32_t *mask, uint32_t flags);

static const char *LOG_TAG = "INIT";

/* ============================= Module handles ============================= */

/* -------------- Time -------------- */
static rtc_handle_t rtc_handle =  {0};

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


/* -------------- Motor -------------- */
static motor_handle_t motor_handle = {0};


/* -------------- Light sensors and Multiplexer -------------- */
static VEML7700_handle_t veml7700_arr[FLOATSAT_LIGHT_SENSOR_AMOUNT] = {

    {
        .i2c            = {0},
        .gain           = VEML7700_ALS_GAIN_X1_8,
        .int_time       = VEML7700_ALS_IT_100MS,
        .persistence    = VEML7700_ALS_PERS_1
    },
    {
        .i2c            = {0},
        .gain           = VEML7700_ALS_GAIN_X1_8,
        .int_time       = VEML7700_ALS_IT_100MS,
        .persistence    = VEML7700_ALS_PERS_1
    },
    {
        .i2c            = {0},
        .gain           = VEML7700_ALS_GAIN_X1_8,
        .int_time       = VEML7700_ALS_IT_100MS,
        .persistence    = VEML7700_ALS_PERS_1
    },
};

static TCA9548_handle_t multiplexer = {
    .dev_address = FLOATSAT_MULTIPLEXER_ADDR,
    .current_channel = 0U
};

static light_sensor_handle_t light_sensor = {
    .multiplexer    = &multiplexer,
    .sensors        = {&veml7700_arr[0], &veml7700_arr[1], &veml7700_arr[2]}
};


/* -------------- Madgwick filter -------------- */
static madgwick_filter_t madgwick_filter = {0};

#ifdef FUSION_H
static FusionAhrs fusion_ahrs = {0};
static FusionBias fusion_bias = {0};

static const FusionAhrsSettings fusion_ahrs_settings = {
    .sampleRate = 100.0f,
    .convention = FusionConventionNwu,
    .gain = 5.0f,
    .gyroscopeRange = 0.0f,
    .accelerationRejection = 00.0f,
    .magneticRejection = 0.0f, // Leave this to 0.0f !
    .rejectionTimeout = 5.0f,
};

static const FusionBiasSettings fusion_bias_settings = {
    .stationaryPeriod = 3.0f,
    .stationaryThreshold = 0.5f,
    .sampleRate = (1000 /CONTROL_LOOP_PERIOD_MS)
};

#endif

/* -------------- Core -------------- */
static photo_info_t photo_arr[CORE_MAX_PHOTO_INSTRUCTIONS] = {0}; 
static control_config_t g_control_config = {0};
static data_bus_reader_t core_data_bus_reader = {0};

static floatsat_core_t core_handle = {
    .photo_arr          = photo_arr,
    .telementry_handle  = &telemetry_handle,
    .g_control_config   = &g_control_config,

    .motor_handle       = &motor_handle,
    .imu_handle         = &IMU_handle,
    .data_bus_reader    = &core_data_bus_reader,
    .rtc                = &rtc_handle,
    .light_sensor       = &light_sensor,

    .debug_id           = DEBUG_PACKET_ID_IMU,
};


/* -------------- Control -------------- */
static control_vars_t control_vars = {0};

static control_handle_t control_handle = {
    .imu                = &IMU_handle,
    .madgwick           = &madgwick_filter,
    .telemetry_handle   = &telemetry_handle,
    .motor              = &motor_handle,
    .g_control_config   = &g_control_config,
    .p_core_task        = &core_handle.task_handle,
    .control_vars       = &control_vars,
#ifdef FUSION_H
    .fusion_ahrs            = &fusion_ahrs,
    .fusion_ahrs_settings   = &fusion_ahrs_settings,
    .fusion_bias            = &fusion_bias,
    .fusion_bias_settings   = &fusion_bias_settings,
#endif
};


/* -------------- All handles struct -------------- */
floatsat_handles_t floatsat_handles = {
    .cmd_manager_handle     =   &cmd_manager,
    .control_handle         =   &control_handle,
    .telemetry_handle       =   &telemetry_handle,
    .madgwick_filter        =   &madgwick_filter,
	.core_handle			=  	&core_handle,
};

// Burn, baby, burn
floatsat_err_t FloatSat_Init(const floatsat_periph_t *periph, 
                             const mx_user_constants_t *constants, 
                             floatsat_callbacks_t *out_callbacks)
{
    if(!periph){
        return ERR_INVALID_ARG;
    }
    
    if(periph->imu_i2c == periph->light_sensor_i2c.periph){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret = ERR_OK;
    
    /*To count which subsystems where initialized properly to determine a degraded state 
      in which to run*/
    uint32_t init_mask = 0U; 

    // FreeRTOS objects MUST be initialized AFTER all peripherals.
    // The main reason is that creating an object (Queue, smphr etc.) sets the BASEPRI register to 0x50
    // Which makes all interrupts with a priority value higher then 5 not serviceable
    // The HAL Tick interrupt has a priority higher than 5, so the HAL Tick freezes if this happens
    uint32_t base_pri = __get_BASEPRI();

    if(base_pri != 0U){
        return ERR_INVALID_STATE;
    }
    
    // Reset the ESP32 to avoid bad boot due to slow power up ramp
    HAL_GPIO_WritePin(periph->esp_en_port, periph->esp_en_pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(periph->esp_en_port, periph->esp_en_pin, GPIO_PIN_SET);


    // Assign the peripheral and user config to each corresponding handle and then call each init function


    /* ------------------------------ IMU ------------------------------ */
    IMU_handle.i2c_handle           = periph->imu_i2c;
    IMU_handle.i2c_periph_init_fn   = periph->i2c_init_fn;
    IMU_handle.i2c_sda_pin          = periph->i2c_sda_pins[Util_I2C2Index(IMU_handle.i2c_handle->Instance)];
    IMU_handle.i2c_scl_pin          = periph->i2c_scl_pins[Util_I2C2Index(IMU_handle.i2c_handle->Instance)];
    IMU_handle.i2c_gpio_peirh       = periph->i2c_gpio;

    ret = IMU_Init(&IMU_handle);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializaing IMU. Error code: 0x%04x",ret);
    
    ret = IMU_CheckParams(&IMU_handle); // Test for proper initialization
    LOG_ON_ERR(ret, LOG_TAG, "Error verifying IMU registers. Error code: 0x%04x",ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_IMU);

    
    /* ------------------------------ Madwick ------------------------------ */
    // LOG_ON_ERR(Madgwick_Init(&madgwick_filter),err ,ret,
    //     LOG_TAG, "Errorr initializing Madgwick filter. Error code: 0x%04x",ret);

    /* ----------------------------- Motor ----------------------------- */
    motor_handle.pwm_timer                  = periph->motor_pwm_timer;
    motor_handle.encoder_timer              = periph->encoder_timer;
    motor_handle.encoder_sampling_timer     = periph->encoder_sampling_timer;
    motor_handle.encoder_sampling_period_ms = constants->encoder_sampling_period_ms;
    ret = Motor_Init(&motor_handle);
    LOG_ON_ERR(ret, LOG_TAG, "Errorr initializing motor controller. Error code: 0x%04x",ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_MOTOR);

    /* ----------------------------- Control ----------------------------- */
    ret = Control_Init(&control_handle);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing control. Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_CONTROL);

    /* ----------------------------- Telemetry ----------------------------- */
    telemetry_handle.uart = periph->main_uart;
    ret = Telemetry_Init(&telemetry_handle);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing telemetry. Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_TELEMETRY);

    /* ----------------------------- Telecommand ----------------------------- */
    cmd_manager.uart = periph->main_uart;
    ret = CmdManager_Init(&cmd_manager);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing telecomand manager. Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_TELECOMMAND);

    /* ----------------------------- Light Sensor----------------------------- */
    for (size_t i = 0; i < FLOATSAT_LIGHT_SENSOR_AMOUNT; i++)
    {
        veml7700_arr[i].i2c = periph->light_sensor_i2c;
    }
    multiplexer.i2c = periph->light_sensor_i2c;

    base_pri = __get_BASEPRI();

    ret = LightSensor_Init(&light_sensor);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing light sensors. Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_TELECOMMAND);

    /* ----------------------------- Core ----------------------------- */
    ret = Core_Init(&core_handle);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing Supervisor (aka Core). Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_CORE);

    /* ------------------------------ RTC ------------------------------ */
    rtc_handle.rtc = periph->rtc;
    ret = RTC_Init(&rtc_handle);
    LOG_ON_ERR(ret, LOG_TAG,"Error initializing RTC");
    update_init_flag(ret, &init_mask, INIT_FLAG_RTC);


    /* ----------------------------- Utility ----------------------------- */
    ret = Util_Init(periph->microsec_timer);
    LOG_ON_ERR(ret, LOG_TAG, "Error initializing utilities. Error code: 0x%04x", ret);
    update_init_flag(ret, &init_mask, INIT_FLAG_UTILITY);

    xTraceEnable(TRC_START);

    /* ============== Create all required FreeRTOS primitives =============== */
    core_handle.cmd_queue = xQueueCreate(CORE_CMD_QUEUE_LEN,sizeof(floatsat_cmd_t));
    cmd_manager.core_cmd_queue = core_handle.cmd_queue;

    telemetry_handle.queue = xQueueCreate(TELEMETRY_QUEUE_SIZE, sizeof(telemetry_packet_t*));
    telemetry_handle.info_semphr = xSemaphoreCreateMutex();


    // Pass the callbacks defined in main.c to their corresponding module
    /* ----------------------------- Motor ----------------------------- */
    out_callbacks->encoder_sample_cb = Motor_Cb_EncoderSampler;
    
    if(init_mask == INIT_MASK_ALL_PASSED){
        return ERR_OK;
    }else{
        return ERR_INIT_FAILURE;
    }

}

static void update_init_flag(floatsat_err_t err, uint32_t *mask, uint32_t flags)
{
    if(err == ERR_OK){
        *mask |= flags;
    }else{
        *mask &= (uint32_t)~flags;
    }
}

const floatsat_handles_t* FloatSat_GetHandles(void)
{
    return &floatsat_handles;
}
