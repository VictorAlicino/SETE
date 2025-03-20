#define DEBUG 0

#ifdef DEBUG
#if DEBUG == 1
#pragma message("DEBUG MODE ENABLED")
#endif
#endif

#include "sensor.hpp"
#include "mqtt.hpp"
#include "wifi.hpp"
#include "storage.hpp"

#include "esp_log.h"
#include "esp_wifi.h"
#include "time.h"
#include "freertos/timers.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

const char* SENSOR_TAG = "Sensor";

extern Storage* storage;
extern MQTT* mqtt;
extern Sensor* sensor;
extern WiFi_STA* wifi;

std::string log_topic = "";

vprintf_like_t original_log_function; // Keeps the original log function to rollback

/**
 * @brief Send the log buffer to MQTT and UART
 * 
 * @param fmt 
 * @param args 
 * @return int Original log function
 */
int mqtt_and_uart_log_vprintf(const char *fmt, va_list args) {
    static char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (len >= 0 && len < sizeof(buffer)) {
        //printf("%s", buffer);
        esp_mqtt_client_publish(
            mqtt->get_client(),
            std::string(sensor->get_mqtt_root_topic() + "/log").c_str(),
            buffer, len-1 /*Ignore \n*/, 1, 0
            );

        if (original_log_function) {
            original_log_function(fmt, args);
        }
    }
    return len;
}

/**
 * @brief Get the MAC address of the ESP32
 * 
 * @param mac_str 
 */
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

Sensor::Sensor()
{
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
    #if DEBUG == 1
    this->mqtt_root_topic = "SETE/sensors/debug/sete004/" + mac_str_s;
    #else
    this->mqtt_root_topic = "SETE/sensors/sete004/" + mac_str_s;
    #endif
    this->mqtt_callback_topic = this->mqtt_root_topic + "/callback";

    int64_t nvs_buffer_time = storage->get_int64(SENSOR_BASIC_DATA, "BUFFER_TIME");
    if(nvs_buffer_time == NULL)
    {
        ESP_LOGI(SENSOR_TAG, "BUFFER_TIME not found in NVS, setting default value");
        this->payload_buffer_time = 10000000; // 30 seconds
        storage->store_data_int64(SENSOR_BASIC_DATA, "BUFFER_TIME", this->payload_buffer_time);
    }
    else
    {
        this->payload_buffer_time = nvs_buffer_time;
    }

    free(mac_str);
    ESP_LOGI(SENSOR_TAG, "Payload will be buffered for %lld microseconds", this->payload_buffer_time);
    ESP_LOGI(SENSOR_TAG, "%s initialized with designator %s", this->name.c_str(), this->designator.c_str());
}

std::string Sensor::get_name(){return this->name;}
std::string Sensor::get_designator(){return this->designator;}
std::string Sensor::get_mqtt_root_topic(){return this->mqtt_root_topic;}
std::string Sensor::get_mqtt_callback_topic(){return this->mqtt_callback_topic;}
int64_t Sensor::get_payload_buffer_time(){return this->payload_buffer_time;}

void Sensor::set_payload_buffer_time(int64_t buffer_time)
{
    this->payload_buffer_time = buffer_time;
    storage->store_data_int64(SENSOR_BASIC_DATA, "BUFFER_TIME", this->payload_buffer_time);
    ESP_LOGI(SENSOR_TAG, "BUFFER_TIME set to %lld", this->payload_buffer_time);
}

void Sensor::transfer_log_to_mqtt()
{
    log_topic = this->mqtt_root_topic + "/log";
    original_log_function = esp_log_set_vprintf(mqtt_and_uart_log_vprintf);
    dump_info();
    ESP_LOGW(SENSOR_TAG, "Sending all logs to MQTT from now on");
}

void Sensor::rollback_log_to_uart()
{
    esp_log_set_vprintf(original_log_function);
    ESP_LOGE(SENSOR_TAG, "MQTT not available, logs rollbacked to UART");
}

float Sensor::get_internal_temperature()
{
    float temperature;
    ESP_ERROR_CHECK(temperature_sensor_enable(this->temperature_sensor));
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(this->temperature_sensor, &temperature));
    ESP_ERROR_CHECK(temperature_sensor_disable(this->temperature_sensor));
    return temperature;
}

std::string Sensor::get_ota_update_uri()
{
    return this->ota_update_uri;
}

void Sensor::set_ota_update_uri(std::string uri)
{
    this->ota_update_uri = uri;
}

char* Sensor::time_now()
{
    time_t now;
    static char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    return strftime_buf;
}

