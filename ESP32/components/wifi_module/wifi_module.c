#include <stdio.h>
#include "wifi_module.h"

static EventGroupHandle_t s_wifi_event_group;
static uint32_t s_retry_num = 0U;

static const char *TAG = "WIFI"; 

static void wifi_event_handler(void *arg, esp_event_base_t event_base, 
                                int32_t event_id, void *event_data )
{
    if(event_base == WIFI_EVENT){

        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            if(s_retry_num < WIFI_MAX_RETRY_NUM){
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }else{
                ESP_LOGW(TAG, "Connection to AP failed");
                s_retry_num = 0U;
                xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_FAIL);

            }
            break;
        
        default:
            break;
        }       


    }else if(event_base == IP_EVENT){
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:{
            ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0U;
            xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_CONNECTED);
            break;
        }    

        
        default:
            break;
        }
    }

} 

static esp_err_t wifi_init_ap(wifi_module_config_t *config)
{
    if(!config){
        return ESP_ERR_INVALID_ARG;   
    }


    return ESP_OK;
}

static esp_err_t wifi_init_sta(wifi_module_config_t *config)
{
    if(!config){
        return ESP_ERR_INVALID_ARG;   
    }

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strncpy((char*)&wifi_config.sta.ssid, (char*)config->ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)&wifi_config.sta.password, (char*)config->password, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_BIT_CONNECTED | WIFI_BIT_FAIL,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_BIT_CONNECTED) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s\n\n",
                 (char*)wifi_config.sta.ssid, (char*)wifi_config.sta.password);
    } else if (bits & WIFI_BIT_FAIL) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 (char*)wifi_config.sta.ssid, (char*)wifi_config.sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }


    return ESP_OK;
}

esp_err_t Wifi_Init(wifi_module_config_t *config)
{
    if(!config){
        return ESP_ERR_INVALID_ARG;
    }

    nvs_flash_init();
    if(config->wifi_mode == WIFI_MODE_STA){
        return wifi_init_sta(config);
    }else if(config->wifi_mode == WIFI_MODE_AP){
        return wifi_init_ap(config);
    }else{
        return ESP_ERR_INVALID_ARG;
    }


    return ESP_OK;

}
