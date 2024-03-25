#include "int_cb.h"
#include "rpi.h"
#include <stdarg.h>

void initialize_intcb(IntCB *cb, int* data, size_t capacity, int override){
    cb->end = 0;
    cb->start = 0;
    cb->count = 0;
    cb->capacity = capacity;
    cb->queue = data;
    cb->override = override;
}

size_t increment_intcb(size_t capacity, size_t v){
    if(v+1==capacity){
        return 0;
    }
    return v+1;
}

size_t decrement_intcb(size_t capacity, size_t v){
    if(v==0){
        return capacity-1;
    }
    return v-1;
}

void push_intcb(IntCB *cb, int v){

    if(cb->count==cb->capacity){
        if(!(cb->override)){
            #if DEBUG
                uart_dprintf(CONSOLE, "\x1b[31msender queue out of bound\x1b[0m\r\n");
            #endif 
            return;
        }else{
            size_t next = increment_intcb(cb->capacity, cb->end);
            cb->queue[cb->end] = v;
            cb->end = next;
            cb->start = next;
        }
    }else{
        size_t next = increment_intcb(cb->capacity, cb->end);
        cb->queue[cb->end] = v;
        cb->end = next;
        cb->count++;
    }
    
}

int is_empty_intcb(IntCB *cb){
    return cb->count==0;
}

int is_full_intcb(IntCB *cb){
    return cb->count==cb->capacity;
}

int pop_intcb(IntCB *cb){
    if(is_empty_intcb(cb)){
        #if DEBUG
            uart_dprintf(CONSOLE, "\x1b[31mAttempted to pop empty sender queue\x1b[0m\r\n");
        #endif 
        return -1;
    }
    int res = cb->queue[cb->start];
    cb->start = increment_intcb(cb->capacity, cb->start);
    cb->count--;
    return res;
}

// void iter_elements_backwards(IntCB *cb, void (*function)(), ...){

//     va_list args;

//     va_start(args, function);

//     int index = decrement_intcb(cb->capacity, cb->end);
//     int c = 0;
//     int i;
//     while(c!=cb->count){
//         i = cb->queue[index];
//         function(i, args);
//         index = decrement_intcb(cb->capacity, index);
//         c += 1;
//     }

//     va_end(args);
// }
