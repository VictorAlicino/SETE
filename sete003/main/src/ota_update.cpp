#include "ota_update.hpp"
#include "sensor.hpp"

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"

#define HASH_LEN 32

#ifndef RED_LED
#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37
#endif

extern Sensor* sensor;

static const char *OTA_TAG = "Over-The-Air Update";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}


static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(OTA_TAG, "%s %s", label, hash_print);
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}

void ota_led_blink(void *pvParameters)
{
    while (true)
    {
        gpio_set_level(RED_LED, 1);
        gpio_set_level(GREEN_LED, 0);
        gpio_set_level(BLUE_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(RED_LED, 0);
        gpio_set_level(GREEN_LED, 0);
        gpio_set_level(BLUE_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);

        gpio_set_level(RED_LED, 0);
        gpio_set_level(GREEN_LED, 1);
        gpio_set_level(BLUE_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(RED_LED, 0);
        gpio_set_level(GREEN_LED, 0);
        gpio_set_level(BLUE_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);

        gpio_set_level(RED_LED, 0);
        gpio_set_level(GREEN_LED, 0);
        gpio_set_level(BLUE_LED, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(RED_LED, 0);
        gpio_set_level(GREEN_LED, 0);
        gpio_set_level(BLUE_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void ota_update(std::string uri)
{
    ESP_LOGI(OTA_TAG, "Starting OTA task");

    esp_http_client_config_t config = {
        .url = uri.c_str(),
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .event_handler = _http_event_handler,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(OTA_TAG, "Attempting to download update from %s", config.url);
    TaskHandle_t led_blink_task = NULL;
    xTaskCreate(ota_led_blink, "ota_led_blink", 2048, NULL, 5, &led_blink_task);

    esp_err_t ret = esp_https_ota(&ota_config);
    vTaskDelete(led_blink_task);

    if (ret == ESP_OK) {
        ESP_LOGI(OTA_TAG, "OTA Succeed, Rebooting...");
        for(int i = 5; i > 0; i--)
        {
            ESP_LOGI(OTA_TAG, "Rebooting in %d seconds...", i);
            gpio_set_level(RED_LED, 0);
            gpio_set_level(GREEN_LED, 1);
            gpio_set_level(BLUE_LED, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(RED_LED, 0);
            gpio_set_level(GREEN_LED, 0);
            gpio_set_level(BLUE_LED, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        esp_restart();
        ESP_LOGE(OTA_TAG, "Reboot failed");
        ESP_LOGW(OTA_TAG, "Please reboot manually");
        return;
    } else {
        ESP_LOGE(OTA_TAG, "Firmware upgrade failed");
        return;
    }
}