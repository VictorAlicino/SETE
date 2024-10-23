#pragma once

#include "driver/gpio.h"

class PIR{
private:
    gpio_num_t pin;
public:
    PIR(gpio_num_t pin);
    int read();
    const char* read_to_json();
};