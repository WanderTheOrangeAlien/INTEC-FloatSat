#include "floatsat_log.h"
#include <string.h>

static int ringbuff_push(ringbuff_t *ringbuff, uint8_t *data, size_t data_len);
static int ringbuff_pop(ringbuff_t *ringbuff, uint8_t *data, size_t data_len);
static void Log_AddErrorCount();

static char log_buffer[LOG_BUFFER_SIZE] = {0};
static char temp_buffer[LOG_MAX_MSG_SIZE] = {0};

static uint32_t messages_dropped = 0U; 

static ringbuff_t ringbuff = {
    .buffer     = (uint8_t*)log_buffer,
#ifndef TEST
    .max_len    = LOG_BUFFER_SIZE
#else
    .max_len    = TEST_LOG_BUFFER_SIZE 
#endif
};

static const char error_msg[] = "MAX LOG STRING SIZE EXCEEDED\n";

/// @brief Log to internal buffer instead of sending it through hardware
///  Messages must be separated with a newline character (\n)
/// @param format 
void Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int written = vsnprintf(temp_buffer, LOG_MAX_MSG_SIZE , format, args);

    if(written >= LOG_MAX_MSG_SIZE){
        // Welp, an error when logging, this is awkward
        // Replace the message with "MAX LOG STRING LENGTH"
        strcpy(temp_buffer, error_msg);
        written = strlen(error_msg);
    }
    
    // Copy bytes as circular buffer. -1 to leave out the NULL terminator
    int ret = ringbuff_push(&ringbuff, (uint8_t*)temp_buffer, written);
    
    if(ret != 0){
        // TODO: Raise Log buffer full error
    }

}


void Log_ResetBuffer()
{
    memset(ringbuff.buffer, 0, ringbuff.max_len);
    ringbuff.full = false;
    ringbuff.head = 0;
    ringbuff.tail = 0;
}

int Log_PopNextMsg(uint8_t *buffer, size_t *out_msg_size)
{
    // Get the length of the message
    uint8_t *p = (uint8_t*)strchr((char*)&ringbuff.buffer[ringbuff.tail],'\n');

    if(!p){

        // Check for wrap around
        if(ringbuff.head < ringbuff.tail){
            p = (uint8_t*)strchr((char*)ringbuff.buffer,'\n');
            if(!p){
                return -2;
            }

            size_t offset = (size_t)(p - ringbuff.buffer);
            *out_msg_size = ringbuff.max_len - ringbuff.tail + offset;

        }else{
            // No new message or invalid fromat
            Log_AddErrorCount();
            return - 1;
        }
        
    }else{
        *out_msg_size = (size_t)(p - &ringbuff.buffer[ringbuff.tail] + 1U);
    }


    int ret = ringbuff_pop(&ringbuff, buffer, *out_msg_size);

    if(ret != 0){
        Log_AddErrorCount();
        return ret;
    }
}

static void Log_AddErrorCount()
{
    // If an error occurs when logging, add to the counter
    messages_dropped++;

}

/* Dumb circular buffer implementation */
static int ringbuff_push(ringbuff_t *ringbuff, uint8_t *data, size_t data_len)
{
    if(ringbuff->full){
        return -1;
    }

    int space = 0;
    if(ringbuff->head < ringbuff->tail){
        // The buffer has wrapped around
        space = ringbuff->tail - ringbuff->head;
    }else{
        space = ringbuff->max_len - (ringbuff->head - ringbuff->tail);
    }

    if(space < data_len){
        return -1;
    }

    int next = ringbuff->head + data_len;

    if(next > ringbuff->max_len){
        // Wrap around
        int bytes_to_end = ringbuff->max_len - ringbuff->head;
        next = next - ringbuff->max_len;   
        if(next > ringbuff->tail){
            // Not enough space
            return -1;
        }
        memcpy(&ringbuff->buffer[ringbuff->head], data, bytes_to_end);
        memcpy(ringbuff->buffer, &data[bytes_to_end], data_len - bytes_to_end);
    }else{
        memcpy(&ringbuff->buffer[ringbuff->head], data, data_len);
    }
    ringbuff->head = next;

    ringbuff->full = (ringbuff->head == ringbuff->tail);

    return 0;
}

static int ringbuff_pop(ringbuff_t *ringbuff, uint8_t *data, size_t data_len)
{
    if(ringbuff->head == ringbuff->tail && !ringbuff->full){
        return -1;
    }

    // Check for available space
    int space = 0;
    if(ringbuff->head < ringbuff->tail){
        // The buffer has wrapped around
        space = ringbuff->max_len - (ringbuff->tail - ringbuff->head);
    }else{
        space = ringbuff->head - ringbuff->tail;
    }

    if(space < data_len){
        return -1;
    }

    int next = ringbuff->tail + data_len;
    if(next > ringbuff->max_len){
        // Wrap around
        int bytes_to_end = ringbuff->max_len - ringbuff->tail;
        next = next - ringbuff->max_len;    

        memcpy(data, &ringbuff->buffer[ringbuff->tail], bytes_to_end);
        memcpy(&data[bytes_to_end], ringbuff->buffer, data_len - bytes_to_end);
    }else{
        memcpy(data, &ringbuff->buffer[ringbuff->tail], data_len);
    }
    ringbuff->tail = next;
    return 0;

    
}

const ringbuff_t *Log_GetRingbuff(void)
{
    return (const ringbuff_t *)&ringbuff;
}