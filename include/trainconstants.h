#ifndef TRAINCONSTANTS_H
#define TRAINCONSTANTS_H

#include <stdint.h>

#define VALID_TRAINS {1, 2, 54, 55, 58, 77}

int train_terminal_speed(uint32_t train_num, uint32_t train_speed);
int train_stopping_acceleration(uint32_t train_num, uint32_t train_speed);
int train_velocity_offset(uint32_t train_num, uint32_t train_speed);
int sensor_distance_between(char track, uint32_t src_sensor, uint32_t dest_sensor);
int starting_sensor_for_train(char track, uint32_t train_id);
int starting_next_sensor_for_train(char track, uint32_t train_id);
int train_min_speed(uint32_t train_num);
int stopping_dist_for_train(char track, uint32_t train_id);
int stopping_speed_for_train(char track, uint32_t train_id);
int nav_reverse_stop_offset(uint32_t train_id);

#endif
