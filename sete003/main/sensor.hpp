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
    std::string get_name();
    std::string get_designator();
    std::string get_mqtt_root_topic();

    void transfer_log_to_mqtt();
    void rollback_log_to_uart();
    float get_internal_temperature();
};