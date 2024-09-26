#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/ringbuf.h"
#include "ld2461.hpp"

#include <string.h>

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    // TODO: Load board informations

    LD2461 ld2461 = LD2461(
        UART_NUM_2,
        GPIO_NUM_36,
        GPIO_NUM_35,
        9600
    );

}
