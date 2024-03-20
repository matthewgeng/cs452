#ifndef DELAYSTOP_H
#define DELAYSTOP_H

#include <stdint.h>

typedef struct DelayStopMsg{
    uint8_t train_number;
    int delay_until;
} DelayStopMsg;

void delay_stop();

#endif
