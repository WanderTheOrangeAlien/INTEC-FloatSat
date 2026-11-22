#ifndef DEBUG_BUFFER_H
#define DEBUG_BUFFER_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "esp_err.h"

#define DBG_BUFFER_HEX_MARKER       0x03

typedef struct debug_buffer_t {
    size_t  buffer_size;
    uint8_t *buff;
    size_t  idx;
    bool    buffer_full;
    bool    send_on_full;
    bool    active;
}debug_buffer_t;

esp_err_t DbgBuffer_Init(debug_buffer_t *handle);
esp_err_t DbgBuffer_Printf(debug_buffer_t *handle, const char *format, ...);
esp_err_t DbgBuffer_Send(debug_buffer_t *handle);
esp_err_t DbgBuffer_PrintHex(debug_buffer_t *handle, uint8_t *buffer, uint16_t len);


#endif