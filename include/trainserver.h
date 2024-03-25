#ifndef TRAINSERVER_H
#define TRAINSERVER_H

#include <stdint.h>
#include "pathfinding.h"

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
  TRAIN_SERVER_SWITCH_RESET
} train_arg_type;

typedef struct TrainServerMsgSimple{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    // TODO: will need to add a new arg for 3 sensors
} TrainServerMsgSimple;

typedef struct TrainServerMsg{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    // TODO: will need to add a new arg for 3 sensors
    char data[350];
} TrainServerMsg;

typedef enum {
    ACCELERATING,
    DECELERATING,
    CONSTANT_SPEED,
    STOPPED,
} TrainSpeedState;

typedef struct TrainState {
    uint8_t train_id;
    uint32_t last_speed;
    uint8_t train_dest;
    int train_location;
    SensorPath train_sensor_path;
    uint8_t cur_sensor_index;
    uint8_t got_sensor_path;
    uint8_t sensor_to_stop;
    int delay_time;
    uint8_t next_nav_switch_change;
    NewSensorInfo new_sensor; // correct new data from pathfinder after a sensor trigger

    NewSensorInfo new_sensor_new; // raw new data from pathfinder after a sensor trigger
    NewSensorInfo new_sensor_err;

    int reversed;
    uint8_t last_triggered_sensor;
    uint8_t is_reversing;
    int cur_train_speed; // 0 - 14
    int cur_physical_speed; // mm/s o
    int distance_between_sensors;
    int last_distance_between_sensors;
    int minimum_moving_train_speed;
    uint32_t terminal_physical_speed; // mm/s
    int last_new_sensor_time; // us since last new sensor
    TrainSpeedState train_speed_state; // accelerating, deccelerating, constant speed, stopped?
    int sensor_query_time; 
    int predicted_next_sensor_time; // TODO; not sure if this should be set to 0
    uint32_t offset;
    uint32_t train_print_start_row;
    uint32_t train_print_start_col;
    int active;
    struct TrainState* next;
} TrainState;

void train_states_init(TrainState* ts, uint8_t num_trains);
TrainState* getNextFreeTrainState(TrainState** ts);
void reclaimTrainState(TrainState** nextFreeTrainState, TrainState* ts);

void trainserver();


#endif
