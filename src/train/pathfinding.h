#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <stdint.h>

typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
} node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */
};

struct track_node {
  const char *name;
  node_type type;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];
};

typedef struct HeapNode {
  uint8_t node_index;
  uint32_t dist;
  uint8_t switches[20];
  uint8_t num_switches;
} HeapNode;

typedef struct PathMessage{
    uint8_t src;
    uint8_t dest;
} PathMessage;

#endif