#include "pir.hpp"
#include <string.h>

PIR::PIR(gpio_num_t pin){
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    this->pin = pin;

    esp_err_t err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);
}

int PIR::read(){
    return gpio_get_level(this->pin);
}

const char* PIR::read_to_json(){
    static char json[64];
    memset(json, 0, sizeof(json));
    sprintf(json, "{\"detection\":%d}", gpio_get_level(this->pin));
    return json;   
}