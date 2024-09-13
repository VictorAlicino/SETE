#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define FOREVER while(true)
#define RED_LED GPIO_NUM_37
#define GREEN_LED GPIO_NUM_36
#define BLUE_LED GPIO_NUM_35
#define PIR GPIO_NUM_13

int people_count = 0;

extern "C"
{
    void app_main(void);
}

void people_counter(void *pvParameters)
{
    FOREVER
    {
        printf("--------------------\n");
        printf("People count: %d\n", people_count/2);
        printf("--------------------\n");
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
}

void people_detector(void *pvParameters)
{
    FOREVER
    {
        int state = gpio_get_level(PIR);
        people_count += state;
        gpio_set_level(BLUE_LED, 1);
        gpio_set_level(RED_LED, state);
        gpio_set_level(GREEN_LED, !state);
        vTaskDelay(2500/portTICK_PERIOD_MS);
        gpio_set_level(BLUE_LED, 0);
        printf("Accumulator: %d\n", people_count);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    printf("Hello World ESP32-S3!!\n");

    TaskHandle_t p_people_accumulator = NULL;
    TaskHandle_t p_people_counter = NULL;

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

    xTaskCreate(people_detector, "people_accumulator", 2048, NULL, 1, &p_people_accumulator);
    xTaskCreate(people_counter, "people_counter", 2048, NULL, 1, &p_people_counter);
}
