
/*
D0-------------------Bottom--------------D3    D0--------D3
|                                        |    |          |
|                                        |    |          |
Left               Horizontal          Right  |    | Vertical |
|                                        |    |          |
|                                        |    |          |
D1--------------------Top----------------D2    D1--------D2
                   \      |
                    \    |
|<------------------LD2461-------------->|
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

typedef enum detection_area_side{
    LEFT,
    BOTTOM,
    RIGHT,
    TOP,
    NONE
}detection_area_side_t;

typedef struct target{
    point_t current_position;
    point_t entered_position;
    detection_area_side_t entered_side;
    point_t exited_position;
    detection_area_side_t exited_side;
    bool traversed;
}target_t;

class Detection{
private:
    detection_area_t detection_area;
    point_t targets_previous[MAX_TARGETS_DETECTION];
    target_t targets[MAX_TARGETS_DETECTION];

    int entered_detections;
    int exited_detections;
    int gave_up_detections;

    int64_t buffer_detection_payload_timer;

    /**
     * @brief Vector product of the two vectors
     * @note This vector product is altered to use the values pre-calculated 
     * from the detection_area, so it is not necessary to calculate the vector again
     * 
     * @param vecAB_index Index of the vector AB in the detection_area
     * @param pointA_index Index of point A from AB in the detection_area
     * @param pointC Target point detected from the LD2461
     * @return true If the target is on the left of the vector AB
     * @return false If the target is not on the left of the vector AB
     */
    bool _pre_calc_vector_product(
        uint8_t vecAB_index,
        uint8_t pointA_index,
        point_t pointC);

    /**
     * @brief Check if the target is inside the detection area
     * 
     * @param target Target point detected from the LD2461
     * @return true If the target is inside the detection area
     * @return false If the target is not inside the detection area
     */
    bool _is_target_in_detection_area(point_t target);

public:
    /**
     * @brief Construct a new Detection object
     * 
     * @param D0 Top left corner
     * @param D1 Bottom left corner
     * @param D2 Bottom right corner
     * @param D3 Top right corner
     * @param buffer_time Time to wait before sending the next detection payload
     * 
     * @note The buffer_time is in microseconds
     */
    Detection(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3,
        int64_t buffer_time = 300000000
    );

    /**
     * @brief Set the detection area
     * 
     * @param D0 Top left corner
     * @param D1 Bottom left corner
     * @param D2 Bottom right corner
     * @param D3 Top right corner
     */
    void set_detection_area(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3
    );

    void set_buffer_time(int64_t buffer_time);

    detection_area_side_t get_crossed_side(point_t point);
    bool check_if_detected(uint8_t target_index);
    void start_detection();
    void update_targets(ld2461_detection_t* report);
    void count_detections(int target_index);
    void detect();
    void mqtt_send_detections();
};