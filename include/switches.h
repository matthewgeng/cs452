#ifndef SWITCHES_H
#define SWITCHES_H

#include <stdint.h>
#include <stddef.h>

typedef struct SwitchChange {
  uint8_t switch_num;
  char dir;
} SwitchChange;

int change_switches_cmd(int switch_tid, SwitchChange scs[], int num_switch_changes);
int get_switches_setup(int switch_tid, char switch_states[]);

void switches_server();

#endif
