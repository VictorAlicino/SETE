#include <stdio.h>
#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FOREVER while(true)
#define RED_LED GPIO_NUM_37
#define GREEN_LED GPIO_NUM_36
#define BLUE_LED GPIO_NUM_35
#define PIR GPIO_NUM_13

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    printf("Hello World ESP32-S3!!\n");
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_config_t pir_config = {
        .pin_bit_mask = (1ULL << 13),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t pir_pin = gpio_config(&pir_config);

    if (pir_pin != ESP_OK)
    {
        printf("An error has occured on pin 13\n");
        esp_restart();
    }

    FOREVER
    {
        int state = gpio_get_level(PIR);
        gpio_set_level(BLUE_LED, 1);
        gpio_set_level(RED_LED, state);
        gpio_set_level(GREEN_LED, !state);
        vTaskDelay(10/portTICK_PERIOD_MS);
        gpio_set_level(BLUE_LED, 0);
    }
}
