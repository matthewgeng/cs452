#ifndef CHAR_CB_H
#define CHAR_CB_H

#include <stddef.h>
#include "constants.h"

typedef struct CharCB{
    size_t end;
    size_t start;
    size_t capacity;
    size_t count;
    int override;
    char* queue;
} CharCB;

void initialize_charcb(CharCB *cb, char* queue, size_t capacity, int override);
void push_charcb(CharCB *cb, char v);
char peek_charcb(CharCB *cb);
char pop_charcb(CharCB *cb);
int is_empty_charcb(CharCB* cb);

#endif
