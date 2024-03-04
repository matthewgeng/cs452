#ifndef TRAINCONTROL_H
#define TRAINCONTROL_H

#include "rpi.h"
#include "console.h"

void tr_v_measure(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed);

void executeFunction(int console_tid, int marklin_tid, int reverse_tid, int clock, char *str, uint32_t last_speed[]);
// void build_sensor_str(int i, char *sensors_str, int *sensors_str_index);
void reverse();
void switchesSetup(int console_tid, int marklin_tid);

#endif