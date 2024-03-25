#ifndef DELAYEXECUTE_H
#define DELAYEXECUTE_H

#include <stdint.h>

typedef struct DelayExecuteMsg{
    uint8_t train_number;
    int delay_until;
} DelayExecuteMsg;

void delay_execute1();
void delay_execute2();
void delay_execute3();
void delay_execute4();
void delay_execute5();

#endif
