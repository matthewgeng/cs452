#ifndef TRAINCONSTANTS_H
#define TRAINCONSTANTS_H

#include <stdint.h>

int train_terminal_speed(uint32_t train_num, uint32_t train_speed);
int train_stopping_acceleration(uint32_t train_num, uint32_t train_speed);
int sensor_distance_between(char track, uint32_t src_sensor, uint32_t dest_sensor);

#endif
