#ifndef CIRCULAR_BUFFER_H
#define BUFFER_H

#include <stddef.h>

typedef struct cb {
    void* buffer;
    void* buffer_end;
    size_t capacity;
    size_t count;
    size_t item_size;
    void* head;
    void* tail;
    int overwrite;
} cb;

void cb_init(cb* cb, void* buffer, size_t capacity, size_t item_size, int overwrite);
int cb_full(cb* cb);
int cb_empty(cb* cb);
void cb_push_back(cb* cb,  void* item);
void cb_push_back_many(cb* cb,  void* items);
void cb_pop_front(cb* cb, void* item);

#endif
