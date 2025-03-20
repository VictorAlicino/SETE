#pragma once

#include "nvs.h"

typedef enum : uint8_t {
    SENSOR_BASIC_DATA = 0,
    WIFI_BASIC_DATA = 1,
} storage_type_t;

class Storage{
private:
public:
    Storage();

    // Save Data
    void store_data_str(storage_type_t type, const char* key, const char* value); // Store String

    void store_data_int32(storage_type_t type, const char* key, int32_t value);   // Store Integers
    void store_data_int64(storage_type_t type, const char* key, int64_t value);   // Store Integers
    void store_data_uint8(storage_type_t type, const char* key, uint8_t value);   // Store Integers
    void store_data_uint16(storage_type_t type, const char* key, uint16_t value); // Store Integers
    void store_data_uint32(storage_type_t type, const char* key, uint32_t value); // Store Integers

    // Get Data
    char* get_str(storage_type_t type, const char* key);       // Get String

    int get_int32(storage_type_t type, const char* key);         // Get Integers
    int64_t get_int64(storage_type_t type, const char* key);   // Get Integers
    uint8_t get_uint8(storage_type_t type, const char* key);   // Get Integers
    uint16_t get_uint16(storage_type_t type, const char* key); // Get Integers
    uint32_t get_uint32(storage_type_t type, const char* key); // Get Integers

};