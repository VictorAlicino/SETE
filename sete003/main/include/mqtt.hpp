#pragma once

#include "mqtt_client.h"

class MQTT{
private:
    esp_mqtt_client_handle_t client;
public:
    MQTT(const char* uri);
    esp_mqtt_client_handle_t get_client();
    esp_err_t publish(const char* topic, const char* payload);
};