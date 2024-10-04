#include "mqtt.hpp"

MQTT::MQTT(const char* uri)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address = uri,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(
                            client,
                            ESP_EVENT_ANY_ID,
                            mqtt_event_handler,
                            client);
}