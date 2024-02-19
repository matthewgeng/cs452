#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

extern uint64_t* p_idle_task_total;
extern uint64_t* p_program_start;


void idle_task();
void time_user();
void rootTask();

#endif