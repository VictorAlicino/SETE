#include "detection.hpp"
#include "pir.hpp"
#include "mqtt.hpp"
#include "esp_log.h"

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
    }
}

bool Detection::_pre_calc_vector_product(
        uint8_t vecAB_index,
        uint8_t pointA_index,
        point_t pointC)
{
    return (
        detection_area.F[vecAB_index].x * (pointC.y - detection_area.D[pointA_index].y) -
        detection_area.F[vecAB_index].y * (pointC.x - detection_area.D[pointA_index].x)) >= 0;
}

bool Detection::_is_target_in_detection_area(point_t point_target)
{
    return _pre_calc_vector_product(VEC_D0D1, POINT_D0, point_target) &&
           _pre_calc_vector_product(VEC_D1D2, POINT_D1, point_target) &&
           _pre_calc_vector_product(VEC_D2D3, POINT_D2, point_target) &&
           _pre_calc_vector_product(VEC_D3D4, POINT_D3, point_target);
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

bool Detection::check_if_detected(uint8_t target_index) {
    bool was_in_detection_area = _is_target_in_detection_area(targets_previous[target_index]);
    bool is_in_detection_area = _is_target_in_detection_area(targets[target_index].current_position);

    if (!was_in_detection_area && is_in_detection_area) {
        ESP_LOGI(DETECTION_TAG, "Target %u entered detection area",target_index);
        targets[target_index].entered_position = targets[target_index].current_position;
        return true;
    } else if (was_in_detection_area && !is_in_detection_area) {
        // Check if exited on the same side
        ESP_LOGI(DETECTION_TAG, "Target %u exited detection area",target_index);
        targets[target_index].exited_position = targets[target_index].current_position;
        ESP_LOGI(DETECTION_TAG, "Target %u entered at (%.1f, %.1f) and exited at (%.1f, %.1f)\n",
            target_index,
            targets[target_index].entered_position.x,
            targets[target_index].entered_position.y,
            targets[target_index].exited_position.x,
            targets[target_index].exited_position.y
        );
        targets[target_index].traversed = true;
        return false;
    }
    return false;
}

detection_area_side Detection::get_crossed_side(point_t point) {
    if (point.x <= detection_area.D[0].x) return LEFT;
    if (point.x >= detection_area.D[2].x) return RIGHT;
    if (point.y <= detection_area.D[0].y) return BOTTOM;
    if (point.y >= detection_area.D[1].y) return TOP;
    return NONE;
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

void Detection::start_detection() {
    ld2461_detection_t detection_frame = ld2461_setup_detection();
    ld2461->report_detections(&detection_frame);
    
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        targets[i].current_position = {
            (float)(int8_t)(detection_frame.target[i].x)/10,
            (float)(int8_t)(detection_frame.target[i].y)/10
        };
    }
}

void Detection::detect() {
    ld2461_detection_t detection_frame = ld2461_setup_detection();
    ld2461->report_detections(&detection_frame);

    if(detection_frame.detected_targets == 0)
    {
        //ESP_LOGI(DETECTION_TAG, "No targets detected");
        return;
    }

    std::string targets_str = "Target in Area: ";
    for(int i=0; i<MAX_TARGETS_DETECTION; i++)
    {
        if(check_if_detected(i)) {
            targets_str += std::to_string(i) + ", ";
        }
        if(targets[i].traversed == true){
            detection_area_side side = get_crossed_side(targets[i].exited_position);
            switch (side)
            {
            case LEFT:
                ESP_LOGI(DETECTION_TAG, "Target %u exited on the LEFT side",i);
                break;
            case RIGHT:
                ESP_LOGI(DETECTION_TAG, "Target %u exited on the RIGHT side",i);
                break;
            case BOTTOM:
                ESP_LOGI(DETECTION_TAG, "Target %u exited on the BOTTOM side",i);
                break;
            case TOP:
                ESP_LOGI(DETECTION_TAG, "Target %u exited on the TOP side",i);
                break;
            case NONE:
                ESP_LOGI(DETECTION_TAG, "Target %u exited on NONE side",i);
                break;
            }
            targets[i].traversed = false;
        }
    }
    //ESP_LOGI(DETECTION_TAG, "%s",targets_str.c_str());
    update_targets(&detection_frame);
}