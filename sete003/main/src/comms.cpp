#include "comms.hpp"
#include "detection.hpp"
#include "mqtt.hpp"
#include "wifi.hpp"
#include "ota_update.hpp"

#include "esp_log.h"
#include "cJSON.h"

#include <string>

extern Detection* detection;
extern MQTT* mqtt;
extern WiFi_STA* wifi;
extern Sensor* sensor;

const char* COMMS_TAG = "COMMS";

void process_server_message(
    std::string topic,
    std::string data
)
{
    if(topic == "/reset")
    {
        ESP_LOGI("COMMS", "Resetting device by Server command");
        esp_restart();
    }
    else if(topic == "/update")
    {
        ESP_LOGI("COMMS", "Updating device by Server command");
        
        cJSON* root = cJSON_Parse(data.c_str());
        if(root == NULL)
        {
            ESP_LOGE(COMMS_TAG, "Invalid JSON");
            return;
        }

        cJSON* url = cJSON_GetObjectItem(root, "url");
        if(url == NULL)
        {
            ESP_LOGE(COMMS_TAG, "Invalid URL");
            return;
        }
        ota_update(url->valuestring);
    }
    else if(topic == "/detection_area/set")
    {
        ESP_LOGI("COMMS", "Setting detection area from Server command");
        cJSON* root = cJSON_Parse(data.c_str());
        if(root == NULL)
        {
            ESP_LOGE(COMMS_TAG, "Invalid JSON");
            return;
        }

        cJSON* new_D0 = cJSON_GetObjectItem(root, "D0");
        cJSON* new_D1 = cJSON_GetObjectItem(root, "D1");
        cJSON* new_D2 = cJSON_GetObjectItem(root, "D2");
        cJSON* new_D3 = cJSON_GetObjectItem(root, "D3");
        cJSON* new_S0 = cJSON_GetObjectItem(root, "S0");
        cJSON* new_S1 = cJSON_GetObjectItem(root, "S1");

        float D0_x = cJSON_GetObjectItem(new_D0, "x")->valuedouble;
        float D0_y = cJSON_GetObjectItem(new_D0, "y")->valuedouble;

        float D1_x = cJSON_GetObjectItem(new_D1, "x")->valuedouble;
        float D1_y = cJSON_GetObjectItem(new_D1, "y")->valuedouble;

        float D2_x = cJSON_GetObjectItem(new_D2, "x")->valuedouble;
        float D2_y = cJSON_GetObjectItem(new_D2, "y")->valuedouble;

        float D3_x = cJSON_GetObjectItem(new_D3, "x")->valuedouble;
        float D3_y = cJSON_GetObjectItem(new_D3, "y")->valuedouble;

        float S0_x = cJSON_GetObjectItem(new_S0, "x")->valuedouble;
        float S0_y = cJSON_GetObjectItem(new_S0, "y")->valuedouble;

        float S1_x = cJSON_GetObjectItem(new_S1, "x")->valuedouble;
        float S1_y = cJSON_GetObjectItem(new_S1, "y")->valuedouble;

        detection->set_detection_area(
            {D0_x, D0_y},
            {D1_x, D1_y},
            {D2_x, D2_y},
            {D3_x, D3_y},
            {S0_x, S0_y},
            {S1_x, S1_y}
        );

        cJSON_Delete(root);
    }
    else if(topic == "/detection_area/get")
    {
        ESP_LOGI("COMMS", "Sending detection area to callback topic by Server command");
        cJSON* root = cJSON_CreateObject();
        cJSON* D0 = cJSON_CreateObject();
        cJSON* D1 = cJSON_CreateObject();
        cJSON* D2 = cJSON_CreateObject();
        cJSON* D3 = cJSON_CreateObject();
        cJSON* S0 = cJSON_CreateObject();
        cJSON* S1 = cJSON_CreateObject();

        detection_area_t detection_area = detection->get_detection_area();

        cJSON_AddItemToObject(D0, "x", cJSON_CreateNumber(detection_area.D[0].x));
        cJSON_AddItemToObject(D0, "y", cJSON_CreateNumber(detection_area.D[0].y));

        cJSON_AddItemToObject(D1, "x", cJSON_CreateNumber(detection_area.D[1].x));
        cJSON_AddItemToObject(D1, "y", cJSON_CreateNumber(detection_area.D[1].y));

        cJSON_AddItemToObject(D2, "x", cJSON_CreateNumber(detection_area.D[2].x));
        cJSON_AddItemToObject(D2, "y", cJSON_CreateNumber(detection_area.D[2].y));

        cJSON_AddItemToObject(D3, "x", cJSON_CreateNumber(detection_area.D[3].x));
        cJSON_AddItemToObject(D3, "y", cJSON_CreateNumber(detection_area.D[3].y));

        cJSON_AddItemToObject(S0, "x", cJSON_CreateNumber(detection_area.S[0].x));
        cJSON_AddItemToObject(S0, "y", cJSON_CreateNumber(detection_area.S[0].y));

        cJSON_AddItemToObject(S1, "x", cJSON_CreateNumber(detection_area.S[1].x));
        cJSON_AddItemToObject(S1, "y", cJSON_CreateNumber(detection_area.S[1].y));

        cJSON_AddItemToObject(root, "D0", D0);
        cJSON_AddItemToObject(root, "D1", D1);
        cJSON_AddItemToObject(root, "D2", D2);
        cJSON_AddItemToObject(root, "D3", D3);
        cJSON_AddItemToObject(root, "S0", S0);
        cJSON_AddItemToObject(root, "S1", S1);

        char* data = cJSON_Print(root);
        mqtt->publish(
            sensor->get_mqtt_callback_topic().c_str(),
            data
        );

        cJSON_Delete(root);
    }
    else if(topic == "/detection_area/invert")
    {
        if(data == "true")
        {
            detection->set_enter_exit_inverted(true);
        }
        else if(data == "false")
        {
            detection->set_enter_exit_inverted(false);
        }
        else
        {
            ESP_LOGW(COMMS_TAG, "Invalid data for invert");
        }
    }
    else if(topic == "/raw_data")
    {
        if(data == "true")
        {
            ESP_LOGI(COMMS_TAG, "Transmitting raw data by Server command");
            detection->set_raw_data_sent(true);
        }
        else if(data == "false")
        {
            ESP_LOGI(COMMS_TAG, "Raw data transmission stopped by Server command");
            detection->set_raw_data_sent(false);
        }
        else
        {
            ESP_LOGW(COMMS_TAG, "Invalid data for raw_data");
        }
    }
    else if(topic == "/payload_buffer_time/set")
    {
        ESP_LOGI(COMMS_TAG, "Setting payload buffer time by Server command");
        int64_t buffer_time = std::stoll(data);
        sensor->set_payload_buffer_time(buffer_time);
    }
    else if(topic == "/payload_buffer_time/get")
    {
        ESP_LOGI(COMMS_TAG, "Sending payload buffer time to callback topic by Server command");
        cJSON* root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "buffer_time", cJSON_CreateNumber(sensor->get_payload_buffer_time()));
        char* data = cJSON_Print(root);
        mqtt->publish(
            sensor->get_mqtt_callback_topic().c_str(),
            data
        );
        cJSON_Delete(root);
    }
    else
    {
        ESP_LOGE(COMMS_TAG, "Invalid command");
    }
}