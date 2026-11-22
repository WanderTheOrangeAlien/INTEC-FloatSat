#include <string.h>
#include "Arducam_OV2640.h"

static void Camera_Task(void *params);
static esp_err_t OV2640_WriteReg(ov2640_handle_t *handle, uint8_t reg, uint8_t val);
static esp_err_t OV2640_ReadReg(ov2640_handle_t *handle, uint8_t reg, uint8_t *val);
static esp_err_t OV2640_InitRegs(ov2640_handle_t *handle);
static esp_err_t OV2640_RunInitSeq(ov2640_handle_t *handle, const sensor_reg_t init_seq[]);


static esp_err_t Arducam_GetFifoSize(ov2640_handle_t *handle, uint32_t *size);
static esp_err_t Arducam_WriteReg(ov2640_handle_t *handle, uint8_t reg, uint8_t val);
static esp_err_t Arducam_ReadReg(ov2640_handle_t *handle, uint8_t reg, uint8_t *val);
static esp_err_t Arducam_SendCmd(ov2640_handle_t *handle, uint8_t cmd);



static const char LOG_TAG[] = "Arducam_OV2640";

esp_err_t OV2640_Init(ov2640_handle_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->spi_config.queue_size == 0U){
        ESP_LOGE(LOG_TAG, "SPI Queue size cannot be zero");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    spi_bus_config_t spi_bus_config = {
        .miso_io_num        = handle->spi_config.pin_miso,
        .mosi_io_num        = handle->spi_config.pin_mosi,
        .sclk_io_num        = handle->spi_config.pin_sclk,
        .quadwp_io_num      = -1,
        .quadhd_io_num      = -1,
        .max_transfer_sz    = handle->spi_config.max_transfer_size,
    };

    spi_device_interface_config_t devconfig = {
        .clock_speed_hz     =   (8U * 1000U * 1000U),
        .mode               =   0U, // TODO: Is this the right mode?
        .spics_io_num       =   handle->spi_config.pin_cs,
        .queue_size         =   handle->spi_config.queue_size,
    };


    bool spi_bus_init = false;
    ret = spi_bus_initialize(handle->spi_config.host, &spi_bus_config, SPI_DMA_CH_AUTO);
    ESP_GOTO_ON_ERROR(ret, cleanup, LOG_TAG, "Error initializing SPI bus");
    spi_bus_init = true;

    ret = spi_bus_add_device(handle->spi_config.host, &devconfig, &handle->spi_handle);
    ESP_GOTO_ON_ERROR(ret, cleanup, LOG_TAG, "Error adding SPI device ");

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source                     = I2C_CLK_SRC_DEFAULT,
        .i2c_port                       = handle->i2c_config.i2c_port,
        .scl_io_num                     = handle->i2c_config.pin_scl,
        .sda_io_num                     = handle->i2c_config.pin_sda,
        .glitch_ignore_cnt              = OV2640_I2C_GLITCH_IGNORE,
        .flags.enable_internal_pullup   = true,
    };
    
    ret = i2c_new_master_bus(&i2c_bus_config, &handle->i2c_bus_handle);
    ESP_GOTO_ON_ERROR(ret, cleanup, LOG_TAG, "Error initializing I2C bus");
    i2c_device_config_t i2c_devconfig = {
        .dev_addr_length    = I2C_ADDR_BIT_LEN_7,
        // Shift 1 to the right because the 0x60 address in the datasheet is the full byte and not the 7-bit address
        // Which means that the actual 7-bit address is 0x60 > 1
        .device_address     = OV2640_ADDRESS >> 1U, 
        .scl_speed_hz       = handle->i2c_config.clk_freq_hz,
    };

    ret = i2c_master_bus_add_device(handle->i2c_bus_handle, &i2c_devconfig, &handle->i2c_handle);
    ESP_GOTO_ON_ERROR(ret, cleanup, LOG_TAG, "Error adding I2C device");

    ret = OV2640_InitRegs(handle);
    ESP_GOTO_ON_ERROR(ret, cleanup, LOG_TAG, "Error in OV2640 initialization sequence");

    // xTaskCreate(Camera_Task,"Camera", OV2640_TASK_STACK_SIZE, handle, OV2640_TASK_PRIO, &handle->task_handle);

    return ESP_OK;

