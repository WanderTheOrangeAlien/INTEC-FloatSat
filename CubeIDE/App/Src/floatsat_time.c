#include "floatsat_time.h"
#include <string.h>

static uint8_t rtc_weekday(uint16_t y, uint8_t m, uint8_t d);
static inline uint32_t Time_TimeToSeconds(const floatsat_time_t *t);

// Static reference to the single rtc handle so that we can use it in the alarm callback
static rtc_handle_t *s_rtc = NULL;

floatsat_err_t RTC_Init(rtc_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->rtc){
        return ERR_INVALID_ARG;
    }

    if(handle->rtc->Init.HourFormat != RTC_HOURFORMAT_24){
        return ERR_INVALID_ARG;
    }

    s_rtc = handle;

    return ERR_OK;
}

floatsat_err_t RTC_GetTimeDate(rtc_handle_t *handle, floatsat_time_t *time, floatsat_date_t *date)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    taskENTER_CRITICAL();
    HAL_StatusTypeDef ret_time, ret_date;
    ret_time = HAL_RTC_GetTime(handle->rtc, &rtc_time, RTC_FORMAT_BIN);
    ret_date = HAL_RTC_GetDate(handle->rtc, &rtc_date, RTC_FORMAT_BIN);
    taskEXIT_CRITICAL();

    if(ret_time != HAL_OK || ret_date != HAL_OK){
        return ERR_CORE_RTC_READ_FAIL;
    }

    if(time){
        *time = (floatsat_time_t){
            .hours      = rtc_time.Hours,
            .minutes    = rtc_time.Minutes,
            .seconds    = rtc_time.Seconds 
        };
    }

    if(date){
        *date = (floatsat_date_t){
            .day        = rtc_date.Date,
            .month      = rtc_date.Month,
            .year       = rtc_date.Year
        };
    }

    return ERR_OK;
}
floatsat_err_t RTC_SetTimeDate(rtc_handle_t *handle, const floatsat_time_t *time, const floatsat_date_t *date)
{
    if(!handle || !time || !date){
        return ERR_INVALID_ARG;
    }
    RTC_TimeTypeDef rtc_time = {
        .Hours      = time->hours,
        .Minutes    = time->minutes,
        .Seconds    = time->seconds,
    };
    RTC_DateTypeDef rtc_date = {
        .Year       = date->year,
        .Month      = date->month,
        .Date       = date->day,
        .WeekDay    = rtc_weekday(2000U + (uint16_t)date->year, date->month, date->day) 
    };

    HAL_StatusTypeDef ret = HAL_RTC_SetTime(handle->rtc, &rtc_time, RTC_FORMAT_BIN);
    if(ret != HAL_OK){
        return ERR_CORE_RTC_WRITE_FAIL;
    }

    ret = HAL_RTC_SetDate(handle->rtc, &rtc_date, RTC_FORMAT_BIN);
    if(ret != HAL_OK){
        return ERR_CORE_RTC_WRITE_FAIL;
    }

    return ERR_OK;
}

