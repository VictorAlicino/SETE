#include "radar.hpp"
#include "ld2461.hpp"
#include "cJSON.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "freertos/queue.h"

uint8_t ld2461_generate_checksum(uint8_t command_word, int8_t* command_value, uint8_t data_length){
    uint8_t sum = 0;
    sum += command_word;
    for(uint16_t i=0; i < data_length-1; i++)
    {
        sum += command_value[i];
    }
    return sum;
}

void send_radar_sample(radar_sample sample, uart_port_t uart_num){
    int last_valid = -1;
    if(sample.T0.x != 0.0 || sample.T0.y != 0.0) last_valid = 0;
    if(sample.T1.x != 0.0 || sample.T1.y != 0.0) last_valid = 1;
    if(sample.T2.x != 0.0 || sample.T2.y != 0.0) last_valid = 2;
    if(sample.T3.x != 0.0 || sample.T3.y != 0.0) last_valid = 3;
    if(sample.T4.x != 0.0 || sample.T4.y != 0.0) last_valid = 4;

    int num_samples = last_valid + 1;
    int8_t* command_value = (int8_t*)malloc(num_samples * 2); // 2 bytes por target

    if(num_samples > 0) {
        command_value[0] = (sample.T0.x);
        command_value[1] = (sample.T0.y);
    }
    if(num_samples > 1) {
        command_value[2] = (sample.T1.x);
        command_value[3] = (sample.T1.y);
    }
    if(num_samples > 2) {
        command_value[4] = (sample.T2.x);
        command_value[5] = (sample.T2.y);
    }
    if(num_samples > 3) {
        command_value[6] = (sample.T3.x);
        command_value[7] = (sample.T3.y);
    }
    if(num_samples > 4) {
        command_value[8] = (sample.T4.x);
        command_value[9] = (sample.T4.y);
    }

    uint8_t command_value_size = num_samples * 2;

    if (command_value_size == 0) { 
        free(command_value);
        return;  // Nenhum dado válido para enviar
    }

    uint8_t* payload = (uint8_t*)malloc(command_value_size + 10);

    payload[0] = 0xFF;
    payload[1] = 0xEE;
    payload[2] = 0xDD;
    payload[3] = 0x00;
    payload[4] = command_value_size + 1; // Command Word + Command Value
    payload[5] = LD2461_COMMAND_RADAR_REPORT_1; // Command Word

    memcpy(&payload[6], command_value, command_value_size);

    payload[6 + command_value_size] = ld2461_generate_checksum(LD2461_COMMAND_RADAR_REPORT_1, command_value, command_value_size);
    payload[7 + command_value_size] = 0xDD;
    payload[8 + command_value_size] = 0xEE;
    payload[9 + command_value_size] = 0xFF;

    uart_write_bytes(uart_num, payload, command_value_size + 10);

    free(command_value);
    free(payload);
}