cleanup:

    if(handle->spi_handle){
        spi_bus_remove_device(handle->spi_handle);
        handle->spi_handle = NULL;
    }

    if(spi_bus_init){
        spi_bus_free(handle->spi_config.host);
    }
    

    if(handle->i2c_handle){
        i2c_master_bus_rm_device(handle->i2c_handle);
        handle->i2c_handle = NULL;
    }
    if(handle->i2c_bus_handle){
        i2c_del_master_bus(handle->i2c_bus_handle);
        handle->i2c_bus_handle = NULL;
    }



    return ret;
}

/// @brief Initiate an image capture. It will send the capture command to the camera an poll
/// until the write to its internal FIFO is complete. On success, the number of available bytes
/// to read is stored in `len`
/// @param handle 
/// @param len Amount of bytes available to read
/// @return 
esp_err_t OV2640_Capture(ov2640_handle_t *handle, size_t *len)
{
    if(!handle || !len){
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->state != ARDUCAM_STATE_READY){
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_OK;
    handle->state = ARDUCAM_STATE_LOCKED;
    
    // Clear FIFO write done flag
    ret = Arducam_WriteReg(handle, ARDUCAM_REG_FIFO_CTRL, ARDUCAM_FIFO_CTRL_CLEAR_WRT_DONE);
    ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "(Capture) Error writing reg");    
    
    // Start capture
    ret = Arducam_WriteReg(handle, ARDUCAM_REG_FIFO_CTRL, ARDUCAM_FIFO_CTRL_CAP_START); 
    ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "(Capture) Error writing reg");
    

    uint8_t reg_val = 0x00;
    TickType_t start = xTaskGetTickCount();
    // Poll until capture is complete

    do{
        ret = Arducam_ReadReg(handle, ARDUCAM_REG_VSYNC, &reg_val);
        ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "(Capture) Error initiating capture");
        
        if((reg_val & ARDUCAM_VSYNC_W_FIFO_DONE) != 0U){
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(ARDUCAM_FIFO_POLL_PERIOD_MS));
    
    }while( xTaskGetTickCount() < start + pdMS_TO_TICKS(ARDUCAM_FIFO_POLL_TIMEOUT_MS) );

    ret = Arducam_GetFifoSize(handle, &handle->fifo_size);
    ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "(Capture) Error getting fifo len");

    *len = handle->fifo_size;
    handle->state = ARDUCAM_STATE_READY;


    return ESP_OK;
err:
    handle->fifo_size = 0U;
    *len = 0U;
    handle->state = ARDUCAM_STATE_READY;
    return ret;
}

/// @brief Read a chunk of `len` bytes from the camera FIFO
/// @param handle 
/// @param buffer Buffer to store the chunk. Must be DMA-capable (allocated with `heap_caps_malloc(size, MALLOC_CAP_DMA)`)
/// @param len Number of bytes to read
/// @param remaining Number of bytes remaining in the FIFO
/// @return 
esp_err_t OV2640_ReadChunk(ov2640_handle_t *handle, uint8_t *buffer, size_t len, size_t *remaining)
{
    if(!handle || !buffer || !remaining || len == 0U){
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->state != ARDUCAM_STATE_READY){
        ESP_LOGE(LOG_TAG,"(ReadChunk) handle state was not READY");
        return ESP_ERR_INVALID_STATE;
    }

    if(handle->fifo_size == 0U){
        ESP_LOGE(LOG_TAG,"(ReadChunk) FIFO is empty");
        return ESP_ERR_INVALID_STATE;
    }

    if(len > handle->fifo_size){
        ESP_LOGE(LOG_TAG,"(ReadChunk) Transer length of %u was larger than the " 
            "remaining bytes in the FIFO (%u)",
            len, handle->fifo_size);
        return ESP_ERR_INVALID_SIZE;
    }
    esp_err_t ret = ESP_OK;
    handle->state = ARDUCAM_STATE_LOCKED;

    ret = spi_device_acquire_bus(handle->spi_handle, portMAX_DELAY);
    ESP_GOTO_ON_ERROR(ret, err_bus_not_acquired, LOG_TAG, "");
    
    // Send the FIFO burst read command
    spi_transaction_t t = {
        .length = 8U,
        .flags  = SPI_TRANS_CS_KEEP_ACTIVE | SPI_TRANS_USE_TXDATA,
        .tx_data = {ARDUCAM_REG_FIFO_BURST}
    };
    ret = spi_device_polling_transmit(handle->spi_handle, &t);
    ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "");

    // Read `len` bytes into the provided buffer
    spi_transaction_t t2 = {
        .length     = len * 8U,
        .rx_buffer  = buffer
    };
    ret = spi_device_transmit(handle->spi_handle, &t2);
    ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "");

    handle->fifo_size -= len;

    spi_device_release_bus(handle->spi_handle);
    handle->state = ARDUCAM_STATE_READY;
    *remaining = handle->fifo_size;
    return ESP_OK;

    
