#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/ringbuf.h"
#include "ld2461.hpp"

#include <string.h>

#define HEADER 0xFFEEDD
#define FOOTER 0xDDEEFF

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    // UART Workflow

    // 1.Set Communication Parameters - Setting baud rate, data bits, stop bits, etc.
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    
    // 2.Set Communication Pins - Assigning pins for connection to a device
    ESP_ERROR_CHECK(uart_set_pin(
        uart_num,           // UART Num
        GPIO_NUM_36,        // TX
        GPIO_NUM_35,        // RX
        UART_PIN_NO_CHANGE, // RTS
        UART_PIN_NO_CHANGE  // CTS
    ));

    // 3.Install Drivers - Allocating ESP32's resources for the UART driver
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(
        uart_num,           // UART Num
        uart_buffer_size,   // RX Buffer Size
        0,                  // TX Buffer Size
        10,                 // Queue Size
        &uart_queue,        // Pointer to Queue
        0                   // Flags to Interruptions
    ));

    // 4.Run UART Communication - Sending/receiving data

    // Sending
    //get_version_number_and_uid(&frame);
    uint8_t datas[] = {0xFF, 0xEE, 0xDD, 0x00, 0x02, 0x09, 0x01, 0x0A, 0xDD, 0xEE, 0xFF};
    uart_write_bytes(uart_num, (const void*)datas, sizeof(datas));

    // Receiving
    uint8_t* readed_frame = (uint8_t*)malloc(100);
    ld2461_frame_t frame = ld2461_setup_frame();


    uint8_t* data_length = (uint8_t*)malloc(2);
    uint8_t* command_word = (uint8_t*)malloc(100);
    uint8_t* checksum = (uint8_t*)malloc(100);
    uint8_t* data = (uint8_t*)malloc(100);
    while(*command_word != 0x09)
    {
        // Catch the Header
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xFF) {continue;}
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xEE) {continue;}
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}

        // Data Length - Command Word (1Byte) + Command Value
        uart_read_bytes(uart_num, data_length, 2, 100);
        *data_length = (data_length[0] << 8) | data_length[1];

        // Command Word
        uart_read_bytes(uart_num, command_word, 1, 100);

        // Command Value
        uint8_t* command_value = (uint8_t*)malloc(*data_length-1);
        for(int i=0; i<(*data_length-1); i++)
        {
            uart_read_bytes(uart_num, command_value+i, 1, 100);
        }

        // Checksum
        uart_read_bytes(uart_num, checksum, 1, 100);

        // Catch the footer
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xEE) {printf("\n");continue;}
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xFF) {printf("\n");continue;}
        
        // Building the frame
        readed_frame[0] = 0xFF;
        readed_frame[1] = 0xEE;
        readed_frame[2] = 0xDD;
        readed_frame[3] = *data_length;
        readed_frame[4] = *command_word;
        for(int i=0; i<(*data_length-1); i++)
        {
            readed_frame[5+i] = command_value[i];
        }
        readed_frame[5+(*data_length-1)] = *checksum;
        readed_frame[6+(*data_length-1)] = 0xDD;
        readed_frame[7+(*data_length-1)] = 0xEE;
        readed_frame[8+(*data_length-1)] = 0xFF;

        // Printing the frame
        //for(int i=0; i<9+(*data_length-1); i++)
        //{
        //    printf("%02X", readed_frame[i]);
        //}
        //printf("\n");
        //frame.data_length = *data_length;
        //frame.command_word = (ld2461_command_word_t)(*command_word);
        //frame.command_value = command_value;
        //frame.checksum = *checksum;
        free(command_value);
    }
    ld2461_version_t version = {
        .year = (uint16_t)(2020+(readed_frame[5] >> 4)),
        .month = (uint8_t)(readed_frame[5] & 0x0F),
        .day = readed_frame[6],
        .major = readed_frame[7],
        .minor = readed_frame[8],
        .id_number = (uint32_t)(
            (readed_frame[9] << 24) |
            (readed_frame[10] << 16) |
            (readed_frame[11] << 8) |
            (readed_frame[12])
        )
    };
    printf("LD2461 Detected!\n");
    printf("ID: ");for(int i=0; i<4; i++)
    {
        printf("%02X", (uint8_t)(version.id_number >> (8*(3-i))));
    }printf("\n");
    printf("Version: %d.%d\n", version.major, version.minor);
    printf("Date: %02d/%02d/%02d\n", version.day, version.month, version.year);

    free(data_length);
    free(command_word);
    free(checksum);
    free(data);
    // 5.Use Interrupts - Triggering interrupts on specific communication events
        // Nah ...
    
    // 6.Deleting a Driver - Freeing allocated resources if a UART communication is no longer required
    uart_driver_delete(uart_num);
}
