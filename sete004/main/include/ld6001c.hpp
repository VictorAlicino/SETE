/*
LD6001C Detection Frame Format
--------------------
FRAME HEADER - NUMBER OF BYTES -   TYPE   - ENTRIES - EXITS - CHECK (XOR)
55 AA          1 byte            1 byte    2 bytes   2 bytes  1 byte
*/

#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"

typedef struct ld6001
{
    float PeopleCntSoftVerison;
    float RangeRes;
    float VelRes;
    uint16_t TIME;
    uint8_t PROG;
    uint16_t Range;
    uint8_t Sen;
    uint8_t Heart_Time;
    uint8_t Debug;
    uint16_t Height;
    int8_t XboundaryN;
    int8_t XboundaryP;
    int8_t YboundaryN;
    int8_t YboundaryP;
}ld6001_t;

typedef struct ld6001_frame
{
    uint8_t number_of_bytes;
    uint8_t type;
    uint16_t entries;
    uint16_t exits;
    uint8_t check;
}ld6001_frame_t;

typedef enum ld6001_AT_command{
    START = 0x01,
    STOP = 0x00,
    READ = 0x02,
    SENSIBILITY = 0x03,
    TIME = 0x04,
    HEATIME = 0x05,
    RANGE = 0x06,
    DEBUG = 0x07,
    X_POSITIVE = 0x08,
    X_NEGATIVE = 0x09,
    Y_POSITIVE = 0x0A,
    Y_NEGATIVE = 0x0B,
    HEIGHT = 0x0C
}

class LD6001C
{
private:
    uart_port_t uart_num;
    uint8_t tx_pin;
    uint8_t rx_pin;
public:
    ld6001_t info

    /**
     * @brief Construct a new LD6001C object
     * 
     * @warning Avoid using UART_NUM_0, it is used by the ESP32 UART
     * 
     * @param uart_num UART port number
     * @param tx_pin TX Pin GPIO Number
     * @param rx_pin RX Pin GPIO Number
     */
    LD6001C(
        uart_port_t uart_num,
        uint8_t tx_pin,
        uint8_t rx_pin
    );

    void read_data(ld6001_frame_t* frame);

    bool send_at_command();
}