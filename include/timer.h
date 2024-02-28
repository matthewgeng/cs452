#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define INTERVAL 10000 // 10 ms  
// #define INTERVAL 1000000 // 1000 ms  

void timer_init();
uint32_t sys_time();
void update_c1();
void update_status_reg();

#endif
