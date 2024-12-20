#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "wifi.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"


#include "lwip/err.h"
#include "lwip/sys.h"

static const char* WIFI_TAG = "WiFi Station";

int s_retry_num = 0;

static void event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
    )
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_WIFI_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Set the dynamic hostname based on the MAC Address
 * 
 * @param netif Network Interface object to be used with the hostname
 */
void set_dynamic_hostname(esp_netif_t* netif)
{
    uint8_t mac[6];
    char hostname[32];

    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Failed to get MAC Address");
        return;
    }

    snprintf(hostname, sizeof(hostname), "SETE-Sonare-%02X%02X", mac[0], mac[5]);

    esp_netif_set_hostname(netif, hostname);

    ESP_LOGI(WIFI_TAG, "Hostname set to: %s", hostname);
}

WiFi_STA::WiFi_STA(std::string ssid, std::string password)
{

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    set_dynamic_hostname(netif);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        this,
        &instance_any_id
        )
    );
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        this,
        &instance_got_ip
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .threshold = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "WiFi Station Mode Enable");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID: %s",
                 ssid.c_str());
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID: %s",
                 ssid.c_str());
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("a.st1.ntp.br");
    esp_netif_sntp_init(&config);

}

int8_t WiFi_STA::get_rssi()
{
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    return ap_info.rssi;
}

wifi_ap_record_t WiFi_STA::get_ap_info()
{
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    return ap_info;
}

bool WiFi_STA::is_connected()
{
    return xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT;
}

void WiFi_STA::shutdown()
{
    ESP_LOGW(WIFI_TAG, "Shutting down WiFi");
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
}