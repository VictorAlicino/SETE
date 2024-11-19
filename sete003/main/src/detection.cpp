#include "detection.hpp"
#include "pir.hpp"
#include "mqtt.hpp"
#include "esp_log.h"

#include <esp_timer.h>
#include "math.h"
#include "cJSON.h"

#ifndef MAX_TARGETS_DETECTION
#define MAX_TARGETS_DETECTION 5
#endif

#define POINT_D0 0  // Top left corner
#define POINT_D1 1  // Bottom left corner
#define POINT_D2 2  // Bottom right corner
#define POINT_D3 3  // Top right corner
#define VEC_D0D1 0  // Left face (Vector D0-D1)
#define VEC_D1D2 1  // Bottom face (Vector D1-D2)
#define VEC_D2D3 2  // Right face (Vector D2-D3)
#define VEC_D3D4 3  // Top face (Vector D3-D0)

// Global Variables (plis get rid of those)
extern LD2461* ld2461;
extern PIR* pir;
extern MQTT* mqtt;
extern Sensor* sensor;

const char* DETECTION_TAG = "DETECTION";

Detection::Detection(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3
    )
{
    entered_detections = 0;
    exited_detections = 0;
    gave_up_detections = 0;
    set_detection_area(D0, D1, D2, D3);
    for(int i=0; i<MAX_TARGETS_DETECTION; i++){
        targets[i].current_position = {0, 0};
        targets[i].entered_position = {0, 0};
        targets[i].exited_position = {0, 0};
        targets[i].traversed = false;
        targets[i].entered_side = NONE;
        targets[i].exited_side = NONE;
    }
}

point* Detection::get_detection_area_point()
{
    return detection_area.D;
}

detection_area_t Detection::get_detection_area()
{
    return detection_area;
}

std::pair<bool, float> Detection::_pre_calc_vector_product(
        uint8_t vecAB_index,
        uint8_t pointA_index,
        point_t pointC)
{
    float result = (
        detection_area.F[vecAB_index].x * (pointC.y - detection_area.D[pointA_index].y) -
        detection_area.F[vecAB_index].y * (pointC.x - detection_area.D[pointA_index].x));
    return std::pair<bool, float>(result >= 0, result);
}

bool Detection::_is_target_in_detection_area(point_t point_target)
{
    return _pre_calc_vector_product(VEC_D0D1, POINT_D0, point_target).first &&
           _pre_calc_vector_product(VEC_D1D2, POINT_D1, point_target).first &&
           _pre_calc_vector_product(VEC_D2D3, POINT_D2, point_target).first &&
           _pre_calc_vector_product(VEC_D3D4, POINT_D3, point_target).first;
}

void Detection::set_detection_area(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3
    )
{
    detection_area.D[0] = D0;
    detection_area.D[1] = D1;
    detection_area.D[2] = D2;
    detection_area.D[3] = D3;

    detection_area.F[0] = {D1.x - D0.x, D1.y - D0.y};
    detection_area.F[1] = {D2.x - D1.x, D2.y - D1.y};
    detection_area.F[2] = {D3.x - D2.x, D3.y - D2.y};
    detection_area.F[3] = {D0.x - D3.x, D0.y - D3.y};
}

const char* detection_area_side_str[] = {
    "LEFT",
    "BOTTOM",
    "RIGHT",
    "TOP",
    "NONE"
};

bool Detection::check_if_detected(uint8_t target_index)
{
    bool was_in_detection_area = _is_target_in_detection_area(targets_previous[target_index]);
    bool is_in_detection_area = _is_target_in_detection_area(targets[target_index].current_position);

    if (!was_in_detection_area && is_in_detection_area) {
        targets[target_index].entered_position = targets[target_index].current_position;
        targets[target_index].entered_side = get_crossed_side(targets[target_index].current_position);
        // ESP_LOGI(DETECTION_TAG, "Target %u entered detection area at %s",
        //     target_index,
        //     detection_area_side_str[targets[target_index].entered_side]
        // );
        return true;
    } else if (was_in_detection_area && !is_in_detection_area) {
        targets[target_index].exited_position = targets[target_index].current_position;
        targets[target_index].exited_side = get_crossed_side(targets[target_index].current_position);
        targets[target_index].traversed = true;
        // ESP_LOGI(DETECTION_TAG, "Target %u exited detection area at %s",
        //     target_index,
        //     detection_area_side_str[targets[target_index].exited_side]
        // );
        return false;
    }
    return false;
}

detection_area_side Detection::get_crossed_side(point_t point)
{
    float min = 10; // This value needs to be higher than the maximum value of the vector product to work
    detection_area_side_t side = NONE;

    float a = _pre_calc_vector_product(VEC_D0D1, POINT_D0, point).second; // Left
    float b = _pre_calc_vector_product(VEC_D1D2, POINT_D1, point).second; // Top
    float c = _pre_calc_vector_product(VEC_D2D3, POINT_D2, point).second; // Right
    float d = _pre_calc_vector_product(VEC_D3D4, POINT_D3, point).second; // Bottom

    if(a <= min){min = a; side = LEFT;}
    if(b < min ) {min = b; side = TOP;}
    if(c < min ) {min = c; side = RIGHT;}
    if(d < min ) {min = d; side = BOTTOM;}

    return side;
}

void Detection::update_targets(ld2461_detection_t* report)
{
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        targets_previous[i] = targets[i].current_position;
    }
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        targets[i].current_position = {
            (float)(int8_t)(report->target[i].x)/10,
            (float)(int8_t)(report->target[i].y)/10
        };
    }
}

