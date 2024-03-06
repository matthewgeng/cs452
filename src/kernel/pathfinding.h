#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <stdint.h>

typedef struct PathMessage{
    uint8_t src;
    uint8_t dest;
} PathMessage;

typedef struct Edge{
    Node *dest;
    uint8_t switch_setting;
    //TODO: maybe distance
} Edge;

typedef struct Node{
    uint8_t sensor_num;
    Edge *edges[];
} Node;

#endif