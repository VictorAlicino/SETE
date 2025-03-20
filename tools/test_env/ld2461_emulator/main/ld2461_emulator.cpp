#include <stdio.h>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <vector>
#include <iostream>
#include <queue>
#include <esp_log.h>

#include "include/wifi.hpp"
#include "include/mqtt.hpp"
#include "include/sensor.hpp"
#include "include/storage.hpp"
#include "include/radar.hpp"

#define UART_NUM UART_NUM_2

WiFi_STA* wifi;
MQTT* mqtt;
Sensor* sensor;
Storage* storage;

const char* TAG = "EMULATOR";

extern std::queue<radar_sample> radar_queue;

extern bool mqtt_connected;

extern "C" 
{
    void app_main(void);
}

void app_main(void)
{
    storage = new Storage();
    // Conectar no WiFi
    wifi = new WiFi_STA("50 centavos a hora", "duzentoseoito");
    sensor = new Sensor();

    vTaskDelay(1000/portTICK_PERIOD_MS);

    // Conectar no MQTT Broker
    mqtt = new MQTT("mqtt://144.22.195.55:1883");

    // Abrir porta UART
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    esp_err_t err = uart_param_config(UART_NUM, &uart_config);
    ESP_ERROR_CHECK(err);
    err = uart_set_pin(UART_NUM, 35, 36, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(err);

    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    err = uart_driver_install(
        UART_NUM,           // UART Num
        uart_buffer_size,   // RX Buffer Size
        0,                  // TX Buffer Size
        10,                 // Queue Size
        &uart_queue,        // Pointer to Queue
        0                   // Flags to Interruptions
    );
    ESP_ERROR_CHECK(err);


    // Initialize MQTT
    vTaskDelay(2000/portTICK_PERIOD_MS);
    mqtt->subscribe(std::string(sensor->get_mqtt_root_topic() + "/data/#").c_str(), 0);

    // Receber confirmação do Sensor003
    ESP_LOGI(TAG,"Waiting for MCU confirmation...");
    bool mcu_init = false;
    while(!mcu_init){
        uint8_t data;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0xFF) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0xEE) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0xDD) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0x00) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0x02) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0x09) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0x01) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        if (data != 0x0A) continue;
        uart_read_bytes(UART_NUM, &data, 1, 100);
        uart_read_bytes(UART_NUM, &data, 1, 100);
        uart_read_bytes(UART_NUM, &data, 1, 100);
        mcu_init = true;
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    uint8_t return_data[18] = {
        0xFF, 0xEE, 0xDD, 
        0x00, 0x09, 
        0x09,
        0x53, 0x10, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00,
        0xDD, 0xEE, 0xFF 
    };
    ESP_LOGI(TAG,"MCU Found!");

    uart_write_bytes(UART_NUM, (const char*)return_data, 18);
    ESP_LOGI(TAG,"MCU Confirmed!");
    //while(true){ uart_write_bytes(UART_NUM, (const char*)return_data, 18); vTaskDelay(100/portTICK_PERIOD_MS);}

    uint8_t idle_data[10] = {
        0xFF, 0xEE, 0xDD, // Header
        0x00, 0x01, // Length
        0x07, // Command Word
        0x07, // Checksum
        0xDD, 0xEE, 0xFF // Footer
    };

    while (true) {
        if (!radar_queue.empty()) {
            send_radar_sample(radar_queue.front(), UART_NUM);
            radar_queue.pop();
            
            // Aguarde a transmissão ser concluída antes de prosseguir
            uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(50));
        } 
        else {
            uart_write_bytes(UART_NUM, (const char*)idle_data, 10);
            
            // Aguarde a transmissão ser concluída antes de prosseguir
            uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(50));
        }
    
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Reduzi para 50ms para evitar delays excessivos
    }
}
