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
#include "include/radar.hpp"

#include <queue>

const char* MQTT_TAG = "MQTT";

bool mqtt_connected = false;
std::queue<radar_sample> radar_queue;

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
        //sensor->transfer_log_to_mqtt();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = true;
        sensor->rollback_log_to_uart();
        break;
    case 7:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        {
            // Verifica se o tópico é o esperado
            std::string root_topic_str = sensor->get_mqtt_root_topic();
            const char *root_topic = root_topic_str.c_str();
            size_t root_topic_len = root_topic_str.length();
            if (event->topic_len == root_topic_len + 5 && 
                strncmp(event->topic, root_topic, root_topic_len) == 0 && 
                strncmp(event->topic + root_topic_len, "/data", 5) == 0) 
            {
                // Copia os dados recebidos para um buffer seguro
                char data_buf[256];  // Ajuste conforme necessário
                size_t data_len = (event->data_len < sizeof(data_buf) - 1) ? event->data_len : (sizeof(data_buf) - 1);
                memcpy(data_buf, event->data, data_len);
                data_buf[data_len] = '\0'; // Garante terminação correta

                // Variáveis para armazenar os valores extraídos
                float T0_X, T0_Y, T1_X, T1_Y, T2_X, T2_Y, T3_X, T3_Y, T4_X, T4_Y;

                // Processa os dados JSON com sscanf
                int parsed = sscanf(data_buf, 
                    "{\"t_0\": {\"x\": %f,\"y\": %f},\"t_1\": {\"x\": %f,\"y\": %f},"
                    "\"t_2\": {\"x\": %f,\"y\": %f},\"t_3\": {\"x\": %f,\"y\": %f},"
                    "\"t_4\": {\"x\": %f,\"y\": %f}}",
                    &T0_X, &T0_Y, &T1_X, &T1_Y, &T2_X, &T2_Y, &T3_X, &T3_Y, &T4_X, &T4_Y
                );

                // Verifica se todos os valores foram extraídos corretamente
                if (parsed == 10) {
                    radar_sample sample = {
                        .T0 = {(int8_t)(T0_X * 10), (int8_t)(T0_Y * 10)},
                        .T1 = {(int8_t)(T1_X * 10), (int8_t)(T1_Y * 10)},
                        .T2 = {(int8_t)(T2_X * 10), (int8_t)(T2_Y * 10)},
                        .T3 = {(int8_t)(T3_X * 10), (int8_t)(T3_Y * 10)},
                        .T4 = {(int8_t)(T4_X * 10), (int8_t)(T4_Y * 10)}
                    };
                    radar_queue.push(sample);
                } else {
                    ESP_LOGW(MQTT_TAG, "MQTT JSON parse failed. Received: %s", data_buf);
                }
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

void MQTT::shutdown()
{
    ESP_LOGW(MQTT_TAG, "Shutting down MQTT");
    esp_mqtt_client_stop(this->client);
    esp_mqtt_client_destroy(this->client);
}