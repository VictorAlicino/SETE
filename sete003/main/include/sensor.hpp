#pragma once

#include <string>

#include "driver/temperature_sensor.h"

typedef struct coordinate{
    float x;
    float y;
}coordinate_t;

typedef struct detection_area{
    coordinate_t top_left;
    coordinate_t bottom_right;
}detection_area_t;

class Sensor{
private:
    std::string name;
    std::string designator;
    std::string mqtt_root_topic;

    temperature_sensor_handle_t temperature_sensor;

    int entered_detections = 0;
    int exited_detections = 0;

    detection_area_t detection_area;
public:
    Sensor();
    std::string get_name();
    std::string get_designator();
    std::string get_mqtt_root_topic();

    void transfer_log_to_mqtt();
    void rollback_log_to_uart();
    float get_internal_temperature();
};