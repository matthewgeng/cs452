#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

// MIN HEAP
typedef struct Heap {
    void** data;
    size_t capacity;
    size_t length;
    int (*cmp)(const void* a, const void* b);
} Heap;

void heap_init(Heap* heap, void **data, size_t capacity, int (*cmp)(const void* a, const void* b));
void heap_push(Heap* heap, void* item);
void* heap_peek(Heap* heap);
void* heap_pop(Heap* heap);

#endif