err:
    spi_device_release_bus(handle->spi_handle);

err_bus_not_acquired:
    // The chunk is no longer valid. Set fifo_size to 0 to prevent any more reads
    handle->fifo_size = 0U;
    *remaining = 0U;
    
    memset(buffer, 0, len);
    handle->state = ARDUCAM_STATE_READY;
    return ret;
}

esp_err_t OV2640_SetResolution(ov2640_handle_t *handle, ov2640_res_t res)
{
    if(!handle || res < 0 || res >= OV2640_RES_MAX){
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = ESP_OK;
    const sensor_reg_t *seq = OV2640_RESOLUTIONS[res];

    ret = OV2640_RunInitSeq(handle, seq);
    ESP_RETURN_ON_ERROR(ret, LOG_TAG, "Error changing the resolution");
    handle->resolution = res;

    return ESP_OK;
}

/*  static void Camera_Task(void *params)
{
    ov2640_handle_t *handle = (ov2640_handle_t*)params;
    esp_err_t ret = ESP_OK;

    while(1){

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(handle->xfer_len == 0U){
            ESP_LOGE(LOG_TAG, "(Camera Task) Transfer length was 0");
            continue;
        }
        // Send at most buffer_size bytes
        size_t bytes_to_read = (handle->xfer_len > handle->buffer_size) ? handle->buffer_size : handle->xfer_len;
        spi_transaction_t t = {
            .length = bytes_to_read * 8U,
            .rx_buffer = handle->rx_buffer,
        };
        memset(handle->rx_buffer, 0, handle->buffer_size);
        
        ret = spi_device_acquire_bus(handle->spi_handle, portMAX_DELAY);
        ESP_GOTO_ON_ERROR(ret, err, LOG_TAG, "(Camera Task) Error acquiring SPI bus");

        ret = spi_device_transmit(handle->spi_handle, &t);
        ESP_GOTO_ON_ERROR(ret, err, LOG_TAG,"(Camera Task) Error reading FIFO");

        handle->xfer_len -= bytes_to_read;

        if(handle->callbacks.on_fifo_read){
            handle->callbacks.on_fifo_read(handle->rx_buffer, bytes_to_read, handle->task_handle);
        }

        
        if(handle->xfer_len == 0U){
            spi_device_release_bus(handle->spi_handle);
            if(handle->callbacks.on_capture_cmplt){
                handle->callbacks.on_capture_cmplt();
            }
        }

        continue;
err:
        memset(handle->rx_buffer, 0, handle->buffer_size);
        handle->xfer_len = 0U;


    }
} */


static esp_err_t OV2640_InitRegs(ov2640_handle_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(OV2640_WriteReg(handle, 0xFF, 0x01), LOG_TAG, "Error in OV2640 init"); // Select register bank 1 (Camera)
    ESP_RETURN_ON_ERROR(OV2640_WriteReg(handle, 0x12, 0x80), LOG_TAG, "Error in OV2640 init"); // Factory Reset 
    
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(OV2640_RunInitSeq(handle, OV2640_JPEG_INIT_SEQ), LOG_TAG, "Error in OV2640 init");
    ESP_RETURN_ON_ERROR(OV2640_RunInitSeq(handle, OV2640_YUV422), LOG_TAG, "Error in OV2640 init");
    ESP_RETURN_ON_ERROR(OV2640_RunInitSeq(handle, OV2640_JPEG), LOG_TAG, "Error in OV2640 init");
    
    ESP_RETURN_ON_ERROR(OV2640_WriteReg(handle, 0xFF, 0x01), LOG_TAG, "Error in OV2640 init");
    ESP_RETURN_ON_ERROR(OV2640_WriteReg(handle, 0x15, 0x00), LOG_TAG, "Error in OV2640 init"); // COM10 to default value
    
    ESP_RETURN_ON_ERROR(OV2640_RunInitSeq(handle, OV2640_320x240_JPEG), LOG_TAG, "Error in OV2640 init");
    handle->resolution = OV2640_RES_320x240;

    return ESP_OK;
}

/// @brief Run an init sequence with the format of the original Arducam Arduino library.
/// The last member of the array MUST be {0xFF, 0xFF}
/// @param handle 
/// @param init_seq 
/// @return 
static esp_err_t OV2640_RunInitSeq(ov2640_handle_t *handle, const sensor_reg_t init_seq[])
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    size_t idx = 0U;
    while( !(init_seq[idx].addr == 0xFF && init_seq[idx].val == 0xFF) ){
        ret = OV2640_WriteReg(handle, init_seq[idx].addr, init_seq[idx].val);
        
        if(ret != ESP_OK){
            return ret;
        }
        ++idx;
    }
    return ESP_OK;
}

