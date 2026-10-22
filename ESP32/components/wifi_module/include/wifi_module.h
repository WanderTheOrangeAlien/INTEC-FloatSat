#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"


// #define WIFI_MODE_STA   0
// #define WIFI_MODE_AP   1

#define WIFI_BIT_CONNECTED      (1 << 0)
#define WIFI_BIT_FAIL           (1 << 1)

#define WIFI_MAX_RETRY_NUM      10

typedef struct wifi_module_config_t {
    const char *ssid;
    const char *password;
    wifi_mode_t wifi_mode;
}wifi_module_config_t;


esp_err_t Wifi_Init(wifi_module_config_t *config);

#endif