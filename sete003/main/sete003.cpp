#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define DEBUG 0

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

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    ESP_LOGI("SET003", "Firmware Compiled on [ %s @ %s ]", __DATE__, __TIME__);
    // Initialize Storage (NVS)
    storage = new Storage();

    // Initialize WiFi
    #if DEBUG == 1
    wifi = new WiFi_STA("50 centavos a hora", "duzentoseoito");
    #else
    wifi = new WiFi_STA("Farma A Filial", "Filial#2200");
    #endif
    //wifi = new WiFi_STA("CAMPOS_EXT", "salsicha");

    // Initialize Board
    sensor = new Sensor();

    ESP_LOGI(TAG, "Initializing program");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // Initialize GPIO
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(RED_LED, 1);

    // Initialize Sensors
    ld2461 = new LD2461(
        UART_NUM_2,
        GPIO_NUM_36,    // TX Pin
        GPIO_NUM_35,    // RX Pin
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

    {
        // Initialize Detection
        char* D0_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D0_X");
        char* D0_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D0_Y");

        char* D1_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D1_X");
        char* D1_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D1_Y");

        char* D2_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D2_X");
        char* D2_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D2_Y");

        char* D3_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D3_X");
        char* D3_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_D3_Y");

        char* S0_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_S0_X");
        char* S0_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_S0_Y");

        char* S1_X = storage->get_str(SENSOR_BASIC_DATA, "LD2461_S1_X");
        char* S1_Y = storage->get_str(SENSOR_BASIC_DATA, "LD2461_S1_Y");

        bool enter_exit_inverted = storage->get_uint8(SENSOR_BASIC_DATA, "ENTER_EXIT");

        // Test if the values are not NULL
        if(D0_X != NULL && D0_Y != NULL &&
            D1_X != NULL && D1_Y != NULL &&
            D2_X != NULL && D2_Y != NULL &&
            D3_X != NULL && D3_Y != NULL &&
            S0_X != NULL && S0_Y != NULL &&
            S1_X != NULL && S1_Y != NULL)
        {
            ESP_LOGI(TAG, "Using stored values for the detection area");
            if(enter_exit_inverted == 0) enter_exit_inverted = false;
            detection = new Detection(
                {std::stof(D0_X), std::stof(D0_Y)},    // D0
                {std::stof(D1_X), std::stof(D1_Y)},    // D1
                {std::stof(D2_X), std::stof(D2_Y)},    // D2
                {std::stof(D3_X), std::stof(D3_Y)},    // D3
                {std::stof(S0_X), std::stof(S0_Y)},    // S0
                {std::stof(S1_X), std::stof(S1_Y)},    // S1
                enter_exit_inverted
            );
            free(D0_X); free(D0_Y);
            free(D1_X); free(D1_Y);
            free(D2_X); free(D2_Y);
            free(D3_X); free(D3_Y);
        }
        else // If the values are NULL, we will use the default values
        {
            ESP_LOGI(TAG, "Values for the detection area not found, using default values");
            detection = new Detection(
                {-2, 3},    // D0
                {-2, 1.8},  // D1
                {2, 1.8},   // D2
                {2, 3},     // D3
                {-2, 1.8},  // S0
                {2, 1.8}    // S1
            );

            // Save the default values
            detection->set_detection_area(
                {-2, 3},    // D0
                {-2, 1.8},  // D1
                {2, 1.8},   // D2
                {2, 3},     // D3
                {-2, 1.8},  // S0
                {2, 1.8}    // S1
            );

            ESP_LOGI(TAG, "Default values for the detection area stored");
        }
    }
    ESP_LOGI(TAG, "Detection area: (%f, %f), (%f, %f), (%f, %f), (%f, %f)",
        detection->get_detection_area_point()[0].x, detection->get_detection_area_point()[0].y,
        detection->get_detection_area_point()[1].x, detection->get_detection_area_point()[1].y,
        detection->get_detection_area_point()[2].x, detection->get_detection_area_point()[2].y,
        detection->get_detection_area_point()[3].x, detection->get_detection_area_point()[3].y
    );

    // Initialize Variables
    std::string sensor_state;
    detection->start_detection();
    int64_t last_payload_time = esp_timer_get_time();
    int64_t time_now = 0;
    // Main Loop
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    detection->set_raw_data_sent(true);
    mqtt->subscribe(std::string(sensor->get_mqtt_root_topic() + "/command/#").c_str(), 0);
    {
        ESP_LOGI(TAG, "Firmware compiled on [ %s @ %s (UTC -3)]", __DATE__, __TIME__);
        const esp_partition_t *running = esp_ota_get_running_partition();
        ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%lx)",
             running->type, running->subtype, running->address);
    }
    ESP_LOGI(TAG, "Finished initialization");
    sensor->start_free_memory = esp_get_free_heap_size();

    ESP_LOGI(TAG, "Started on [%s (GMT +0)]", sensor->get_current_timestamp().c_str());
    sensor->change_time_zone("GMT +3"); // Change to UTC-3
    while(flag_0)
    {
        //printf("%s\n", sensor->get_current_timestamp().c_str());
        time_now = esp_timer_get_time();
        detection->detect();
        sensor_state = (
            "{"
                "\"internal_temperature\": " + std::to_string(sensor->get_internal_temperature()) + ","
                "\"free_memory\": " + std::to_string(esp_get_free_heap_size()) + ","
                "\"rssi\": " + std::to_string(wifi->get_rssi()) + ","
                "\"uptime\": " + std::to_string(esp_timer_get_time() / 1000000) + ","
                "\"last_boot_reason\": " + std::to_string(esp_reset_reason()) +
            "}"
        );
        if(time_now - last_payload_time > sensor->get_payload_buffer_time())
        {
            detection->mqtt_send_detections();
            mqtt->publish(
                std::string(sensor->get_mqtt_root_topic() + "/info").c_str(),
                sensor_state.c_str()
            );
            last_payload_time = time_now;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}