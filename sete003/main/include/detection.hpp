
/*
D0--------------------------------------D3    D0--------D3
|                                        |    |          |
|                                        |    |          |
|               Horizontal               |    | Vertical |
|                                        |    |          |
|                                        |    |          |
D1--------------------------------------D2    D1--------D2
*/

#pragma once

#include "ld2461.hpp"
#include "mqtt.hpp"
#include "sensor.hpp"

typedef struct point{
    float x;
    float y;
}point_t;

typedef point vec2_t;

typedef struct detection_area{
    /*
    D0: Top left corner
    D1: Bottom left corner
    D2: Bottom right corner
    D3: Top right corner

    F0: D0-D1
    F1: D1-D2
    F2: D2-D3
    F3: D3-D0
    */
    point_t D[4]; // Points of the detection area
    vec2_t F[4]; // Vectors of detection area faces
}detection_area_t;

class Detection{
private:
    detection_area_t detection_area;
    point_t targets_previous[MAX_TARGETS_DETECTION];
    point_t targets_current[MAX_TARGETS_DETECTION];

    int entered_detections;
    int exited_detections;
    int gave_up_detections;

    bool _pre_calc_vector_product(
        uint8_t vecAB_index,
        uint8_t pointA_index,
        point_t pointC);
    bool _is_target_in_detection_area(point_t target);

public:
    void set_detection_area(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3
    );


};