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
        0,   // TX Buffer Size
        10,                 // Queue Size
        &uart_queue,        // Pointer to Queue
        0                   // Flags to Interruptions
    ));

    // 4.Run UART Communication - Sending/receiving data

    // Sending
    ld2461_frame_t frame = {
        .header = LD2461_FRAME_HEADER,
        .data_length = 2,
        .command_word = LD2461_COMMAND_ID_AND_VERSION,
        .command_value = {0},  // Inicialize com zeros ou valores específicos
        .checksum = 0,
        .end = LD2461_FRAME_END
    };
    get_version_number_and_id(&frame);
    uint8_t datas[] = {0xFF, 0xEE, 0xDD, 0x00, 0x02, 0x09, 0x01, 0x0A, 0xDD, 0xEE, 0xFF};
    uart_write_bytes(uart_num, (const void*)datas, sizeof(datas));
    //uart_write_bytes(uart_num, (void *)0xFFEEDD, sizeof(0xFFEEDD));
    //uart_write_bytes(uart_num, (void *)0x0002, sizeof(0x0002));
    //uart_write_bytes(uart_num, (void *)0x09, sizeof(0x09));
    //uart_write_bytes(uart_num, (void *)0x01, sizeof(0x01));
    //uart_write_bytes(uart_num, (void *)0x0A, sizeof(0x0A));
    //uart_write_bytes(uart_num, (void *)0xFFEEDD, sizeof(0xFFEEDD));
    //if(uart_wait_tx_done(uart_num, 100) == ESP_OK)
    //{
    //    printf("Data sent successfully\n");
    //}
    //else
    //{
    //    printf("Data not sent\n");
    //}
    //// Receiving
    //uint8_t datar[128];
    //int length = 0;
    //ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    //length = uart_read_bytes(uart_num, datar, length, 10);
    //if (length < 0)
    //{
    //    printf("Error reading data\n");
    //}
    //printf("Received: ");
    //for(int i=0; i < length; i++)
    //{
    //    printf("%02X", datar[i]);
    //}
    //printf("\n");
    //uart_flush(uart_num);
    // Loop printing the received data
    
    
    uint8_t* data_length = (uint8_t*)malloc(2);
    uint8_t* command_word = (uint8_t*)malloc(100);
    uint8_t* checksum = (uint8_t*)malloc(100);
    uint8_t* data = (uint8_t*)malloc(100);
    while(1)
    {
        // Catch the Header
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xFF) {continue;}
        //printf("%02X", *data);
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xEE) {continue;}
        printf("FF%02X", *data);
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}
        printf("%02X ", *data);

        // Data Length - Command Word (1Byte) + Command Value
        uart_read_bytes(uart_num, data_length, 2, 100);
        *data_length = (data_length[0] << 8) | data_length[1];
        printf("%02X ", *data_length);

        // Command Word
        uart_read_bytes(uart_num, command_word, 1, 100);
        printf("%02X ", *command_word);

        // Command Value
        uint8_t* command_value = (uint8_t*)malloc(*data_length-1);
        for(int i=0; i<(*data_length-1); i++)
        {
            uart_read_bytes(uart_num, command_value+i, 1, 100);
        }
        for(int i=0; i<(*data_length-1); i++)
        {
            printf("%02X", command_value[i]);
        }

        // Checksum
        uart_read_bytes(uart_num, checksum, 1, 100);
        printf(" %02X ", *checksum);

        // Catch the footer
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xDD) {printf("\n");continue;}
        printf("%02X", *data);
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xEE) {printf("\n");continue;}
        printf("%02X", *data);
        uart_read_bytes(uart_num, data, 1, 100);
        if(*data != 0xFF) {printf("\n");continue;}
        printf("%02X ", *data);
        printf("\n");
        free(command_value);
    }
    free(data_length);
    free(command_word);
    free(checksum);
    free(data);
    // 5.Use Interrupts - Triggering interrupts on specific communication events
    // 6.Deleting a Driver - Freeing allocated resources if a UART communication is no longer required
}
