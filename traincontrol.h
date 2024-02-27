#ifndef TRAINCONTROL_H
#define TRAINCONTROL_H

#include "rpi.h"
#include "console.h"

void executeFunction(int console_tid, int marklin_tid, int clock, char *str, uint32_t last_speed[]);
void build_sensor_str(int i, char *sensors_str, int *sensors_str_index);
void switchesSetup(int console_tid, int marklin_tid);

#endif