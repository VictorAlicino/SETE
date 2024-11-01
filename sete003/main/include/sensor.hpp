#pragma once

#include <string>

#include "driver/temperature_sensor.h"

class Sensor{
private:
    std::string name;
    std::string designator;
    std::string mqtt_root_topic;

    temperature_sensor_handle_t temperature_sensor;
public:
    Sensor();
    /**
     * @brief Get Sensor Full name
     * 
     * @return std::string Sensor Name
     */
    std::string get_name();

    /**
     * @brief Get Sensor designator
     * @note Designator is a short name for the sensor composed by the first and last byte of the MAC address
     * @return std::string Sensor Designator
     */
    std::string get_designator();

    /**
     * @brief Get the mqtt root topic
     * 
     * @return std::string mqtt root topic
     */
    std::string get_mqtt_root_topic();

    /**
     * @brief Transfers the log from STDIO to MQTT
     * 
     */
    void transfer_log_to_mqtt();

    /**
     * @brief Rollback the log function for what it was before
     * 
     */
    void rollback_log_to_uart();

    /**
     * @brief Get the internal temperature of the ESP32
     * 
     * @return float Internal Temperature
     */
    float get_internal_temperature();

    void dump_info();
};