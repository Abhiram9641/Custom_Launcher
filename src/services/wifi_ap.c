#include "wifi_ap.h"
#include "../config/sys_config.h"
#include "../system/sys_mgr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include <string.h>

#include "../system/sys_mgr.h"

static const char *TAG = "WIFI_AP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
        sys_mgr_send_ui_msg(OTA_STATE_CONNECTED, 0.0f, 0.0f, "Client Connected");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                 MAC2STR(event->mac), event->aid);
        sys_mgr_send_ui_msg(OTA_STATE_IDLE, 0.0f, 0.0f, "Awaiting Connection");
    }
}

void wifi_ap_init(void) {
    ESP_LOGI(TAG, "Initializing Native ESP-IDF WiFi SoftAP");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.ap.ssid, APP_WIFI_SSID, sizeof(wifi_config.ap.ssid) - 1);
    strcpy((char*)wifi_config.ap.password, APP_WIFI_PASS);
    wifi_config.ap.ssid_len = strlen(APP_WIFI_SSID);
    wifi_config.ap.channel = APP_WIFI_CHANNEL;
    wifi_config.ap.max_connection = APP_WIFI_MAX_CONN;
    wifi_config.ap.authmode = (strlen(APP_WIFI_PASS) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP initialized successfully. SSID: %s", APP_WIFI_SSID);
}
