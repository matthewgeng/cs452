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
  TRAIN_SERVER_SWITCH_RESET
} train_arg_type;

typedef struct TrainServerMsgSimple{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
} TrainServerMsgSimple;

typedef struct TrainServerMsg{
    train_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    char data[300];
} TrainServerMsg;

typedef struct TrainState {
    uint8_t train_id;
    uint32_t last_speed;
    uint8_t train_dest;
    int train_location;
    SensorPath train_sensor_path;
    uint8_t got_sensor_path;
    uint8_t sensor_to_stop;
    int delay_time;
    uint8_t next_sensor;
    uint8_t next_sensor_err;
    uint8_t next_sensor_new;
    uint8_t last_triggered_sensor;
    uint8_t does_reset;
    int cur_train_speed; // 0 - 14
    int cur_physical_speed; // mm/s 
    uint32_t distance_between_sensors;
    uint32_t last_distance_between_sensors;
    uint32_t terminal_physical_speed; // mm/s
    uint32_t last_new_sensor_time; // us since last new sensor
    TrainSpeedState train_speed_state; // accelerating, deccelerating, constant speed, stopped?
    int sensor_query_time; 
    int predicted_next_sensor_time; // TODO; not sure if this should be set to 0
    TrainState* next;
} TrainState;

void train_states_init(TrainState* ts, uint8_t num_trains);
TrainState* getNextFreeTrainState(TrainState** ts);
void reclaimTrainState(TrainState** nextFreeTrainState, TrainState* ts);

void trainserver();

typedef enum {
    ACCELERATING,
    DECELERATING,
    CONSTANT_SPEED,
    STOPPED,
} TrainSpeedState;


#endif
