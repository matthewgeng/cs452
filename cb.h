#ifndef CB_H
#define CB_H

#include <stddef.h>

#define MAX_NUM_TASKS 20

typedef struct IntCB{
    size_t end;
    size_t start;
    size_t size;
    int queue[MAX_NUM_TASKS];
} IntCB;

void initialize(IntCB *cb);
void push(IntCB *cb, int v);
int pop(IntCB *cb);
int is_empty(IntCB cb);

#endif