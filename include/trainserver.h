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
  TRAIN_SERVER_GO,
  TRAIN_SERVER_TRACK_CHANGE,
} train_arg_type;

typedef struct TrainServerMsgSimple{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint8_t arg3;
} TrainServerMsgSimple;

typedef struct TrainServerMsg{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint8_t arg3;
    char data[300];
} TrainServerMsg;

void trainserver();


#endif