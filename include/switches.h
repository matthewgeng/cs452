#ifndef SWITCHES_H
#define SWITCHES_H

#include <stdint.h>
#include <stddef.h>

typedef struct SwitchChange {
  uint8_t switch_num;
  char dir;
} SwitchChange;

void switches_server();

#endif
