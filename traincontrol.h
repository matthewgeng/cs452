#ifndef TRAINCONTROL_H
#define TRAINCONTROL_H

#include "rpi.h"
#include "console.h"

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]);
void rv(int marklin_tid, unsigned int trainNumber, uint32_t last_speed[]);
void sw(int console_tid, int marklin_tid, unsigned int switchNumber, char switchDirection);
void executeFunction(int console_tid, int marklin_tid, char *str, uint32_t last_speed[]);
void build_sensor_str(int i, char *sensors_str, int *sensors_str_index);
void switchesSetup(int console_tid, int marklin_tid);

#endif