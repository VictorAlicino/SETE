#include "storage.hpp"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include <cstring>

const char* STORAGE_TAG = "STORAGE";

const char* storage_type_name[] = {
    "SENSORS",
    "WIFI"
};

Storage::Storage(){
    //Initialize NVS
    ESP_LOGI(STORAGE_TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGW(STORAGE_TAG, "No free pages or new version found, erasing flash");
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(STORAGE_TAG, "NVS initialized");
}

void Storage::store_data_str(storage_type_t type, const char* key, const char* value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_str(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %s in %s", value, key);
}

void Storage::store_data_int32(storage_type_t type, const char* key, int32_t value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_i32(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %ld in %s", value, key);
}

void Storage::store_data_int64(storage_type_t type, const char* key, int64_t value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_i64(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %lld in %s", value, key);
}

void Storage::store_data_uint8(storage_type_t type, const char* key, uint8_t value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_u8(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %u in %s", value, key);
}

void Storage::store_data_uint16(storage_type_t type, const char* key, uint16_t value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_u16(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %u in %s", value, key);
}

void Storage::store_data_uint32(storage_type_t type, const char* key, uint32_t value){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    nvs_set_u32(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("STORAGE", "Stored %lu in %s", value, key);
}


char* Storage::get_str(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    size_t required_size;
    nvs_get_str(nvs_handle, key, NULL, &required_size);
    char* value = (char*)malloc(required_size); // FOR THE LOVE OF GOD, DON'T FORGET TO FREE THIS MEMORY
    err = nvs_get_str(nvs_handle, key, value, &required_size);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}

int Storage::get_int32(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    int32_t value;
    err = nvs_get_i32(nvs_handle, key, &value);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}

int64_t Storage::get_int64(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    int64_t value;
    err = nvs_get_i64(nvs_handle, key, &value);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}

uint8_t Storage::get_uint8(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    uint8_t value;
    err = nvs_get_u8(nvs_handle, key, &value);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}

uint16_t Storage::get_uint16(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    uint16_t value;
    err = nvs_get_u16(nvs_handle, key, &value);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}

uint32_t Storage::get_uint32(storage_type_t type, const char* key){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(storage_type_name[type], NVS_READONLY, &nvs_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }
    uint32_t value;
    err = nvs_get_u32(nvs_handle, key, &value);
    if(err != ESP_OK)
    {
        ESP_LOGE(STORAGE_TAG, "Error (%s) reading value!", esp_err_to_name(err));
        return NULL;
    }
    nvs_close(nvs_handle);
    return value;
}
