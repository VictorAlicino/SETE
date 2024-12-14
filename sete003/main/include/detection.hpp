
/*
D0------------------Bottom---------------D3    D0--------D3
|                                        |    |          |
|   S0----------Detection Line------S1   |    |          |
|                                        |    |          |
Left               Horizontal          Right  | Vertical |
|                                        |    |          |
|                                        |    |          |
|                                        |    |          |
D1-------------------Top-----------------D2    D1--------D2
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

    F_D0: distance(D0-D1)
    F_D1: distance(D1-D2)
    F_D2: distance(D2-D3)
    F_D3: distance(D3-D0)

    L: S0-S1
    L_D: sqrt((S0.x - S1.x)^2 + (S0.y - S1.y)^2)
    */
    point_t D[4];                   // Points of the detection area
    point_t S[2];                   // Points of the detection segment
    vec2_t F[4];                    // Segment of detection area faces
    float F_D[4];                   // Length of the detection area faces
    vec2_t L;                       // Segment of the detection area
    float L_D;                      // Length of the detection line
    uint8_t portrait_landscape;     // 0: Portrait, 1: Landscape
}detection_area_t;

typedef enum detection_area_side{
    LEFT,
    BOTTOM,
    RIGHT,
    TOP,
    NONE
}detection_area_side_t;

typedef struct target{
    point_t current_position;                   // Current position of the target
    point_t previous_position;                  // Previous position of the target
    float previous_distance;                    // Distance from the previous position to the detection line
    bool line_side;                             // Side of the detection line where the target is
    point_t entered_position;                   // Position where the target entered the detection area
    point_t exited_position;                    // Position where the target exited the detection area
    point_t detection_segment_crossed_position; // Position where the target crossed the detection segment
    detection_area_side_t entered_side;         // Side where the target entered the detection area
    detection_area_side_t exited_side;          // Side where the target exited the detection area
    bool traversed;                             // Flag to indicate if the target traversed the detection area
}target_t;

class Detection{
private:
    detection_area_t detection_area;
    point_t targets_previous[MAX_TARGETS_DETECTION];
    target_t targets[MAX_TARGETS_DETECTION];

    int entered_detections;
    int exited_detections;
    int gave_up_detections;

    bool send_raw_detection_payload;
    bool enter_exit_inverted;

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
    std::pair<bool, float> _pre_calc_vector_product_area(
        uint8_t vecAB_index,
        uint8_t pointA_index,
        point_t pointC
    );

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
     * 
     */
    Detection(
        point_t D0,
        point_t D1,
        point_t D2,
        point_t D3,
        point_t S0,
        point_t S1,
        bool enter_exit_inverted = false
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
        point_t D3,
        point_t S0,
        point_t S1
    );

    /**
     * @brief Get the detection area
     * 
     * @return detection_area_t 
     */
    detection_area_t get_detection_area();

    point_t* get_detection_area_point();

    detection_area_side_t get_crossed_side(point_t point);

    void set_raw_data_sent(bool send_raw_data);
    void set_enter_exit_inverted(bool inverted);

    std::pair<bool, float> _pre_calc_vector_product_segment(point_t pointC);

    bool check_if_detected(uint8_t target_index);
    void start_detection();
    void update_targets(ld2461_detection_t* report);
    void count_detections(int target_index);
    void detect();
    void mqtt_send_detections();
};