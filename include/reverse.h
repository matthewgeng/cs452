#ifndef REVERSE_H
#define REVERSE_H

#include <stdint.h>

typedef struct ReverseMsg{
    uint8_t train_number;
    uint8_t last_speed;
} ReverseMsg;

void reverse();


#endif