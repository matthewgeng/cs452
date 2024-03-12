#ifndef DELAYSTOP_H
#define DELAYSTOP_H

#include <stdint.h>

typedef struct DelayStopMsg{
    uint8_t train_number;
    int delay_until;
} DelayStopMsg;

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]);
void delay_stop();


#endif