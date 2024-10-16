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

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "sensor.hpp"
#include "ld2461.hpp"
#include "pir.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"

// LED GPIOs
#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37

// GLobal Variables
MQTT* mqtt;
Sensor* sensor;

// Local Variables
static const char *TAG = "SET003";

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
    WiFi_STA wifi = WiFi_STA();

    // Initialize Board
    sensor = new Sensor();

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // TODO: Load board informations

    // Initialize GPIO
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    // Initialize Sensors
    LD2461 ld2461 = LD2461(
        UART_NUM_2,
        GPIO_NUM_36,
        GPIO_NUM_35,
        9600
    );
    PIR pir = PIR(GPIO_NUM_48);

    // Initialize MQTT
    mqtt = new MQTT("mqtt://144.22.195.55:1883");



    ld2461_frame_t ld2461_frame = ld2461_setup_frame();
    ld2461_version_t ld2461_version = ld2461.get_version_and_id(&ld2461_frame);
    ESP_LOGI(TAG, "Detected LD2461 running on v%01X.%01X from %d/%d/%d",
                    ld2461_version.major, ld2461_version.minor,
                    ld2461_version.month,
                    ld2461_version.day,
                    ld2461_version.year);
    
    std::string ld2461_payload = "";
    std::string pir_payload = "";
    std::string payload = "";
    while(true)
    {
        ld2461.read_data(&ld2461_frame);
        ld2461_payload = ld2461.detection_to_json(&ld2461_frame);
        pir_payload = pir.read_to_json();
        payload = (
            "{\"" +
                sensor->get_designator() +
                    "\":{\"ld2461\":" +
                        ld2461_payload +
                    ",\"pir\":" +
                        pir_payload +
            "}}"
        );
        mqtt->publish(
            std::string(sensor->get_mqtt_root_topic() + "/json").c_str(),
            payload.c_str()
        );
        ESP_LOGI(TAG, 
            "Internal Temperature: %.2f°C | Free memory: %" PRIu32 " bytes | RSSI: %d dBm | Uptime: %llds | Last boot reason: %d",
            sensor->get_internal_temperature(),
            esp_get_free_heap_size(),
            wifi.get_rssi(),
            esp_timer_get_time() / 1000000,
            esp_reset_reason()
        );
        printf("%s", ld2461.frame_to_string(&ld2461_frame));
        printf(" -> ");
        ld2461.report_detections();
        if(pir.read() == 1){
            printf("## PIR: Detected |\n");
        }
        else{
            printf("## PIR: Not Detected |\n");
        }
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
