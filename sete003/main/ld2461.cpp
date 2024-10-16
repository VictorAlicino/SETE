#include <stdio.h>
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>
#include "ld2461.hpp"

#define RED_LED GPIO_NUM_45
#define GREEN_LED GPIO_NUM_38
#define BLUE_LED GPIO_NUM_37

const char* RADAR_TAG = "LD2461";

ld2461_frame_t ld2461_setup_frame()
{
    ld2461_frame_t frame = {
        .data_length = NULL,
        .command_word = LD2461_COMMAND_NULL,
        .command_value = NULL,
        .checksum = 0,
    };
    return frame;
}

ld2461_detection_t ld2461_setup_detection()
{
    ld2461_detection_t detection;
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        detection.target[i].x = 0;
        detection.target[i].y = 0;
    }
    return detection;
}

LD2461::LD2461(
    uart_port_t uart_num,
    gpio_num_t tx_pin,
    gpio_num_t rx_pin,
    int baudrate
){
    esp_err_t err = ESP_OK;

    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    err = uart_param_config(uart_num, &uart_config);
    ESP_ERROR_CHECK(err);

    err = uart_set_pin(
        uart_num,           // UART Num
        tx_pin,             // TX
        rx_pin,             // RX
        UART_PIN_NO_CHANGE, // RTS
        UART_PIN_NO_CHANGE  // CTS
    );
    ESP_ERROR_CHECK(err);

    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    err = uart_driver_install(
        uart_num,           // UART Num
        uart_buffer_size,   // RX Buffer Size
        0,                  // TX Buffer Size
        10,                 // Queue Size
        &uart_queue,        // Pointer to Queue
        0                   // Flags to Interruptions
    );
    ESP_ERROR_CHECK(err);

    this->uart_num = uart_num;
}

void LD2461::read_data(ld2461_frame_t* frame)
{
    bool data_ready = false;
    uint8_t* data_length = (uint8_t*)malloc(2);
    uint8_t* command_word = (uint8_t*)malloc(1);
    uint8_t* checksum = (uint8_t*)malloc(1);
    uint8_t* data = (uint8_t*)malloc(1);
    int bytes_available = 0;
    uint8_t retries_num = 0x0;

    while(!data_ready)
    {
        retries_num++;
        if(retries_num > 10){ESP_LOGW(RADAR_TAG, "Be advised that the UART is not in sync...");}
        if(retries_num > 20){ESP_LOGE(RADAR_TAG, "UART not in sync... Rebooting..."); esp_restart();}
        // Catch the Header
        bytes_available = uart_read_bytes(this->uart_num, data, 1, 100);
        if(bytes_available <= 0){
            gpio_set_level(RED_LED, 0);
            ESP_LOGE(RADAR_TAG, "No data available");
            continue;
        }
        if(*data != 0xFF) {continue;}
        uart_read_bytes(this->uart_num, data, 1, 100);
        if(*data != 0xEE) {continue;}
        uart_read_bytes(this->uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}

        // Data Length - Command Word (1Byte) + Command Value
        uart_read_bytes(this->uart_num, data_length, 2, 100);
        *data_length = (data_length[0] << 8) | data_length[1];

        // Command Word
        uart_read_bytes(this->uart_num, command_word, 1, 100);

        // Command Value
        uint8_t* command_value = (uint8_t*)malloc(*data_length-1);
        for(int i=0; i<(*data_length-1); i++)
        {
            uart_read_bytes(this->uart_num, command_value+i, 1, 100);
        }

        // Checksum
        uart_read_bytes(this->uart_num, checksum, 1, 100);

        // Catch the footer
        uart_read_bytes(this->uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}
        uart_read_bytes(this->uart_num, data, 1, 100);
        if(*data != 0xEE) {printf("\n");continue;}
        uart_read_bytes(this->uart_num, data, 1, 100);
        frame->data_length = *data_length;
        frame->command_word = (ld2461_command_word_t)(*command_word);
        frame->command_value = (uint8_t*)malloc(*data_length-1);
        for(int i=0; i<(*data_length-1); i++)
        {
            frame->command_value[i] = command_value[i];
        }
        frame->checksum = *checksum;
        free(command_value);
       data_ready = true;
       gpio_set_level(RED_LED, 1);
    }
    free(data_length);
    free(command_word);
    free(checksum);
    free(data);
}

uint8_t LD2461::ld2461_generate_checksum(ld2461_frame_t* frame)
{
    uint8_t sum = 0;
    if (frame == NULL) throw "Frame is NULL";
    sum += frame->command_word;
    for(uint16_t i=0; i < frame->data_length-1; i++)
    {
        sum += frame->command_value[i];
    }
    (frame->checksum )= sum & 0xFF;
    return sum;
}

ld2461_version_t LD2461::get_version_and_id(ld2461_frame_t* frame)
{
    if (frame == NULL) throw "Frame is NULL";

    uint8_t payload[] = {0xFF, 0xEE, 0xDD, 0x00, 0x02, 0x09, 0x01, 0x0A, 0xDD, 0xEE, 0xFF};
    uart_write_bytes(this->uart_num, (const void*)payload, sizeof(payload));
    this->read_data(frame);
    while(frame->command_word != LD2461_COMMAND_ID_AND_VERSION)
    {
        this->read_data(frame);
    }

    ld2461_version_t version = {
        .year = (uint16_t)(2020+(frame->command_value[0] >> 4)),
        .month = (uint8_t)(frame->command_value[0] & 0x0F),
        .day = frame->command_value[1],
        .major = frame->command_value[2],
        .minor = frame->command_value[3],
        .id_number = (uint32_t)(
            (frame->command_value[4] << 24) |
            (frame->command_value[5] << 16) |
            (frame->command_value[6] << 8) |
            (frame->command_value[7])
        )
    };
    return version;
}

