#ifndef TCA9548_STM32_H
#define TCA9548_STM32_H

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "floatsat_types.h"
#include "floatsat_err_log.h"

#include "floatsat_i2c.h"

#define TCA9548_TIMEOUT_MS      100U
#define TCA9548_INIT_ATTEMPTS   5U


typedef struct TCA9548_handle_t {
    i2c_info_t          i2c;
    uint8_t             dev_address;
    uint8_t             current_channel;
}TCA9548_handle_t;

floatsat_err_t TCA9548_Init(TCA9548_handle_t *handle);
floatsat_err_t TCA9548_SelectChannel(TCA9548_handle_t *handle, uint8_t channel);

#endif