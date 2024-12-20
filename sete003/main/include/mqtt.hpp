#pragma once

#include "mqtt_client.h"

class MQTT{
private:
    esp_mqtt_client_handle_t client;
public:
    /**
     * @brief Construct a new MQTT object
     * @note URI format: mqtt://username:password@host:port
     * 
     * @param uri URI to connect to the MQTT Broker
     */
    MQTT(const char* uri);

    /**
     * @brief Get the client object
     * 
     * @return esp_mqtt_client_handle_t 
     */
    esp_mqtt_client_handle_t get_client();

    /**
     * @brief Publish a message to a topic
     * 
     * @param topic Topic to publish
     * @param payload Payload to publish
     * @return esp_err_t ESP_OK if success
     */
    esp_err_t publish(const char* topic, const char* payload);

    /**
     * @brief Subscribe to a topic
     * 
     * @param topic Topic to subscribe
     * @param qos Quality of Service
     * @return esp_err_t ESP_OK if success
     */
    esp_err_t subscribe(const char* topic, int qos);

    void shutdown();
};