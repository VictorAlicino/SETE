#include "mqtt.hpp"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "string.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sensor.cpp"
#include "comms.hpp"

const char* MQTT_TAG = "MQTT";

bool mqtt_connected = false;

extern Sensor* sensor;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;
        sensor->transfer_log_to_mqtt();
        gpio_set_level(GPIO_NUM_37, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = true;
        sensor->rollback_log_to_uart();
        gpio_set_level(GPIO_NUM_37, 0);
        break;

    case 7:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        {
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
            //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            std::string root_topic = sensor->get_mqtt_root_topic() + "/command";
            std::string topic = std::string(event->topic, event->topic_len);
            if(topic.substr(0, root_topic.length()) == root_topic)
            {
                process_server_message(
                    topic.substr(root_topic.length(), topic.length()),
                    std::string(event->data, event->data_len)
                );
            }
            break;
        }
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}

MQTT::MQTT(const char* uri)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
    this->client = esp_mqtt_client_init(&mqtt_cfg);
    if(this->client == NULL)
    {
        ESP_LOGE(MQTT_TAG, "Failed to initialize MQTT");
        return;
    }
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(
        this->client,
        (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
        );
    esp_mqtt_client_start(this->client);
}

esp_err_t MQTT::publish(const char* topic, const char* payload)
{
    int a = esp_mqtt_client_publish(
        this->client,
        topic,
        payload,
        0,
        0,
        0);
    if(a<0) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t MQTT::subscribe(const char* topic, int qos)
{
    int a = esp_mqtt_client_subscribe(
        this->client,
        topic,
        qos
        );
    if(a<0) return ESP_FAIL;
    ESP_LOGD(MQTT_TAG, "Subscribed to %s", topic);
    return ESP_OK;
}

esp_mqtt_client_handle_t MQTT::get_client(){return this->client;}
