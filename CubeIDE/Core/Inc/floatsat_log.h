#ifndef INC_FLOATSAT_LOG_H
#define INC_FLOATSAT_LOG_H

#include "stm32f4xx_hal.h"
#include <stdio.h>

#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_WARNING   1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_DEBUG     3

#define LOG_COLORCODE_BLACK   "30"
#define LOG_COLORCODE_RED     "31"
#define LOG_COLORCODE_GREEN   "32"
#define LOG_COLORCODE_BROWN   "33"
#define LOG_COLORCODE_BLUE    "34"
#define LOG_COLORCODE_PURPLE  "35"
#define LOG_COLORCODE_CYAN    "36"

#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"

#define LOG_COLOR_BLACK   LOG_COLOR(LOG_COLORCODE_BLACK)
#define LOG_COLOR_RED     LOG_COLOR(LOG_COLORCODE_RED)
#define LOG_COLOR_GREEN   LOG_COLOR(LOG_COLORCODE_GREEN)
#define LOG_COLOR_BROWN   LOG_COLOR(LOG_COLORCODE_BROWN)
#define LOG_COLOR_BLUE    LOG_COLOR(LOG_COLORCODE_BLUE)
#define LOG_COLOR_PURPLE  LOG_COLOR(LOG_COLORCODE_PURPLE)
#define LOG_COLOR_CYAN    LOG_COLOR(LOG_COLORCODE_CYAN)

#define LOG_COLOR_RESET   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR_RED
#define LOG_COLOR_W       LOG_COLOR_BROWN
#define LOG_COLOR_I       LOG_COLOR_GREEN
#define LOG_COLOR_D       LOG_COLOR_RESET

#if LOG_LEVEL >= LOG_LEVEL_ERROR
    #define LOGE(tag, format, ...) \
        do { printf(LOG_COLOR_E "(%lu) [%s] " format LOG_COLOR_RESET "\n", HAL_GetTick(), tag, ##__VA_ARGS__); } while(0)
#else
    #define LOGE(tag, format, ...) do {} while(0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
    #define LOGW(tag, format, ...) \
        do { printf(LOG_COLOR_W "(%lu) [%s] " format LOG_COLOR_RESET "\n", HAL_GetTick(), tag, ##__VA_ARGS__); } while(0)
#else
    #define LOGW(tag, format, ...) do {} while(0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
    #define LOGI(tag, format, ...) \
        do { printf(LOG_COLOR_I "(%lu) [%s] " format LOG_COLOR_RESET "\n", HAL_GetTick(), tag, ##__VA_ARGS__); } while(0)
#else
    #define LOGI(tag, format, ...) do {} while(0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define LOGD(tag, format, ...) \
        do { printf(LOG_COLOR_D "(%lu) [%s] " format LOG_COLOR_RESET "\n", HAL_GetTick(), tag, ##__VA_ARGS__); } while(0)
#else
    #define LOGD(tag, format, ...) do {} while(0)
#endif

#endif