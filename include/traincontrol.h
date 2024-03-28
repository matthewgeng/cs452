#ifndef TRAINCONTROL_H
#define TRAINCONTROL_H

#include "rpi.h"
#include "constants.h"

// void sw(int console_tid, int marklin_tid, unsigned int switchNumber, char switchDirection);
void executeFunction(int console_tid, int train_server_tid, char *str);

// void build_sensor_str(int i, char *sensors_str, int *sensors_str_index);
void reverse();

#endif
