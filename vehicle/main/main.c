#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "ultrasonic.h"
#include "stepper.h"
#include "nvs_flash.h"

#define WIFI_SSID "TOPNET_5438"
#define WIFI_PASS "kl4vhuz7xs"
#define INFRASTRUCTURE_IP "192.168.4.1"
#define HTTP_ENDPOINT "/distance"

static const char *TAG = "VEHICLE";

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to Wi-Fi.");
    }
}

void wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void send_distance_and_receive_action(float distance) {
    char url[128];
   snprintf(url, sizeof(url), "http://192.168.4.1/distance?value=%.2f", distance);


    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000, // 5-second timeout
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        if (content_length > 0) {
            char response[64] = {0};
            esp_http_client_read(client, response, sizeof(response) - 1);
            ESP_LOGI(TAG, "Received action: %s", response);

            if (strcmp(response, "SECURE") == 0) {
                ESP_LOGI(TAG, "Path secure, moving forward.");
                stepper_forward();
            } else if (strcmp(response, "OBSTACLE") == 0) {
                ESP_LOGW(TAG, "Obstacle detected, stopping.");
                stepper_stop();
            } else {
                ESP_LOGW(TAG, "Unexpected response: %s", response);
            }
        } else {
            ESP_LOGE(TAG, "Empty response received.");
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

   if (client) {
    esp_http_client_cleanup(client);
}

}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "NVS Flash initialized.");

    wifi_init();
    init_ultrasonic();
    init_stepper();

    while (1) {
        float distance = measure_distance_cm();
        if (distance < 0) {
            ESP_LOGW(TAG, "Invalid distance measurement. Retrying...");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        ESP_LOGI(TAG, "Measured distance: %.2f cm", distance);
        send_distance_and_receive_action(distance);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
