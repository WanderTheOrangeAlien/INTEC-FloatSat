#include <stdio.h>
#include <string.h>
#include "debug_buffer.h"

static const char DBG_START_MSG[] = "DBG_BUFF_START";

esp_err_t DbgBuffer_Init(debug_buffer_t *handle)
{
    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    handle->buff = calloc(1U, handle->buffer_size);
    handle->buffer_full = false;
    handle->idx = 0U;

    if(!handle->buff){
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t DbgBuffer_Printf(debug_buffer_t *handle, const char *format, ...)
{
    if(!handle->active){
        return ESP_ERR_INVALID_STATE;
    }

    static int delta = 0;
    if(handle->buffer_full){
        return ESP_ERR_INVALID_ARG;
    }

    int n = 0;
    va_list ap;

    va_start(ap, format);
    n = vsnprintf((char*)&handle->buff[handle->idx], handle->buffer_size - handle->idx, format, ap);
    va_end(ap);
    handle->idx += n;

    delta += n;
    if(delta >= 1000){
        // printf("Filled 1KB\n");
        delta = 0;
    }

    if(handle->idx >= handle->buffer_size){
        handle->buffer_full = true;
        if(handle->send_on_full){
            DbgBuffer_Send(handle);
        }
        return ESP_OK;
    }

    handle->buff[handle->idx] = '\0';

    if(n < 0){
        return ESP_ERR_INVALID_ARG;
    }

   
    return ESP_OK;
}

esp_err_t DbgBuffer_PrintHex(debug_buffer_t *handle, uint8_t *buffer, uint16_t len)
{
    if(!handle->active){
        return ESP_ERR_INVALID_STATE;
    }

    if(!handle || !buffer){
        return ESP_ERR_INVALID_ARG;
    }

    if(handle->buffer_full){
        return ESP_ERR_INVALID_STATE;
    }

    if(handle->idx + 3U >= handle->buffer_size){
        handle->buffer_full = true;
        if(handle->send_on_full){
            DbgBuffer_Send(handle);
        }
        return ESP_ERR_NO_MEM;
    }

    // Mark the following section as hex. The following 2 bytes are its size
    handle->buff[handle->idx++] = DBG_BUFFER_HEX_MARKER;
    
    uint16_t to_copy = 0U; 
    if(handle->idx + len + 2U >= handle->buffer_size){
        to_copy = (uint16_t)(handle->buffer_size - handle->idx - 2U);
        handle->buffer_full = true;
    }else {
        to_copy = len;
    }

    memcpy(&handle->buff[handle->idx], &to_copy, 2U);
    handle->idx += 2U;

    // Copy the bytes
    memcpy(&handle->buff[handle->idx], buffer, (size_t)to_copy);
    handle->idx += to_copy;

    if(handle->buffer_full && handle->send_on_full){
        DbgBuffer_Send(handle);
    }

    return ESP_OK;
}

esp_err_t DbgBuffer_Send(debug_buffer_t *handle)
{   
    if(!handle->active){
        return ESP_ERR_INVALID_STATE;
    }

    if(!handle){
        return ESP_ERR_INVALID_ARG;
    }

    const size_t chunk_len = 1024;
    size_t i = 0U, to_send = 0U;


    printf("%s",DBG_START_MSG);
    while(i < handle->buffer_size){
        to_send = (i + chunk_len < handle->buffer_size) ? chunk_len : handle->buffer_size - i;

        fwrite(&handle->buff[i], 1, to_send, stdout);
        fflush(stdout);
        i += to_send;    
    }
    printf("\n=============================== END ===============================\n");

    return ESP_OK;
}