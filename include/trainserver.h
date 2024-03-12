#ifndef TRAINSERVER_H
#define TRAINSERVER_H

#include <stdint.h>

typedef enum {
  TRAIN_SERVER_NONE,
  TRAIN_SERVER_TR,
  TRAIN_SERVER_RV,
  TRAIN_SERVER_SW,
  TRAIN_SERVER_PF,
  TRAIN_SERVER_NAV,
  TRAIN_SERVER_NEW_SENSOR,
  TRAIN_SERVER_NAV_PATH,
} train_arg_type;

typedef struct TrainServerMsgSimple{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
} TrainServerMsgSimple;

typedef struct TrainServerMsg{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    char data[300];
} TrainServerMsg;

void trainserver();

typedef enum {
    ACCELERATING,
    DECELERATING,
    CONSTANT_SPEED,
    STOPPED,
} TrainSpeedState;


#endif
