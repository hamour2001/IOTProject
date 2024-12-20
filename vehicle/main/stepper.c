#include "stepper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define IN1 33
#define IN2 25
#define IN3 26
#define IN4 27

static int stepIndex = 0;
static const int steps[4][4] = {
    {1, 0, 1, 0},
    {1, 0, 0, 1},
    {0, 1, 0, 1},
    {0, 1, 1, 0}
};

void init_stepper(void) {
    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);
}

void stepper_forward(void) {
    gpio_set_level(IN1, steps[stepIndex][0]);
    gpio_set_level(IN2, steps[stepIndex][1]);
    gpio_set_level(IN3, steps[stepIndex][2]);
    gpio_set_level(IN4, steps[stepIndex][3]);

    stepIndex = (stepIndex + 1) % 4;
    vTaskDelay(pdMS_TO_TICKS(5));
}

void stepper_stop(void) {
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
}
