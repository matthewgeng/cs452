#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "switches.h"

#define reverse_slow_index 5

typedef struct NewSensorInfo{
  int next_sensor;
  int next_next_sensor;
  int next_sensor_switch_err;
  int switch_after_next_sensor;
  int reverse_sensor;  
  uint8_t cur_segment_is_reserved;
  uint8_t next_segment_is_reserved;
  uint8_t next_next_segment_is_reserved;
  uint8_t exit_incoming;
} NewSensorInfo;

typedef struct SensorPath{
  uint8_t num_sensors;
  SwitchChange initial_scs[2];
  uint8_t sensors[20];
  uint16_t dists[20];
  uint8_t speeds[20];
  uint8_t does_reverse[20];
  SwitchChange scs[2][20];
} SensorPath;

typedef struct NavPath {
  uint8_t node_index;
  uint32_t dist;
  uint16_t used_segments[50];
  uint8_t num_segments;
  SensorPath sensor_path;
  struct NavPath *next;
} NavPath;


typedef enum {
  PATH_PF,
  PATH_NAV,
  PATH_NEXT_SENSOR,
  PATH_SWITCH_CHANGE,
  PATH_TRACK_CHANGE,
  PATH_NAV_END,
  PATH_SEGMENT_RESET
} path_arg_type;

typedef struct PathMessage{
    path_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t dest;
    char switches[22];
} PathMessage;

void path_finding();

#endif