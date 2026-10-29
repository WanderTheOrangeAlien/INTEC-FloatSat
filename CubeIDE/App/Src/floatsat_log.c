#include "floatsat_log.h"
#include <string.h>


static int ringbuff_push(ringbuff_t *ringbuff, uint8_t *data, size_t data_len);
static int ringbuff_pop(ringbuff_t *ringbuff, uint8_t *data, size_t data_len);
static void Log_AddErrorCount(void);

static char log_buffer[LOG_BUFFER_SIZE] = {0};
static char temp_buffer[LOG_MAX_MSG_SIZE] = {0};

static uint32_t messages_dropped = 0U;
static uint32_t n_init_msg = 0U; 
static TaskHandle_t logger_task_handle = NULL;

static ringbuff_t ringbuff = {
    .buffer     = (uint8_t*)log_buffer,
    .max_len    = LOG_BUFFER_SIZE
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
    
    // Copy bytes as circular buffer.
    int ret = ringbuff_push(&ringbuff, (uint8_t*)temp_buffer, (size_t)written + 1U);
    
    if(ret != 0){
        // TODO: Raise Log buffer full error
    }
    
    va_end(args);
    if(logger_task_handle != NULL){

        // Increment the amount of pending messages (this is the normal operation)
        xTaskNotify(logger_task_handle, 1U, eIncrement);
        
    }else{
        // Logging was called before Task initialization. Increment the amount of init msg
        n_init_msg++;

    }
}


void Log_ResetBuffer(void)
{
    memset(ringbuff.buffer, 0, ringbuff.max_len);
    ringbuff.full = false;
    ringbuff.head = 0;
    ringbuff.tail = 0;
}

// TOOO: Change the return types to floatsat_err_t

floatsat_err_t Log_PopNextMsg(uint8_t *buffer, size_t *out_msg_size, size_t buffer_size)
{
    // Get the length of the next message. All messages end with a NULL terminator
    uint8_t *p = (uint8_t*)strchr((char*)&ringbuff.buffer[ringbuff.tail],'\0');

    if(!p){

        // Check for wrap around
        if(ringbuff.head < ringbuff.tail){
            p = (uint8_t*)strchr((char*)ringbuff.buffer,'\0');
            if(!p){
                // There is not a single \n character in the buffer
                return ERR_INVALID_STATE;
            }

            size_t offset = (size_t)(p - ringbuff.buffer);
            *out_msg_size = ringbuff.max_len - ringbuff.tail + offset;

        }else{
            // No new message or invalid fromat
            Log_AddErrorCount();
            return ERR_INVALID_STATE;
        }
        
    }else{
        *out_msg_size = ((size_t)(p - &ringbuff.buffer[ringbuff.tail]) + 1U);
    }

    if(buffer_size < *out_msg_size){
        return ERR_BUFFER_OVERFLOW;
    }


    int ret = ringbuff_pop(&ringbuff, buffer, *out_msg_size);
    if(ret != 0){
        Log_AddErrorCount();
        return ERR_INVALID_STATE;
    }
    // Since this is the last message, end the info buffer with an END_OF_TEXT character
    buffer[*out_msg_size] = ASCII_ETX;
    return ERR_OK;
}
/// @brief Set the task to which the LOGx functions will send notifications. This will immediately send a notification with
/// the amount of pending init messages
/// @param task_handle Logger Task handle
void Log_SetLoggerTaskHandle(TaskHandle_t task_handle)
{
    logger_task_handle = task_handle;

    // Set the amount of messages pending that were created during init
    xTaskNotify(logger_task_handle,n_init_msg,eSetValueWithOverwrite);
    n_init_msg = 0U;
}

static void Log_AddErrorCount()
{
    // If an error occurs when logging, add to the counter
    messages_dropped++;

}

/* Dumb circular buffer implementation */
static int ringbuff_push(ringbuff_t *buff, uint8_t *data, size_t data_len)
{
	// important: data_len includes the NULL terminator


    if(buff->full){
        return -1;
    }

    // Available space for the ringbuffer to grow
    size_t space = 0;
    if(buff->head < buff->tail){
        // The buffer has wrapped around
        space = buff->tail - buff->head;
    }else{
        space = buff->max_len - (buff->head - buff->tail);
    }

    if(space < data_len){
        return -1;
    }

    size_t next = buff->head + data_len;

    if(next > buff->max_len){
        // Wrap around
        size_t bytes_to_end = (buff->max_len - buff->head);
        next = next - buff->max_len;   
        if(next > buff->tail){
            // Not enough space
            return -1;
        }
        memcpy(&buff->buffer[buff->head], data, bytes_to_end);
        memcpy(buff->buffer, &data[bytes_to_end], data_len - bytes_to_end);
        buff->buffer[data_len - bytes_to_end] = '\0';
    }else{
        memcpy(&buff->buffer[buff->head], data, data_len);
        buff->buffer[buff->head + data_len] = '\0';
    }
    buff->head = next;

    buff->full = (buff->head == buff->tail);

    return 0;
}

static int ringbuff_pop(ringbuff_t *buff, uint8_t *data, size_t data_len)
{
    if(buff->head == buff->tail && !buff->full){
        return -1;
    }

    // Amount of bytes that the buffer currently holds
    size_t buff_len = 0;
    if(buff->head < buff->tail){
        // The buffer has wrapped around
        buff_len = buff->max_len - (buff->tail - buff->head);
    }else{
        buff_len = buff->head - buff->tail;
    }

    if(buff_len < data_len){
        return -1;
    }

    // How many bytes should the tail advance.
    size_t next = buff->tail + data_len;
    if(next > buff->max_len){
        // Wrap around
        size_t bytes_to_end = buff->max_len - buff->tail;
        next = next - buff->max_len;    

        memcpy(data, &buff->buffer[buff->tail], bytes_to_end);
        memcpy(&data[bytes_to_end], buff->buffer, data_len - bytes_to_end);
    }else{
        memcpy(data, &buff->buffer[buff->tail], data_len);
    }
    buff->tail = next;
    return 0;

    
}

const ringbuff_t *Log_GetRingbuff(void)
{
    return (const ringbuff_t *)&ringbuff;
}
