#ifndef FLOATSAT_CORE_H
#define FLOATSAT_CORE_H

#include "floatsat_core_types.h"

#define CORE_CMD_QUEUE_LEN              4
#define CORE_MAG_CALIBRATION_TIME_MS    4000U

void Task_CoreFn(void *args);
floatsat_err_t Core_Init(floatsat_core_t *handle);


#endif