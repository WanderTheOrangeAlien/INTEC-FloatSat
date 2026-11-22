#ifndef FLOATSAT_TIME_H
#define FLOATSAT_TIME_H

#include "stm32f4xx_hal.h"

#include "floatsat_types.h"
#include "floatsat_err_log.h"

#define MAX_FLOATSAT_TIME()   ((floatsat_time_t){23U, 59U, 59U})

typedef struct rtc_handle_t rtc_handle_t;

typedef void (*rtc_callback_t)(rtc_handle_t *handle);

typedef struct rtc_handle_t {
    RTC_HandleTypeDef   *rtc;
    rtc_callback_t      alarm_callback;
    void                *ctx;
}rtc_handle_t;


floatsat_err_t RTC_Init(rtc_handle_t *handle);

floatsat_err_t RTC_GetTimeDate(rtc_handle_t *handle, floatsat_time_t *time, floatsat_date_t *date);
floatsat_err_t RTC_SetTimeDate(rtc_handle_t *handle, const floatsat_time_t *time, const floatsat_date_t *date);
floatsat_err_t RTC_SetAlarm(rtc_handle_t *handle, 
                            const floatsat_time_t *time, 
                            uint8_t day, 
                            const rtc_callback_t callback, 
                            void *args);

floatsat_err_t Time_ParseString(const char *buffer, floatsat_time_t *time);
floatsat_err_t Time_ParseDateString(const char *buffer, floatsat_date_t *date);
floatsat_err_t Time_ParseTimeDateString(const char *buffer, floatsat_time_t *time, floatsat_date_t *date);

int Time_CompareTime(const floatsat_time_t *a, const floatsat_time_t *b);
int Time_Delta(const floatsat_time_t *a, const floatsat_time_t *b);

floatsat_err_t Time_SecondsToTime(uint32_t seconds, floatsat_time_t *time);
floatsat_err_t Time_SubtractSeconds(floatsat_time_t *time, uint32_t seconds, floatsat_time_t *new_time);


#endif