static esp_err_t Arducam_GetFifoSize(ov2640_handle_t *handle, uint32_t *size)
{
    if(!handle || !size){
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    uint8_t regs[3] = {ARDUCAM_REG_FIFO_SIZE_1, ARDUCAM_REG_FIFO_SIZE_2, ARDUCAM_REG_FIFO_SIZE_3};
    uint8_t data[3] = {0};

    for (size_t i = 0; i < 3; i++)
    {
        ret = Arducam_ReadReg(handle, regs[i], &data[i]);
        if(ret != ESP_OK){
            *size = 0U;
            return ret;
        }
        
    }
    // The last register only provides 7 bits. Mask to make sure we have the proper value
    data[2] &= 0x7F;

    *size = ((uint32_t)data[2] << 16U) | ((uint32_t)data[1] << 8U) | ((uint32_t)data[0]);

    return ESP_OK;
}

static esp_err_t OV2640_WriteReg(ov2640_handle_t *handle, uint8_t reg, uint8_t val)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[2] = {reg, val};
    esp_err_t ret = i2c_master_transmit(handle->i2c_handle, buffer, 2U, handle->i2c_config.timeout_ms);

    // vTaskDelay(1);
    return ret;
}

static esp_err_t OV2640_ReadReg(ov2640_handle_t *handle, uint8_t reg, uint8_t *val)
{
    if(!handle || !val){
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    ret = i2c_master_transmit(handle->i2c_handle, &reg, 1U, handle->i2c_config.timeout_ms);
    if(ret != ESP_OK) return ret;

    ret = i2c_master_receive(handle->i2c_handle, val, 1U, handle->i2c_config.timeout_ms);
    // vTaskDelay(1);

    return ret;
}

static esp_err_t Arducam_WriteReg(ov2640_handle_t *handle, uint8_t reg, uint8_t val)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data = {reg | (uint8_t)0x80, val},
        .length = 2U * 8U
    };

    return spi_device_polling_transmit(handle->spi_handle, &t);
}

static esp_err_t Arducam_ReadReg(ov2640_handle_t *handle, uint8_t reg, uint8_t *val)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
        .tx_data = {reg, 0x00},
        .length = 2U * 8U
    };

    esp_err_t ret = spi_device_polling_transmit(handle->spi_handle, &t);
    if(ret != ESP_OK){
        return ret;
    }

    *val = t.rx_data[1];
    return ESP_OK;

}

static esp_err_t Arducam_SendCmd(ov2640_handle_t *handle, uint8_t cmd)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data = {cmd},
        .length = 8U
    };

    return spi_device_polling_transmit(handle->spi_handle, &t);
}