/// @brief Set a new RTC alarm.
/// @param handle 
/// @param time 
/// @param day Day of the month that the alarm fires (1-31). Set 0 to set for today
/// @param callback 
/// @param args 
/// @return 
floatsat_err_t RTC_SetAlarm(rtc_handle_t *handle, 
                            const floatsat_time_t *time, 
                            uint8_t day, 
                            const rtc_callback_t callback, 
                            void *args)
{
    if(!handle || !time || !callback){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    if(day == 0U){
        floatsat_date_t today;
        GOTO_ON_ERR(RTC_GetTimeDate(handle, NULL, &today), err, ret);
        day = today.day;
    }


    RTC_AlarmTypeDef alarm = {
        .AlarmTime = {
            .Hours      = time->hours,
            .Minutes    = time->minutes,
            .Seconds    = time->seconds,
        },
        .AlarmMask              = RTC_ALARMMASK_NONE,
        .AlarmSubSecondMask     = RTC_ALARMSUBSECONDMASK_ALL,
        .AlarmDateWeekDaySel    = RTC_ALARMDATEWEEKDAYSEL_DATE,
        .AlarmDateWeekDay       = day,
        .Alarm                  = RTC_ALARM_A
    };


    GOTO_ON_HAL_ERR(HAL_RTC_DeactivateAlarm(handle->rtc, RTC_ALARM_A), err, ret);
    handle->alarm_callback = callback;
    handle->ctx = args;

    if(HAL_RTC_SetAlarm_IT(handle->rtc, &alarm, RTC_FORMAT_BIN) != HAL_OK){
        return ERR_HAL_ERROR;
    }
    return ERR_OK;

err:
    return ret;

}

floatsat_err_t Time_ParseString(const char *buffer, floatsat_time_t *time)
{
    if(!buffer || !time){
        return ERR_INVALID_ARG;
    }
    
    unsigned int h, m, s; 
    *time = (floatsat_time_t){0}; 

    int ret = sscanf(buffer,"%2u:%2u:%2u", &h, &m, &s);
    
    if(ret < 3 || ret == EOF){
        return ERR_INVALID_TIME;
    }
    
    if(h > 23 || m > 59 || s > 59){
        return ERR_INVALID_TIME;
    }

    time->hours     = (uint8_t)h;
    time->minutes   = (uint8_t)m;
    time->seconds   = (uint8_t)s;

    return ERR_OK;
}

floatsat_err_t Time_ParseDateString(const char *buffer, floatsat_date_t *date)
{
    if(!buffer || !date){
        return ERR_INVALID_ARG;
    }
    memset(date, 0, sizeof(floatsat_date_t));
    
    unsigned int y, m, d;
    int ret = sscanf(buffer, "%2u/%2u/%2u",&m, &d, &y);
    if(ret < 3){
        return ERR_INVALID_ARG;
    }

    if(m > 12U || d > 31U || d == 0U || y < 26U){
        return ERR_INVALID_ARG;
    }

    date->year  = (uint8_t)y;
    date->month = (uint8_t)m;
    date->day   = (uint8_t)d;

    return ERR_OK;
}

floatsat_err_t Time_ParseTimeDateString(const char *buffer, floatsat_time_t *time, floatsat_date_t *date)
{
    if(!buffer || !time || !date){
        return ERR_INVALID_ARG;
    }

    // Expected format: MM/dd/yy hh:mm:ss

    // Find the space
    char date_str[16] = {0}; 
    const char *p = strchr(buffer, ' ');
    if(!p){
        return ERR_INVALID_ARG;
    }

    memcpy(date_str, buffer, (size_t)(p - buffer));


    return ERR_OK;
}

/// @brief Compares two time structures
/// @param a Non-NULL pointer to first time
/// @param b Non-NULL pointer to second time
/// @return -1 if `a` < `b`, 0 if `a` == `b`, and 1 if `a > b`.
int Time_CompareTime(const floatsat_time_t *a, const floatsat_time_t *b)
{
    configASSERT(a != NULL && b != NULL);

    uint32_t a_secs = (3600U * (uint32_t)a->hours) + (60U * (uint32_t)a->minutes) + (uint32_t)a->seconds;
    uint32_t b_secs = (3600U * (uint32_t)b->hours) + (60U * (uint32_t)b->minutes) + (uint32_t)b->seconds;

    return (a_secs > b_secs) - (a_secs < b_secs);
}

/// @brief Returns the difference in seconds between two time structs 
/// @param a 
/// @param b 
/// @return 
int Time_Delta(const floatsat_time_t *a, const floatsat_time_t *b)
{
    configASSERT(a != NULL && b != NULL);

    int a_secs = (int)Time_TimeToSeconds(a);
    int b_secs = (int)Time_TimeToSeconds(b);

    return a_secs - b_secs;
}

floatsat_err_t Time_SecondsToTime(uint32_t seconds, floatsat_time_t *time)
{
    if(!time || seconds > (24U * 3600U)){
        return ERR_INVALID_ARG;
    }

    time->hours     = (uint8_t)(seconds / 3600U);
    time->minutes   = (uint8_t)((seconds % 3600U) / 60U);
    time->seconds   = (uint8_t)(seconds % 60U);

    return ERR_OK;
}

floatsat_err_t Time_SubtractSeconds(floatsat_time_t *time, uint32_t seconds, floatsat_time_t *new_time)
{
    uint32_t s = Time_TimeToSeconds(time);

    if(s < seconds){
        return ERR_INVALID_ARG;
    }

    s -= seconds;
    return Time_SecondsToTime(s, new_time); 
}


// Sakamoto's algorithm
static uint8_t rtc_weekday(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    uint8_t w = (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);  // 0 = Sunday
    return (w == 0) ? RTC_WEEKDAY_SUNDAY : w;   // HAL: Mon=1 .. Sat=6, Sun=7
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    UNUSED(hrtc);
    if(!s_rtc){
        return;
    }

    if(s_rtc->alarm_callback){
        s_rtc->alarm_callback(s_rtc);
    }
}

static inline uint32_t Time_TimeToSeconds(const floatsat_time_t *t)
{
    return (3600U * (uint32_t)t->hours) + (60U * (uint32_t)t->minutes) + (uint32_t)t->seconds;

}