void Detection::count_detections(int target_index)
{
    if(targets[target_index].traversed == false) return;
    if(targets[target_index].entered_side == NONE || targets[target_index].exited_side == NONE) return;

    // ESP_LOGI(DETECTION_TAG, "Target %u entry point: (%.2f, %.2f) | exit point: (%.2f, %.2f) | entered side: %s | exited side: %s",
    //     target_index,
    //     targets[target_index].entered_position.x,
    //     targets[target_index].entered_position.y,
    //     targets[target_index].exited_position.x,
    //     targets[target_index].exited_position.y,
    //     detection_area_side_str[targets[target_index].entered_side],
    //     detection_area_side_str[targets[target_index].exited_side]
    // );
    switch(targets[target_index].entered_side){
        case TOP:
            // User entered the room
            if(targets[target_index].exited_side == BOTTOM){
                entered_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u entered the room | id: 00",target_index);
            }
            // User gave up
            else if(targets[target_index].exited_side == TOP){
                gave_up_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u gave up entering the room | id: 01",target_index);
            }
            else{
                ESP_LOGW(DETECTION_TAG, "Target %u traversed but exited_side is not defined | id: 02",target_index);
            }
            break;
        case BOTTOM:
            // User exited the room
            if(targets[target_index].exited_side == TOP){
                exited_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u exited the room | id: 10",target_index);
            }
            // User gave up
            else if(targets[target_index].exited_side == BOTTOM){
                gave_up_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u gave up exiting the room | id: 11",target_index);
            }
            else{
                ESP_LOGW(DETECTION_TAG, "Target %u traversed but exited_side is not defined | id: 12",target_index);
            }
            break;
        case LEFT:
            // User entered the room
            if(targets[target_index].exited_side == BOTTOM){
                entered_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u entered the room | id: 20",target_index);
            }
            // User gave up
            else if(targets[target_index].exited_side == TOP){
                exited_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u exited the room | id: 21",target_index);
            }
            else{
                ESP_LOGW(DETECTION_TAG, "Target %u traversed but exited_side is not defined | id: 22",target_index);
            }
            break;
        case RIGHT:
            // User exited the room
            if(targets[target_index].exited_side == TOP){
                exited_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u exited the room | id: 30",target_index);
            }
            // User gave up
            else if(targets[target_index].exited_side == BOTTOM){
                gave_up_detections++;
                ESP_LOGI(DETECTION_TAG, "Target %u gave up exiting the room | id: 31",target_index);
            }
            else{
                ESP_LOGW(DETECTION_TAG, "Target %u traversed but exited_side is not defined | id: 32",target_index);
            }
            break;
        case NONE:
            ESP_LOGW(DETECTION_TAG, "Target %u traversed but entered_side is NONE | id: 40",target_index);
            break;
        default:
            ESP_LOGW(DETECTION_TAG, "Target %u traversed but entered_side is not defined | 50",target_index);
            break;
    }
    targets[target_index].entered_side = NONE;
    targets[target_index].exited_side = NONE;
    targets[target_index].traversed = false;
    // ESP_LOGI(DETECTION_TAG, "Entered: %d, Exited: %d, Gave up: %d",
    //     entered_detections,
    //     exited_detections,
    //     gave_up_detections
    // );
}

void Detection::start_detection()
{
    ld2461_detection_t detection_frame;
    ld2461_setup_detection(&detection_frame);
    ld2461->report_detections(&detection_frame);
    
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        targets[i].current_position = {
            (float)(int8_t)(detection_frame.target[i].x)/10,
            (float)(int8_t)(detection_frame.target[i].y)/10
        };
    }
}

void Detection::mqtt_send_detections()
{
    if(entered_detections == 0 && exited_detections == 0 && gave_up_detections == 0) return;
    std::string payload = "{";
    payload += "\"entered\": " + std::to_string(entered_detections) + ",";
    payload += "\"exited\": " + std::to_string(exited_detections) + ",";
    payload += "\"gave_up\": " + std::to_string(gave_up_detections);
    payload += "}";
    mqtt->publish(
        std::string(sensor->get_mqtt_root_topic() + "/data").c_str(),
        payload.c_str()
    );
    entered_detections = 0;
    exited_detections = 0;
    gave_up_detections = 0;
}

void Detection::set_raw_data_sent(bool send_raw_data)
{
    send_raw_detection_payload = send_raw_data;
}

void Detection::detect()
{
    ld2461_detection_t detection_frame;
    ld2461_setup_detection(&detection_frame);
    ld2461->report_detections(&detection_frame);

    if(detection_frame.detected_targets == 0)
    {
        //ESP_LOGI(DETECTION_TAG, "No targets detected");
        return;
    }

    std::string detection_payload = "{";
    std::string targets_str = "Target in Area: ";
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        if(check_if_detected(i)) {
            targets_str += std::to_string(i) + ", ";
        }
        count_detections(i);
        detection_payload += "\"t_" + std::to_string(i) + "\": {";
        detection_payload += "\"x\": " + std::to_string(targets[i].current_position.x) + ",";
        detection_payload += "\"y\": " + std::to_string(targets[i].current_position.y);
        detection_payload += "},";
    }
    detection_payload.pop_back(); // Remove last comma
    detection_payload += "}";
    // if(targets_str.length() > 16) ESP_LOGI(DETECTION_TAG, "%s",targets_str.c_str());
    if(send_raw_detection_payload){
        mqtt->publish(
            std::string(sensor->get_mqtt_root_topic() + "/raw").c_str(),
            detection_payload.c_str()
        );
    }
    update_targets(&detection_frame);
}
