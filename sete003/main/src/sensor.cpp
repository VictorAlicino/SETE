#include "sensor.hpp"
#include "ld2461.hpp"
#include "mqtt.hpp"

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/timers.h"
#include <string>

const char* SENSOR_TAG = "Sensor";

extern MQTT* mqtt;
extern Sensor* sensor;
extern LD2461* ld2461;

std::string log_topic = "";

// Variável para armazenar a função de log original
vprintf_like_t original_log_function;

// Função customizada de log que envia para MQTT e UART
int mqtt_and_uart_log_vprintf(const char *fmt, va_list args) {
    static char buffer[256];  // buffer para armazenar o log formatado
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (len >= 0 && len < sizeof(buffer)) {
        // Publicar a mensagem de log no tópico MQTT
        esp_mqtt_client_publish(mqtt->get_client(), "log/esp32", buffer, 0, 1, 0);
        printf("%s", buffer);

        // Chamar a função de log original para enviar à UART0
        if (original_log_function) {
            original_log_function(fmt, args);
        }
    }
    return len;
}

void get_mac_address_str(char* mac_str)
{
    uint8_t mac[6];
    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (err != ESP_OK) {
        ESP_LOGE(SENSOR_TAG, "Failed to get MAC Address");
        return;
    }
    snprintf(mac_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

Sensor::Sensor(){
    // Init internal temperature sensor
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &this->temperature_sensor));

    // Init Sensor name and designator
    char* mac_str = (char*)malloc(18);
    get_mac_address_str(mac_str);
    std::string mac_str_s(mac_str);
    mac_str_s = mac_str_s.substr(0, 2) + mac_str_s.substr(mac_str_s.length() - 2, 2);
    
    this->name = "Sonare " + mac_str_s;
    this->designator = mac_str_s;
    this->mqtt_root_topic = "SETE/sensors/sete003/" + mac_str_s;

    free(mac_str);
    ESP_LOGI(SENSOR_TAG, "%s initialized with designator %s", this->name.c_str(), this->designator.c_str());
}

std::string Sensor::get_name(){return this->name;}
std::string Sensor::get_designator(){return this->designator;}
std::string Sensor::get_mqtt_root_topic(){return this->mqtt_root_topic;}

void Sensor::transfer_log_to_mqtt(){
    log_topic = this->mqtt_root_topic + "/log";
    original_log_function = esp_log_set_vprintf(mqtt_and_uart_log_vprintf);
    ESP_LOGW(SENSOR_TAG, "Log transfered to MQTT and UART");
}

void Sensor::rollback_log_to_uart(){
    esp_log_set_vprintf(original_log_function);
    ESP_LOGW(SENSOR_TAG, "Log rollbacked to UART");
}

float Sensor::get_internal_temperature(){
    float temperature;
    ESP_ERROR_CHECK(temperature_sensor_enable(this->temperature_sensor));
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(this->temperature_sensor, &temperature));
    ESP_ERROR_CHECK(temperature_sensor_disable(this->temperature_sensor));
    return temperature;
}
