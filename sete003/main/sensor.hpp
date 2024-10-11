#pragma once

#include <string>

class Sensor{
private:
    std::string name;
    std::string designator;
    std::string mqtt_root_topic;
public:
    Sensor();
    std::string get_name();
    std::string get_designator();
    std::string get_mqtt_root_topic();

    void transfer_log_to_mqtt();
    void rollback_log_to_uart();
};