#include "detection.hpp"

#define POINT_D0 0
#define POINT_D1 1
#define POINT_D2 2
#define POINT_D3 3
#define VEC_D0D1 0
#define VEC_D1D2 1
#define VEC_D2D3 2
#define VEC_D3D4 3
 
bool Detection::_pre_calc_vector_product(uint8_t vecAB_index, uint8_t pointA_index, point_t pointC)
{
    return (detection_area.F[vecAB_index].x * (pointC.y - detection_area.D[pointA_index].y) -
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
