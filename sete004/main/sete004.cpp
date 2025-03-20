#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define DEBUG 1

#ifdef DEBUG
#if DEBUG == 1
#pragma message("DEBUG MODE ENABLED")
#endif
#endif

// IDF Headers
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
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "time.h"
#include "sys/time.h"
#include <string>

#include <cstdlib>

// App Headers
#include "sensor.hpp"
#include "ld2461.hpp"
#include "pir.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "detection.hpp"
#include "storage.hpp"

// LED GPIOs
#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37

// Local Variables
static const char *TAG = "SET003";

// Global Variables
Storage* storage;
MQTT* mqtt;
Sensor* sensor;
LD2461* ld2461;
PIR* pir;
WiFi_STA* wifi;
Detection* detection;
extern bool mqtt_connected;

bool flag_0 = true;

extern "C" {
    void app_main(void);
}

void app_main(void)
{
    ESP_LOGI("SET003", "Firmware Compiled on [ %s @ %s ]", __DATE__, __TIME__);

    // Initialize the NVS
    storage = new Storage();

    // Initialize WiFi
    #if DEBUG == 1
    wifi = new WiFi_STA("50 centavos a hora", "duzentoseoito");
    #else
    //wifi = new WiFi_STA("UniFi SeteServicos", "6X.Pa1&bfF");
    wifi = new WiFi_STA("Farma A Filial", "Filial#2200");
    #endif
    //wifi = new WiFi_STA("CAMPOS_EXT", "salsicha");

    // Initialize Board
    sensor = new Sensor();

    // Initialize GPIO
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(RED_LED, 1);

    // Initialize MQTT
    mqtt = new MQTT("mqtt://144.22.195.55:1883");

    // Transfer LOGs to MQTT
    if(mqtt_connected){
        sensor->transfer_log_to_mqtt();
    }

    ESP_LOGI(TAG, "Initializing program");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
}
