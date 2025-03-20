#pragma once

#include <string>

#include "mqtt_client.h"

void process_server_message(
    std::string topic,
    std::string data
    );