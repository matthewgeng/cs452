#ifndef DELAYEXECUTE_H
#define DELAYEXECUTE_H

#include <stdint.h>

typedef enum {
  DELAY_STOP,
  DELAY_RV,
  DELAY_RV_STOP
} delay_exe_argtype;


typedef struct DelayExecuteMsg{
    delay_exe_argtype type;
    int delay;
    int stop_delay;
    uint8_t train_number;
    uint8_t last_speed;
} DelayExecuteMsg;

int delay_stop(int tid, uint8_t train_id, int delay);
int delay_rv(int tid, uint8_t train_id, int delay, uint8_t last_speed);
int delay_rv_stop(int tid, uint8_t train_id, int delay, uint8_t last_speed, int stop_delay);

void delay_execute1();
void delay_execute2();
void delay_execute3();
void delay_execute4();
void delay_execute5();

#endif