void Sensor::change_time_zone(const char* time_zone)
{
    setenv("TZ", time_zone, 1);
    tzset();
}

std::string Sensor::get_current_timestamp()
{
    // Obtém o tempo atual
    auto now = std::chrono::system_clock::now();
    
    // Converte para tempo do sistema
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto timeinfo = std::gmtime(&time_t_now); // Tempo em UTC (GMT)
    
    // Extrai os milissegundos/microsegundos
    auto duration = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

    // Formata a data e hora
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S"); // Data e hora
    oss << "." << std::setfill('0') << std::setw(6) << micros; // Microssegundos
    oss << "+00"; // UTC offset
    
    return oss.str();
}

void Sensor::dump_info()
{
    ESP_LOGI(SENSOR_TAG, "Sensor Name: %s", this->name.c_str());
    ESP_LOGI(SENSOR_TAG, "Sensor Designator: %s", this->designator.c_str());
    ESP_LOGI(SENSOR_TAG, "Sensor MQTT Root Topic: %s", this->mqtt_root_topic.c_str());
    ESP_LOGI(SENSOR_TAG, "Internal Temperature: %.2fºC", this->get_internal_temperature());
    std::string last_boot_reason = "";
    switch(esp_reset_reason())
    {
        case ESP_RST_UNKNOWN:    last_boot_reason = "Reset reason can not be determined"; break;
        case ESP_RST_POWERON:    last_boot_reason = "Reset due to power-on event"; break;
        case ESP_RST_EXT:        last_boot_reason = "Reset by external pin (not applicable for ESP32)"; break;
        case ESP_RST_SW:         last_boot_reason = "Software reset via esp_restart"; break;
        case ESP_RST_PANIC:      last_boot_reason = "Software reset due to exception/panic"; break;
        case ESP_RST_INT_WDT:    last_boot_reason = "Reset (software or hardware) due to interrupt watchdog"; break;
        case ESP_RST_TASK_WDT:   last_boot_reason = "Reset due to task watchdog"; break;
        case ESP_RST_WDT:        last_boot_reason = "Reset due to other watchdogs"; break;
        case ESP_RST_DEEPSLEEP:  last_boot_reason = "Reset after exiting deep sleep mode"; break;
        case ESP_RST_BROWNOUT:   last_boot_reason = "Brownout reset (software or hardware)"; break;
        case ESP_RST_SDIO:       last_boot_reason = "Reset over SDIO"; break;
        case ESP_RST_USB:        last_boot_reason = "Reset by USB peripheral"; break;
        case ESP_RST_JTAG:       last_boot_reason = "Reset by JTAG"; break;
        case ESP_RST_EFUSE:      last_boot_reason = "Reset due to efuse error"; break;
        case ESP_RST_PWR_GLITCH: last_boot_reason = "Reset due to power glitch detected"; break;
        case ESP_RST_CPU_LOCKUP: last_boot_reason = "Reset due to CPU lock up"; break;
        default:                 last_boot_reason = "Unhandled Reset Reason"; break;
    }
    ESP_LOGI(SENSOR_TAG, "Last Boot Reason: %s", last_boot_reason.c_str());
    ESP_LOGI(SENSOR_TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(SENSOR_TAG, "IDF version: %s", esp_get_idf_version());
    ESP_LOGI(SENSOR_TAG, "WiFi Connected: %s", wifi->is_connected() ? "Yes" : "No");
    if(wifi->is_connected())
    {
        std::string pass_censor = "";
        wifi_ap_record_t ap_info = wifi->get_ap_info();
        for(int i=0; i<strlen((char*)ap_info.ssid); i++)
        {
            pass_censor += "*";
        }
        ESP_LOGI(SENSOR_TAG, "Connected to %s", (char*)ap_info.ssid);
        ESP_LOGI(SENSOR_TAG, "Password: %s", pass_censor.c_str());
        ESP_LOGI(SENSOR_TAG, "RSSI: %d", wifi->get_rssi());
        ESP_LOGI(SENSOR_TAG, "Channel: %d", ap_info.primary);
        ESP_LOGI(SENSOR_TAG, "AP BSSID: %02X:%02X:%02X:%02X:%02X:%02X", ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2], ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    }

}

int64_t Sensor::get_free_memory()
{
    return this->start_free_memory - esp_get_free_heap_size();
}

void Sensor::shutdown()
{
    ESP_LOGE(SENSOR_TAG, "Shutting down Sensor");
    mqtt->shutdown();
    wifi->shutdown();
}