#include "cb.h"
#include "rpi.h"

void initialize(IntCB *cb){
    cb->end = 0;
    cb->start = 0;
    cb->size = MAX_NUM_TASKS;
}


size_t increment(size_t size, size_t v){
    if(v+1==size){
        return 0;
    }
    return v+1;
}

void push(IntCB *cb, int v){
    size_t next = increment(cb->size, cb->end);
    if(next==cb->start){
        uart_dprintf(CONSOLE, "\x1b[31msender queue out of bound\x1b[0m\r\n");
        return;
    }
    cb->queue[next] = v;
    cb->end = next;
}

int is_empty(IntCB cb){
    return cb.start==cb.end;
}

int pop(IntCB *cb){
    if(is_empty(*cb)){
        uart_dprintf(CONSOLE, "\x1b[31mAttempted to pop empty sender queue\x1b[0m\r\n");
        return -1;
    }
    int res = cb->queue[cb->start];
    cb->start = increment(cb->size, cb->start);
    return res;
}
