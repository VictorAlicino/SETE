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
#include <string>

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "ld2461.hpp"
#include "pir.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"

#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37

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

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // TODO: Load board informations

    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    LD2461 ld2461 = LD2461(
        UART_NUM_2,
        GPIO_NUM_36,
        GPIO_NUM_35,
        9600
    );

    PIR pir = PIR(GPIO_NUM_48);

    WiFi_STA wifi = WiFi_STA();

    MQTT mqtt = MQTT("mqtt://144.22.195.55:1883");

    ld2461_frame_t ld2461_frame = ld2461_setup_frame();

    ld2461_version_t ld2461_version = ld2461.get_version_and_id(&ld2461_frame);

    ESP_LOGI(TAG, "Detected LD2461 running on v%01X.%01X from %d/%d/%d",
                    ld2461_version.major, ld2461_version.minor,
                    ld2461_version.month,
                    ld2461_version.day,
                    ld2461_version.year);

    char* mac_str = (char*)malloc(18);
    get_mac_address_str(mac_str);
    std::string mac_str_s(mac_str);
    mac_str_s = mac_str_s.substr(0, 2) + mac_str_s.substr(mac_str_s.length() - 2, 2);
    free(mac_str);
    
    std::string ld2461_payload = "";
    std::string pir_payload = "";
    std::string payload = "";

    while(true)
    {
        ld2461.read_data(&ld2461_frame);
        ld2461_payload = ld2461.detection_to_json(&ld2461_frame);
        pir_payload = pir.read_to_json();
        payload = ("{\"" + mac_str_s + "\":{\"ld2461\":" + ld2461_payload + ",\"pir\":" + pir_payload + "}}");
        mqtt.publish("SETE/tech_demo", payload.c_str());
        printf("%s", ld2461.frame_to_string(&ld2461_frame));
        printf(" -> ");
        ld2461.report_detections();
        if(pir.read() == 1){
            printf("## PIR: Detected |\n");
        }
        else{
            printf("## PIR: Not Detected |\n");
        }
    }
}
