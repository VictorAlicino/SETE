#pragma once

#include "mqtt_client.h"

class MQTT{
private:
    esp_mqtt_client_handle_t client;
public:
    MQTT(const char* uri);
};