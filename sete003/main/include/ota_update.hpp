#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include <string>

/**
 * @brief Check if the OTA update was successful
 * @note Taken from "simple_ota_example" from esp-idf examples
 * @return esp_err_t
 */
esp_err_t check_ota_sucess(void);

/**
 * @brief Get the sha256 of partitions object
 * @note Taken from "simple_ota_example" from esp-idf examples
 */
static void get_sha256_of_partitions(void);

/**
 * @brief Print the sha256 of the image
 * @note Taken from "simple_ota_example" from esp-idf examples
 * @param image_hash 
 * @param label 
 */
static void print_sha256(const uint8_t *image_hash, const char *label);

/**
 * @brief Event handler for http client
 * @note Taken from "simple_ota_example" from esp-idf examples
 * @param evt 
 * @return esp_err_t 
 */
esp_err_t _htpp_event_handler(esp_http_client_event_t *evt);

/**
 * @brief Update the firmware
 * @note Taken from "simple_ota_example" from esp-idf examples
 */
void ota_update(std::string uri);
