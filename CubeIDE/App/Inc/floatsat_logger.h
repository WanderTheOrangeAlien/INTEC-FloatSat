#ifndef FLOATSAT_LOGGER_H
#define FLOATSAT_LOGGER_H

#include <string.h>
#include "floatsat_error.h"

#define INFO_BUFFER_WAIT_DELAY_TICKS  10 // Ticks to wait when failed to acquire the info buffer due to it being already taken

void Task_LogFn(void *params);


#endif