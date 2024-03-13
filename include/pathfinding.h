#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "switches.h"

typedef struct SensorPath{
  uint8_t num_sensors;
  uint8_t sensors[40];
  uint16_t dists[40];
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
  PATH_NEXT_SENSOR,
  PATH_SWITCH_CHANGE,
  PATH_TRACK_CHANGE,
  PATH_TRACK_NONE
} path_arg_type;

typedef struct PathMessage{
    path_arg_type type; //'P' arg1 is sensor, 'T' arg1 is train
    uint32_t arg1;
    uint8_t dest;
    char switches[22];
} PathMessage;

void path_finding();

#endif