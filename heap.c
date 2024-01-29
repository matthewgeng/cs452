#include "heap.h"
#include "rpi.h"

// MIN HEAP

void heap_init(Heap* heap, void **data, size_t capacity, int (*cmp)(const void* a, const void* b)) {
    heap->capacity = capacity;
    heap->length = 0;
    heap->data = data;
    heap->cmp = cmp;
}

void swap(Heap* heap, size_t idx_a, size_t idx_b) {
    void* tmp = heap->data[idx_a];
    heap->data[idx_a] = heap->data[idx_b];
    heap->data[idx_b] = tmp;
}

void sift_up(Heap* heap, size_t elem_idx) {
    size_t parent_idx = (elem_idx-1)/2;

    // while element is not at the top and our current element is < our parent element
    while (elem_idx > 0 && heap->cmp(heap->data[elem_idx], heap->data[parent_idx]) < 0) {
        // swap
        swap(heap, elem_idx, parent_idx);

        // update indexes
        elem_idx = parent_idx;
        parent_idx = (elem_idx-1)/2;
    }
}

void heap_push(Heap* heap, void* item) {
    if (heap->length == heap->capacity) {
        // TODO: handle somehow
        uart_printf(CONSOLE, "heap_push failed\r\n");
        return;
    }


    size_t elem_idx = heap->length;
    heap->data[elem_idx] = item;
    heap->length +=1;
    sift_up(heap, elem_idx);
}

void sift_down(Heap* heap, size_t elem_idx) {
    while (elem_idx < heap->length) {
        size_t left = elem_idx*2 + 1;
        size_t right = elem_idx*2 + 2;
        size_t smaller = elem_idx;

        // left is equal to or smaller
        if (left < heap->length && heap->cmp(heap->data[left], heap->data[smaller]) < 0) {
            smaller = left;
        }

        // right is smaller
        if (right < heap->length && heap->cmp(heap->data[right], heap->data[smaller]) < 0) {
            smaller = right;
        }

        // elements have same priority
        if (smaller == elem_idx) {
            return;
        }

        swap(heap, elem_idx, smaller);
        elem_idx = smaller;
    }
}

void* heap_pop(Heap* heap) {
    if (heap->length == 0) {
        // TODO: handle somehow
        return NULL;
    }

    void* min = heap->data[0];
    swap(heap, 0, heap->length-1);
    heap->length -=1;
    sift_down(heap, 0);
    return min;
}
