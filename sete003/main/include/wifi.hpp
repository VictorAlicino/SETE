#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include <string>

#define MAXIMUM_WIFI_RETRY 5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group  = xEventGroupCreate();

/**
 * @brief Get the mac address in cstring format
 * 
 * @param mac_str Mac address string pointer
 */
void get_mac_address_str(char* mac_str);

class WiFi_STA{
private:
public:
    /**
     * @brief Construct a new WiFi_STA object
     * 
     * @param ssid WiFi SSID
     * @param password WiFi Password
     */
    WiFi_STA(std::string ssid, std::string password);

    /**
     * @brief Get WiFi Signal strenght
     * 
     * @return int8_t WiFi signal strenght
     */
    int8_t get_rssi();

    /**
     * @brief Get the ap info object
     * 
     * @return wifi_ap_record_t 
     */
    wifi_ap_record_t get_ap_info();

    /**
     * @brief Check if the WiFi is connected
     * 
     * @return true If the WiFi is connected
     * @return false If the WiFi is not connected
     */
    bool is_connected();

    void shutdown();
};
