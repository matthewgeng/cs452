#include "char_cb.h"
#include "rpi.h"

void initialize_charcb(CharCB *cb, char* data, size_t capacity){
    cb->end = 0;
    cb->start = 0;
    cb->count = 0;
    cb->capacity = capacity;
    cb->queue = data;
}

size_t increment_charcb(size_t capacity, size_t v){
    if(v+1==capacity){
        return 0;
    }
    return v+1;
}

void push_charcb(CharCB *cb, char v){
    size_t next = increment_charcb(cb->capacity, cb->end);
    if(next==cb->start){
        #if DEBUG
            uart_dprintf(CONSOLE, "\x1b[31msender queue out of bound\x1b[0m\r\n");
        #endif 
        return;
    }
    cb->queue[cb->end] = v;
    cb->end = next;
    cb->count++;
}

int is_empty_charcb(CharCB* cb){
    return cb->start==cb->end;
}

char pop_charcb(CharCB *cb){
    if(is_empty_charcb(cb)){
        #if DEBUG
            uart_dprintf(CONSOLE, "\x1b[31mAttempted to pop empty sender queue\x1b[0m\r\n");
        #endif 
        return -1;
    }
    int res = cb->queue[cb->start];
    cb->start = increment_charcb(cb->capacity, cb->start);
    cb->count--;
    return res;
}
