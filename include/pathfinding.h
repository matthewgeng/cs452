#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "switches.h"

typedef struct HeapNode {
  uint8_t node_index;
  uint32_t dist;
  SwitchChange switches[22];
  uint8_t num_switches;
  uint8_t sensors[80];
  uint8_t num_sensors;
  struct HeapNode *next;
} HeapNode;

typedef struct PathMessage{
    char type; //'P' arg1 is sensor, 'T' arg1 is train
    uint8_t arg1;
    uint8_t dest;
} PathMessage;

void path_finding();

#endif