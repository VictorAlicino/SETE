#pragma once

#include "include/ld2461.hpp"
#include "driver/uart.h"

typedef struct point2{
    int8_t x;
    int8_t y;
}point2_t;

typedef struct server_payload{
    point2_t T0;
    point2_t T1;
    point2_t T2;
    point2_t T3;
    point2_t T4;
}radar_sample;

void send_radar_sample(radar_sample sample, uart_port_t uart_num);