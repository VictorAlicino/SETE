#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

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
#include "esp_log.h"
#include "esp_timer.h"
#include <string>

// App Headers
#include "sensor.hpp"
#include "ld2461.hpp"
#include "pir.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "detection.hpp"

// LED GPIOs
#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37

// Local Variables
static const char *TAG = "SET003";

// GLobal Variables
MQTT* mqtt;
Sensor* sensor;
LD2461* ld2461;
PIR* pir;
extern bool mqtt_connected;

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize WiFi
    WiFi_STA wifi = WiFi_STA("50 centavos a hora", "duzentoseoito");

    // Initialize Board
    sensor = new Sensor();

    ESP_LOGI(TAG, "Initializing program");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // Initialize GPIO
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    // Initialize Sensors
    ld2461 = new LD2461(
        UART_NUM_2,
        GPIO_NUM_36,
        GPIO_NUM_35,
        9600
    );
    pir = new PIR(GPIO_NUM_48);

    // Initialize MQTT
    mqtt = new MQTT("mqtt://144.22.195.55:1883");

    // Transfer LOGs to MQTT
    if(mqtt_connected){
        sensor->transfer_log_to_mqtt();
    }

    { // This variables are not needed after this, so we will create a new scope to free them after
    // To be sure of the LD2461 initialization, we will read the firmware version from it
    ld2461_frame_t ld2461_frame = ld2461_setup_frame();
    ld2461_version_t ld2461_version = ld2461->get_version_and_id(&ld2461_frame);
    ESP_LOGI(TAG, "Detected LD2461 running on v%01X.%01X from %d/%d/%d",
                    ld2461_version.major, ld2461_version.minor,
                    ld2461_version.month,
                    ld2461_version.day,
                    ld2461_version.year);
    }

    // Initialize Detection
    Detection detection = Detection(
        {-4, 2},
        {-4, 0.5},
        {4, 0.5},
        {4, 2}
    );

    // Initialize Variables
    std::string sensor_state;
    detection.start_detection();
    // Main Loop
    while(true)
    {
        detection.detect();
        sensor_state = (
            "{"
                "\"internal_temperature\": " + std::to_string(sensor->get_internal_temperature()) + ","
                "\"free_memory\": " + std::to_string(esp_get_free_heap_size()) + ","
                "\"rssi\": " + std::to_string(wifi.get_rssi()) + ","
                "\"uptime\": " + std::to_string(esp_timer_get_time() / 1000000) + ","
                "\"last_boot_reason\": " + std::to_string(esp_reset_reason()) +
            "}"
        );
        mqtt->publish(
            std::string(sensor->get_mqtt_root_topic() + "/info").c_str(),
            sensor_state.c_str()
        );
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}