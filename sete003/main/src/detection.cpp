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
    set_detection_area(D0, D1, D2, D3);
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

bool Detection::_is_target_in_detection_area(point_t target)
{
    return _pre_calc_vector_product(VEC_D0D1, POINT_D0, target) &&
           _pre_calc_vector_product(VEC_D1D2, POINT_D1, target) &&
           _pre_calc_vector_product(VEC_D2D3, POINT_D2, target) &&
           _pre_calc_vector_product(VEC_D3D4, POINT_D3, target);
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

void Detection::start_detection()
{
    std::string ld2461_payload, pir_payload, payload;
    ld2461_detection_t detection_frame = ld2461_setup_detection();
    ld2461->report_detections(&detection_frame);
    for(int i=0; i<detection_frame.detected_targets; i++){
        printf("Target[%d](X=%.1f, Y=%.1f) | ", i, (float)(detection_frame.target[i].x)/10, (float)(detection_frame.target[i].y)/10);
    }
    printf("\n");
    //ld2461_frame_t ld2461_frame = ld2461_setup_frame();
    //ld2461->read_data(&ld2461_frame);
    //ld2461_payload = ld2461->detection_to_json(&ld2461_frame);
    //pir_payload = pir->read_to_json();

    //payload = (
    //    "{"
    //        "\"ld2461\": " + ld2461_payload + ","
    //        "\"pir\": " + pir_payload +
    //    "}"
    //);

    //mqtt->publish(
    //    std::string(sensor->get_mqtt_root_topic() + "/data").c_str(),
    //    payload.c_str()
    //);

}
