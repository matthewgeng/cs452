#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <stddef.h>
#include "constants.h"

typedef struct DelayedTask{
    int tid;
    uint32_t delay_until;
    struct DelayedTask *next;
} DelayedTask;

DelayedTask *dts_init(DelayedTask* dts, size_t size);
DelayedTask *getNextFreeDelayedTask(DelayedTask **nextFreeDelayedTask);
void reclaimDelayedTask(DelayedTask **nextFreeDelayedTask, DelayedTask *dt);

int Time(int tid);
int Delay(int tid, int ticks);
int DelayUntil(int tid, int ticks);

void clock();
void notifier();

#endif