void LD2461::report_detections()
{
    ld2461_detection_t detection = ld2461_setup_detection();
    ld2461_frame_t frame = ld2461_setup_frame();

    this->read_data(&frame);
    while(frame.command_word != LD2461_COMMAND_RADAR_REPORT_1)
    {
        this->read_data(&frame);
    }
    for(int i=0; i<(frame.data_length-1)/2; i++)
    {
        detection.target[i].x = frame.command_value[2*i];
        detection.target[i].y = frame.command_value[2*i+1];
    }
    (frame.data_length/2 > 0) ? gpio_set_level(GREEN_LED, 1) : gpio_set_level(GREEN_LED, 0);
    printf("Detected %d target(s): ", frame.data_length/2);
    for(int i=0; i<frame.data_length/2; i++)
    {
        printf("Target[%d](X=%.1f, Y=%.1f) | ", i+1, (float)(detection.target[i].x)/10, (float)(detection.target[i].y)/10);
    }
}

const char* LD2461::frame_to_string(ld2461_frame_t* frame)
{
    static char buffer[100];
    buffer[0] = '\0';
    memset(buffer, 0, sizeof(buffer));
    // Print frame in hexadecimal
    sprintf(buffer, "FFEEDD");
    sprintf(buffer + strlen(buffer), "%02X", frame->data_length);
    sprintf(buffer + strlen(buffer), "%02X", frame->command_word);
    for(uint16_t i=0; i < (frame->data_length-1); i++)
    {
        sprintf(buffer + strlen(buffer), "%02X", frame->command_value[i]);
    }
    sprintf(buffer + strlen(buffer), "%02X", frame->checksum);
    sprintf(buffer + strlen(buffer), "DDEEFF");
    return buffer;
}

void LD2461::print_frame(ld2461_frame_t* frame)
{
    // Print frame in hexadecimal
    printf("Data Length: %d\n", (frame->data_length));
    printf("Command Word: %d\n", frame->command_word);
    printf("Command Value: ");
    for(uint16_t i=0; i < (frame->data_length-1); i++)
    {
        printf("%d ", frame->command_value[i]);
    }
    printf("Checksum: %02X == %02X", (frame->checksum), this->ld2461_generate_checksum(frame));
    printf("\n");
}

void LD2461::change_baudrate(ld2461_baudrate_t baudrate)
{
    uint8_t payload[] = {
        0xFF, 0xEE, 0xDD,   // Header
        0x00, 0x04,         // Data Length
        0x01,               // Command Word
        0x00, 0x00, 0x00,   // Command Value
        0x00,               // Checksum
        0xDD, 0xEE, 0xFF    // End
        };
    switch(baudrate)
    {
        case LD2461_BAUDRATE_9600:
            payload[6] = 0x01;
            payload[7] = 0xC2;
            payload[8] = 0x00;
            payload[9] = 0xC4;
            break;
        case LD2461_BAUDRATE_19200:
            payload[6] = 0x00;
            payload[7] = 0x61;
            payload[8] = 0x00;
            payload[9] = 0x62;
            break;
        case LD2461_BAUDRATE_38400:
            payload[6] = 0x00;
            payload[7] = 0x30;
            payload[8] = 0x00;
            payload[9] = 0x31;
            break;
        case LD2461_BAUDRATE_57600:
            payload[6] = 0x00;
            payload[7] = 0x19;
            payload[8] = 0x00;
            payload[9] = 0x1A;
            break;
        case LD2461_BAUDRATE_115200:
            payload[6] = 0x00;
            payload[7] = 0x0C;
            payload[8] = 0x00;
            payload[9] = 0x0D;
            break;
        case LD2461_BAUDRATE_256000:
            payload[6] = 0x00;
            payload[7] = 0x03;
            payload[8] = 0x00;
            payload[9] = 0x04;
            break;
        default:
            throw "Invalid Baudrate";
    }
    uart_write_bytes(this->uart_num, (const void*)payload, sizeof(payload));
    uart_set_baudrate(this->uart_num, baudrate);
    ld2461_frame_t frame = ld2461_setup_frame();
    this->read_data(&frame);
    while(frame.command_word != LD2461_COMMAND_CHANGE_BAUDRATE)
    {
        this->read_data(&frame);
    }
    if(frame.command_value[0] == 0x01)
    {
        printf("Baudrate changed successfully!\n");
    }
    else
    {
        printf("Baudrate change failed!\n");
    }
}

const char* LD2461::detection_to_json(ld2461_frame_t* frame){
    static char buffer[100];
    buffer[0] = '\0';
    sprintf(buffer, "{ \"detections\": [");
    for(int i=0; i<frame->data_length/2; i++)
    {
        sprintf(buffer + strlen(buffer), "{ \"x\": %.1f, \"y\": %.1f }", (float)(int8_t)(frame->command_value[2*i])/10, (float)(int8_t)(frame->command_value[2*i+1])/10);
        if(i < frame->data_length/2 - 1)
        {
            sprintf(buffer + strlen(buffer), ", ");
        }
    }
    sprintf(buffer + strlen(buffer), "] }");
    return buffer;
}

