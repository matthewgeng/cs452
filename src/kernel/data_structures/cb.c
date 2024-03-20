#include <stdarg.h>
#include <stdint.h>
#include "cb.h"
#include "util.h"

void cb_init(cb* cb, void* buffer, size_t capacity, size_t item_size, int overwrite) {
    // buffer data
    memset(buffer, 0, capacity*item_size);
    cb->buffer = buffer;
    // cast to (char*) to allow pointer arithmetic to be simpler +1 == moving 1 byte
    cb->buffer_end = (char*)cb->buffer + capacity*item_size;

    // properties
    cb->capacity = capacity;
    cb->item_size = item_size;
    cb->count = 0;

    // pointers
    cb->head = cb->buffer;
    cb->tail = cb->buffer;

    // whether adding an element when full pops out the first element
    cb->overwrite = overwrite;
}

int cb_full(cb* cb) {
    return cb->count == cb->capacity;
}

int cb_empty(cb* cb) {
    return cb->count == 0;
}

void cb_pop(cb* cb) {
    if (cb->count == 0) {
        // todo
        return;
    }

    cb->tail = (char*)cb->tail + cb->item_size;
    if (cb->tail == cb->buffer_end) {
        cb->tail = cb->buffer;
    }
    cb->count -=1;
}

void cb_push_back(cb* cb, void* item) {
    if (cb->count == cb->capacity) {
        if (cb->overwrite) {
            cb_pop(cb);
        } else {
            return;
        }
    }

    memcpy(cb->head, item, cb->item_size);

    cb->head = (char*)cb->head + cb->item_size;
    if (cb->head == cb->buffer_end) {
        cb->head = cb->buffer;
    }
    cb->count +=1;
}

void cb_pop_front(cb* cb, void* item) {
    if (cb->count == 0) {
        // todo
        return;
    }

    memcpy(item, cb->tail, cb->item_size);

    cb->tail = (char*)cb->tail + cb->item_size;
    if (cb->tail == cb->buffer_end) {
        cb->tail = cb->buffer;
    }
    cb->count -=1;
}
