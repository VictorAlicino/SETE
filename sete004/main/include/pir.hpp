#pragma once

#include "driver/gpio.h"

class PIR{
private:
    gpio_num_t pin;
public:
    PIR(gpio_num_t pin);

    /**
     * @brief Read PIR value
     * 
     * @return int 1 if motion detected, 0 otherwise
     */
    int read();

    /**
     * @brief Convert PIR value to JSON in cstring format
     * 
     * @return const char* cstring with JSON format
     */
    const char* read_to_json();
};