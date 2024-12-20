#include "ultrasonic.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_gpio.h"   // Pour esp_rom_delay_us
#include "esp_timer.h"      // Pour esp_timer_get_time
#include "esp_log.h"        // Pour journalisation

// Définir les broches TRIG et ECHO
#define TRIG_PIN 25
#define ECHO_PIN 14
#define TIMEOUT_US 20000   // Timeout de 20 ms (400 cm max)
static const char *TAG = "ULTRASONIC"; // Tag pour les logs

void init_ultrasonic(void)
{
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_level(TRIG_PIN, 0); // Assurez que le TRIG est bas au démarrage
    ESP_LOGI(TAG, "Ultrasonic sensor initialized. TRIG_PIN: %d, ECHO_PIN: %d", TRIG_PIN, ECHO_PIN);
}

float measure_distance_cm(void)
{
    gpio_set_level(TRIG_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));  // Pause pour assurer un signal propre
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);          // TRIG doit être haut pendant 10 µs
    gpio_set_level(TRIG_PIN, 0);

    uint32_t start = 0, end = 0;
    uint32_t timeout_start = (uint32_t)esp_timer_get_time();

    // Attendre que l'écho devienne haut avec timeout
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - timeout_start) > TIMEOUT_US) {
            ESP_LOGE(TAG, "Timeout waiting for echo HIGH");
            return -1;  // Timeout
        }
        start = (uint32_t)esp_timer_get_time();
    }

    timeout_start = (uint32_t)esp_timer_get_time(); // Réinitialiser le timeout

    // Attendre que l'écho devienne bas avec timeout
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - timeout_start) > TIMEOUT_US) {
            ESP_LOGE(TAG, "Timeout waiting for echo LOW");
            return -1;  // Timeout
        }
        end = (uint32_t)esp_timer_get_time();
    }

    float duration = (float)(end - start);  // Durée en microsecondes
    float distance = duration / 58.0;       // Conversion en cm

    if (distance > 400) {
        ESP_LOGW(TAG, "Distance out of range: %.2f cm", distance);
        return -1;  // Hors de portée (max 4m)
    }

    ESP_LOGI(TAG, "Measured distance: %.2f cm", distance);
    return distance;
}
