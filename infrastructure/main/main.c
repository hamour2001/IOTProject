#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "lwip/ip4_addr.h"


#define WIFI_SSID "TOPNET_5438"
#define WIFI_PASS "kl4vhuz7xs"

static const char *TAG = "INFRASTRUCTURE";

// HTTP GET handler for /distance
esp_err_t distance_get_handler(httpd_req_t *req) {
    char *buf;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;

    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory for query string");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[16];
            if (httpd_query_key_value(buf, "value", param, sizeof(param)) == ESP_OK) {
                float distance = atof(param);
                ESP_LOGI(TAG, "Received distance: %.2f cm", distance);

                const char *response = (distance < 10) ? "OBSTACLE" : "SECURE";
                ESP_LOGI(TAG, "Sending response: %s", response);
                httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
                free(buf);
                return ESP_OK;
            }
        }

        ESP_LOGE(TAG, "Invalid or missing 'value' query parameter");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid or missing 'value' query parameter");
        free(buf);
        return ESP_ERR_INVALID_ARG;
    } else {
        ESP_LOGE(TAG, "Missing query string");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing query string");
        return ESP_ERR_INVALID_ARG;
    }
}

// URI handler for the /distance endpoint
httpd_uri_t distance = {
    .uri = "/distance",
    .method = HTTP_GET,
    .handler = distance_get_handler,
};

// Start the HTTP server
void start_http_server() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP server...");
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &distance);
        ESP_LOGI(TAG, "HTTP server started on /distance endpoint.");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server.");
    }
}

// Initialize Wi-Fi in AP mode
void init_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif)); // Stop DHCP server temporarily

    esp_netif_ip_info_t ip_info;
// Configure static IP
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif)); // Restart DHCP server

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi AP started. SSID: %s", WIFI_SSID);
}


void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "NVS Flash initialized.");

    // Initialize Wi-Fi
    init_wifi();

    // Start the HTTP server
    start_http_server();
}
