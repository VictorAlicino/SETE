/*
LD2461 Frame Format
-------------------
Be advised, LD2461 Communication Protocol uses the Big Endian format!!!
FRAME HEADER - DATA LENGHT - COMMAND WORD - COMMAND VALUE - CHECKSUM - FRAME END
0xFFEEDD        2 bytes        1 byte          N bytes       1 byte     0xDDEEFF
*/

#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/uart.h"

#define MAX_TARGETS_DETECTION 5


enum ld2461_baudrate_t
{
    LD2461_BAUDRATE_9600 = 0x002580,
    LD2461_BAUDRATE_19200 = 0x004B00,
    LD2461_BAUDRATE_38400 = 0x009600,
    LD2461_BAUDRATE_57600 = 0x00E100,
    LD2461_BAUDRATE_115200 = 0x01C200,
    LD2461_BAUDRATE_256000 = 0x03E800
};

enum ld2461_command_word_t : uint8_t
{
    LD2461_COMMAND_NULL = 0x00,
    LD2461_COMMAND_CHANGE_BAUDRATE = 0x01,
    LD2461_COMMAND_SET_RADAR = 0x02,
    LD2461_COMMAND_READ_RADAR = 0x03,
    LD2461_COMMAND_ZONE_FILTERING = 0x04,
    LD2461_COMMAND_WITHDRAW_AREAS = 0x05,
    LD2461_COMMAND_READING_AREAS = 0x06,
    LD2461_COMMAND_RADAR_REPORT_1 = 0x07,
    LD2461_COMMAND_RADAR_REPORT_2 = 0x08,
    LD2461_COMMAND_ID_AND_VERSION = 0x09,
    LD2461_COMMAND_RESET = 0x0A,
};

enum ld2461_frame_guard_t : uint32_t
{
    LD2461_FRAME_HEADER = 0xFFEEDD,
    LD2461_FRAME_END = 0xDDEEFF
};

typedef struct ld2461_version
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t major;
    uint8_t minor;
    uint32_t id_number;
}ld2461_version_t;

typedef struct ld2461
{
    ld2461_version_t version;
    uint64_t uid;
    uart_port_t uart_num;
}ld2461_t;

typedef struct ld2461_frame
{
    uint8_t data_length;                // 2 bytes
    ld2461_command_word_t command_word; // 1 byte
    uint8_t* command_value;             // N bytes
    uint8_t checksum;                   // 1 byte
}ld2461_frame_t;

typedef struct ld2461_coordinate
{
    int8_t x;
    int8_t y;
}ld2461_coordinate_t;

typedef struct ld2461_detection
{
    struct ld2461_coordinate target[MAX_TARGETS_DETECTION];
    int8_t detected_targets;
}ld2461_detection_t;

/**
 * @brief Setup a frame with the default values
 * 
 * @return ld2461_frame_t Frame with default values
 */
ld2461_frame_t ld2461_setup_frame();

/**
 * @brief Setup a detection with the default values
 * 
 * @return ld2461_detection_t Detection with default values
 */
ld2461_detection_t ld2461_setup_detection();

class LD2461{
private:
    uart_port_t uart_num;
    /**
     * @brief Generate the checksum for the frame
     * 
     * @param frame Pointer to the frame
     * @return uint8_t Sum of the checksum
     */
    uint8_t ld2461_generate_checksum(ld2461_frame_t* frame);
public:
    /**
     * @brief Construct a new LD2461 object
     * 
     * @note Default Baudrate is 9600
     * @warning Avoid using UART_NUM_0, it is used by the ESP32 UART
     * 
     * @param uart_num UART port number
     * @param tx_pin TX Pin GPIO Number
     * @param rx_pin RX Pin GPIO Number
     * @param baudrate Baudrate for the communication
     */
    LD2461(
        uart_port_t uart_num,
        gpio_num_t tx_pin,
        gpio_num_t rx_pin,
        int baudrate
    );

    /**
     * @brief Read data from the LD2461
     * 
     * @param frame Frame to store the data readed
     */
    void read_data(ld2461_frame_t* frame);

    /**
     * @brief Change the LD2461 baudrate
     * 
     * @warning Note that the baudrate change can not be reverted by reseting to factory settings
     * 
     * @param baudrate Baudrate to change
     */
    void change_baudrate(ld2461_baudrate_t baudrate);
    
    /**
     * @brief Get the version and id
     * 
     * @param frame Frame to store the data readed
     * @return ld2461_version_t LD2461 Version and ID
     */
    ld2461_version_t get_version_and_id(ld2461_frame_t* frame);

    /**
     * @brief Get the detected targets
     * 
     * @param detection Targets detected
     */
    void report_detections(ld2461_detection_t* detection);

    /**
     * @brief Convert the frame to a cstring
     * 
     * @param frame Frame to convert
     * @return const char* cstring with the frame data
     */
    const char* frame_to_string(ld2461_frame_t* frame);

    /**
     * @brief Print the frame to the standard output
     * 
     * @param frame Frame to print
     */
    void print_frame(ld2461_frame_t* frame);

    /**
     * @brief Convert the detection from a frame to a JSON in cstring
     * 
     * @param frame Frame to convert
     * @return const char* cstring with the JSON data
     */
    const char* detection_to_json(ld2461_frame_t* frame);
};
