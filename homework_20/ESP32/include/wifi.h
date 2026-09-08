#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

static inline void wifi_init_sta(const wifi_config_t *wifi_config) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Передаємо конфігурацію за вказівником
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Connecting to WiFi SSID: %s...\n", wifi_config->sta.ssid);
    esp_wifi_connect();
}

#endif // WIFI_CONFIG_H