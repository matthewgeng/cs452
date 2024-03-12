#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "switches.h"

typedef struct SensorPath{
  uint8_t num_sensors;
  uint8_t sensors[80];
  uint16_t dists[80];
} SensorPath;

typedef struct HeapNode {
  uint8_t node_index;
  uint32_t dist;
  SwitchChange switches[22];
  uint8_t num_switches;
  SensorPath sensor_path;
  struct HeapNode *next;
} HeapNode;


typedef enum {
  PATH_PF,
  PATH_NAV,
  PATH_PRECOMPUTE
} path_arg_type;

typedef struct PathMessage{
    path_arg_type type; //'P' arg1 is sensor, 'T' arg1 is train
    uint8_t arg1;
    uint8_t dest;
} PathMessage;

void path_finding();

#endif