#ifndef ARDUCAM_OV2640_H
#define ARDUCAM_OV2640_H

#include "driver/spi_master.h"
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "OV2640_init_seq.h"

// Full 8-bit address of the device. For the actual 7-bit address (used by the ESP IDF), you MUST right shift it once
#define OV2640_ADDRESS          0x60

#define ARDUCAM_FIFO_POLL_PERIOD_MS     10U
#define ARDUCAM_FIFO_POLL_TIMEOUT_MS    200U

#define ARDUCAM_STATE_READY            0x00
#define ARDUCAM_STATE_LOCKED           0x01



/* ========================== OV2640 Register definitions ========================== */
// These register are accessed via the I2C interface


#define OV2640_REG_RA_DLMT      0xFF    // Register bank selection

#define OV2640_REG_GAIN         0x00
#define OV2640_REG_COM1         0x03    // Common control 1
#define OV2640_REG_REG04        0x04    // Register 4. Flip, VREF, HREF and AEC[1:0]
#define OV2640_REG_REG08        0x08    // Frame exposure One-pin control pre-change row number
#define OV2640_REG_COM2         0x09    // Common control 2
#define OV2640_REG_PIDH         0x0A    // Product ID MSB
#define OV2640_REG_PIDL         0x0B    // Product ID LSB
#define OV2640_REG_COM3         0x0C    // Common control 3
#define OV2640_REG_COM4         0x0D    // Common control 4

#define OV2640_REG_AEC          0x10    // Automatic exposure control AEC[9:2]
#define OV2640_REG_COM7         0x12    // Common control 7


#define OV2640_TASK_STACK_SIZE          1024U
#define OV2640_TASK_PRIO                2U
#define OV2640_I2C_GLITCH_IGNORE        5U


/* ======================= ArduCam Register definitions ======================= */
// These register are accessed via the SPI interface
// Describe here (page 19) https://cdn.arducam.com/downloads/shields/ArduCAM_Camera_Shield_Software_Application_Note.pdf

#define ARDUCAM_REG_TEST                0x00U
#define ARDUCAM_REG_CAPTURE_CTRL        0x01U
#define ARDUCAM_REG_BUS_MODE            0x02U
#define ARDUCAM_REG_TIMING              0x03U
#define ARDUCAM_REG_FIFO_CTRL           0x04U
#define ARDUCAM_REG_GPIO_DIR            0x05U
#define ARDUCAM_REG_GPIO_WRITE          0x06U

#define ARDUCAM_REG_FIFO_BURST          0x3C
#define ARDUCAM_REG_FIFO_SINGLE_READ    0x3D
#define ARDUCAM_REG_LCD_CTRL_0          0x3E
#define ARDUCAM_REG_LCD_CTRL_1          0x3F
#define ARDUCAM_REG_VERSION             0x40
#define ARDUCAM_REG_VSYNC               0x41
#define ARDUCAM_REG_FIFO_SIZE_1         0x42
#define ARDUCAM_REG_FIFO_SIZE_2         0x43
#define ARDUCAM_REG_FIFO_SIZE_3         0x44
#define ARDUCAM_REG_GPIO_READ           0x45

/* ======================= ArduCam Register fields ======================= */

// FIFO_CTRL
#define ARDUCAM_FIFO_CTRL_CLEAR_WRT_DONE    (1U << 0)
#define ARDUCAM_FIFO_CTRL_CAP_START         (1U << 1)
#define ARDUCAM_FIFO_CTRL_RESET_W_PTR       (1U << 4)
#define ARDUCAM_FIFO_CTRL_RESET_R_PTR       (1U << 5) 


// FIFO_CTRL
// VSYCN
#define ARDUCAM_VSYNC_W_FIFO_DONE           (1U << 3)


typedef struct spi_config_t {
    uint8_t             *frame_buffer;
    int                 pin_miso;
    int                 pin_mosi;
    int                 pin_sclk;
    int                 pin_cs;
    size_t              max_transfer_size;
    size_t              queue_size;
    spi_host_device_t   host;
}spi_config_t;

typedef struct i2c_config_t {
    i2c_port_num_t  i2c_port;
    uint8_t         pin_sda;
    uint8_t         pin_scl;
    uint32_t        clk_freq_hz;
    uint32_t        timeout_ms;
}i2c_config_t;

typedef enum ov2640_res_t {
    OV2640_RES_160x120      = 0,
    OV2640_RES_176x144      = 1,
    OV2640_RES_320x240      = 2,
    OV2640_RES_352x288      = 3,
    OV2640_RES_640x480      = 4,
    OV2640_RES_800x600      = 5,
    OV2640_RES_1024x768     = 6,
    OV2640_RES_1280x1024    = 7,
    OV2640_RES_1600x1200    = 8,
    OV2640_RES_MAX          = 9
}ov2640_res_t;

typedef struct ov2640_handle_t {
    
    spi_config_t        spi_config;
    i2c_config_t        i2c_config;
    
    spi_device_handle_t     spi_handle;
    i2c_master_dev_handle_t i2c_handle;
    i2c_master_bus_handle_t i2c_bus_handle;
    
    uint32_t                fifo_size;
    
    TaskHandle_t            task_handle;
    uint32_t                xfer_len; // Current capture transfer length

    uint8_t                 state;
    ov2640_res_t            resolution;

}ov2640_handle_t;



esp_err_t OV2640_Init(ov2640_handle_t *handle);

esp_err_t OV2640_Capture(ov2640_handle_t *handle, size_t *len);
esp_err_t OV2640_ReadChunk(ov2640_handle_t *handle, uint8_t *buffer, size_t len, size_t *remaining);
esp_err_t OV2640_SetResolution(ov2640_handle_t *handle, ov2640_res_t res);

#endif