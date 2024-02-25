#ifndef INT_CB_H
#define INT_CB_H

#include <stddef.h>
#include "constants.h"

typedef struct IntCB{
    size_t end;
    size_t start;
    size_t capacity;
    size_t count;
    int* queue;
    int override;
} IntCB;

void initialize_intcb(IntCB *cb, int* queue, size_t capacity, int override);
void push_intcb(IntCB *cb, int v);
int pop_intcb(IntCB *cb);
int is_empty_intcb(IntCB cb);
void iter_elements_backwards(IntCB *cb, void (*function)(), ...);

#endif
