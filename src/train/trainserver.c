#include <stddef.h>
#include "trainserver.h"
#include "syscall.h"
#include "rpi.h"
#include "reverse.h"
#include "switches.h"
#include "pathfinding.h"
#include "delaystop.h"
#include "util.h"
#include "timer.h"
#include "trainconstants.h"
#include "constants.h"
#include "io.h"
#include "cb.h"
#include "sensors.h"

void tr(int marklin_tid, TrainState* ts){
  char cmd[3];
  cmd[0] = ts->cur_train_speed;
  cmd[1] = ts->train_id;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  ts->last_speed = ts->cur_train_speed;
}


void loc_err_handling(int train_location, NewSensorInfo *new_sensor, NewSensorInfo *new_sensor_err, NewSensorInfo *new_sensor_new, uint8_t *last_triggered_sensor){
  if(new_sensor->next_sensor==255 || train_location==new_sensor->next_sensor){
    memcpy(new_sensor, new_sensor_new, sizeof(NewSensorInfo));
    new_sensor_err->next_sensor = 255;
    *last_triggered_sensor = train_location;
  }else if(train_location==new_sensor->next_next_sensor || train_location==new_sensor->next_sensor_switch_err){
    memcpy(new_sensor_err, new_sensor_new, sizeof(NewSensorInfo));
  }else if(train_location==new_sensor_err->next_sensor){
    memcpy(new_sensor, new_sensor_new, sizeof(NewSensorInfo));
    new_sensor_err->next_sensor = 255;
    *last_triggered_sensor = train_location;
  }

}

int calculate_new_current_speed(TrainSpeedState* train_speed_state, int old_speed, int terminal_speed, uint32_t distance_between, uint32_t ticks, uint32_t offset) {

    // mm / 10ms --> mm/ms --> *1000 = mm/ms /10 --> *100

    int average_new_speed = (distance_between*100) / ticks; // millimeters per second
    int new_cur_speed = 0;
    // uart_printf(CONSOLE, "\0337\033[17;1H\033[K old state: %d, old speed: %d, new average speed: %d , terminal speed: %d \0338", *train_speed_state, old_speed, average_new_speed, terminal_speed);

    switch (*train_speed_state) {
        case ACCELERATING:

            if (average_new_speed >= terminal_speed) {
                new_cur_speed = average_new_speed;
                *train_speed_state = CONSTANT_SPEED;
            } else {

                // TODO: weird case due to error or something
                if (old_speed > average_new_speed) {
                    new_cur_speed = old_speed;
                    // new_cur_speed = average_new_speed;
                } else {
                    // normal case
                    new_cur_speed = average_new_speed + (average_new_speed - old_speed);
                }
                
            }

            break;
        case DECELERATING:
            if (terminal_speed == 0 && average_new_speed == old_speed) {
                *train_speed_state = STOPPED;
            } else if (average_new_speed <= terminal_speed || old_speed <= terminal_speed) {
                *train_speed_state = CONSTANT_SPEED;
            } else {
                // TODO: weird case when average speed is greater than the current speed due to some error/turn idk
                if (old_speed < average_new_speed) {
                    new_cur_speed = old_speed;
                    // new_cur_speed = average_new_speed;
                } else {
                    // normal case
                    new_cur_speed = average_new_speed - (old_speed - average_new_speed);
                }
            }

            break;
        case CONSTANT_SPEED:
            new_cur_speed = average_new_speed;
            break;

        case STOPPED:
            // TODO: handle this state
            return 0;

        default:
            // TODO: error
            break;
    }

    // alpha = 0.5
    // old_speed = old_speed*(1-alpha) + new_cur_speed*(alpha)
    int new_speed = (old_speed - (old_speed >> 1)) + (new_cur_speed >> 1);
    // new_speed -= 50;
    // if (*train_speed_state == CONSTANT_SPEED) {
        // new_speed = new_speed * offset / 100;
    // }
    // uart_printf(CONSOLE, "\0337\033[30;1H\033[Knew speed state: %d, avg speed: %u, new speed: %u\0338", train_speed_state, average_new_speed, new_cur_speed);
    return new_speed;
}

void print_estimation(int cout, int sensor_query_time, TrainState* ts){
    int res = (sensor_query_time*10-ts->predicted_next_sensor_time) * (ts->last_distance_between_sensors)/(sensor_query_time*10 - ts->last_new_sensor_time*10);

    new_printf(cout, 0, "\0337\033[%d;%dHActual-prediction(ms): %d-%d = %d        \0338", 5 + ts->train_print_start_row, ts->train_print_start_col,
        sensor_query_time*10, ts->predicted_next_sensor_time, sensor_query_time*10 - ts->predicted_next_sensor_time
    );

    new_printf(cout, 0, "\0337\033[%d;%dHDistance diff (mm):%d        \0338", 1 + 5 + ts->train_print_start_row, ts->train_print_start_col, res);
}

void print_sensor(int cout, uint32_t sensor){
    char s1[] = "\0337\033[21;1H\033[K   Triggered sensor:                    \0338";
    ui2a_no0(sensor, 10, s1+33);
    Puts(cout, 0, s1);
    // uart_printf(CONSOLE, "\0337\033[21;1H\033[KTriggered sensor: %u\0338", sensor);
}

// void print_sensor_and_prediction(int cout, uint32_t sensor, uint8_t next_sensor, int sensor_query_time, int predicted_next_sensor_time){
void print_sensor_and_prediction(int cout, uint32_t sensor, int sensor_query_time, TrainState* ts){
    // char s1[] = "\0337\033[21;1H\033[K   Triggered sensor:                                      Next sensor:            \0338";
    // char s2[] = "\0337\033[22;1H\033[K                                     Predicted next trigger time (ms):         \0338";
    // ui2a_no0(sensor, 10, s1+33); 
    // ui2a_no0(next_sensor, 10, s1+84);
    // Puts(cout, 0, s1);

    // i2a_no0(predicted_next_sensor_time, s2+84);
    // Puts(cout, 0, s2);

    new_printf(cout, 0, "\0337\033[%d;%dHTriggered sensor:%d Next sensor:%d    \0338", 3 + ts->train_print_start_row, ts->train_print_start_col,
        sensor, ts->new_sensor_new.next_sensor
    );
    new_printf(cout, 0, "\0337\033[%d;%dHPredicted next trigger time (ms):%d    \0338", 1 + 3 + ts->train_print_start_row, ts->train_print_start_col,
        ts->predicted_next_sensor_time
    );
}

void train_states_init(TrainState* trains, uint8_t num_trains) {
    for (int i = 0; i < num_trains; i++) {
        TrainState* ts  = trains + i;

        ts->train_id;
        ts->last_speed = 0;
        ts->train_dest = 255;
        ts->train_location = -1;
        ts->train_sensor_path;
        ts->cur_sensor_index = 0;
        ts->got_sensor_path = 0;
        ts->sensor_to_stop = 25;;
        ts->delay_time;
        ts->new_sensor_new;

        ts->new_sensor;
        ts->new_sensor.next_sensor = 255;

        ts->new_sensor_err;
        ts->new_sensor_err.next_sensor = 255;

        ts->reversed = 0;
        ts->last_triggered_sensor = 255;
        ts->does_reset = 0;
        ts->cur_train_speed = 0;
        ts->cur_physical_speed = 0;
        ts->distance_between_sensors = 0;
        ts->last_distance_between_sensors = 0;
        ts->terminal_physical_speed = 0;
        ts->last_new_sensor_time = 0;
        ts->train_speed_state = STOPPED;
        ts->sensor_query_time = -1;
        ts->predicted_next_sensor_time = 0;
        ts->offset = 100; 
        ts->train_print_start_row = 0;
        ts->next = NULL;
        if (i + 1 < num_trains) {
            ts->next = trains + i + 1;
        }
    }
}

TrainState* getNextFreeTrainState(TrainState** ts) {
    if(*ts==NULL){
        return NULL;
    }
    TrainState *free = *ts;
    *ts = (*ts)->next;
    return free;
}

void reclaimTrainState(TrainState** nextFreeTrainState, TrainState* ts) {
    ts->next = *nextFreeTrainState;
    *nextFreeTrainState = ts;
}

int isExpectedSensor(TrainState* ts, int sensor) {
    uint8_t expected_normal_sensor = sensor==ts->new_sensor.next_sensor ||
                                    sensor==ts->new_sensor.next_next_sensor || sensor == ts->new_sensor.next_sensor_switch_err;

    uint8_t expected_alt_sensor = ts->new_sensor_err.next_sensor!=255 && sensor==ts->new_sensor_err.next_sensor;

    return expected_normal_sensor || expected_alt_sensor;
}

TrainState* getTrainState(char track, uint32_t train_id, TrainState** current_trains,  TrainState** next_free_train_state, int* num_available_trains) {

    // not a current train and no more trains available --> continue
    if (train_id > 100 || current_trains[train_id] == NULL && *next_free_train_state == NULL) {
        return NULL;
    }

    if (current_trains[train_id] != NULL) {
        return current_trains[train_id];
    } else {
        TrainState* ts = getNextFreeTrainState(next_free_train_state);
        // above will never be null since we do a null check before 

        ts->train_id = train_id;
        ts->train_location = starting_sensor_for_train(track, train_id);
        ts->new_sensor.next_sensor = starting_next_sensor_for_train(track, train_id);
        // handling the case in which the train (some of the starting positions on track b), trips the starting location
        ts->new_sensor_err.next_sensor = ts->train_location;
        current_trains[train_id] = ts;
        /*
        coords for train printing:
            train 1 train2 
            train 3 train 4 

        0 --> 0, 0
        1 --> 0,1 
        2 --> 1,0
        3 --> 1,1

        train_index = (MAX_NUM_TRAINS - num_available_trains)
        row = 20 * (train_index/2 + 1)
        col = 30 * train_index%2

            0 --> 20, 0
            1 --> 20, 30
            2 --> 40,0
            3 --> 40,30
        */
        int train_index = MAX_NUM_TRAINS - *num_available_trains;
        ts->train_print_start_row = 20 * (train_index/2 + 1);
        ts->train_print_start_col = 50*(train_index%2);
        *num_available_trains = *num_available_trains -1;
        return ts;
    }
}

char get_sensor_letter(int sensor) {
    if (sensor == -1) {
        return 0;
    }

    return 'A' + sensor/16;
}

int get_sensor_digit(int sensor) {
    if (sensor == -1) {
        return -1;
    }

    return sensor%16 + 1;
}

void print_starting_train_locations(int cout, char track, int* valid_trains, int num_trains) {

    // should start printing on line 9, from tasks.c
    int row = 9;
    int i = 0;
    while (i < num_trains) {
        int tr1 = valid_trains[i];
        int sensor_tr1 = starting_sensor_for_train(track, tr1);
        char sensor_letter_tr1 = get_sensor_letter(sensor_tr1);
        int sensor_digit_tr1 = get_sensor_digit(sensor_tr1);

        // at least 2 trains remaining
        if (num_trains - i >= 2) {

            int tr2 = valid_trains[i+1];
            int sensor_tr2 = starting_sensor_for_train(track, tr2);
            char sensor_letter_tr2 = get_sensor_letter(sensor_tr2);
            int sensor_digit_tr2 = get_sensor_digit(sensor_tr2);

            new_printf(cout, 0, "\0337\033[%d;1H\033[KTrain %d-->%c%d    Train %d-->%c%d    \0338", row, 
                tr1, sensor_letter_tr1, sensor_digit_tr1,
                tr2, sensor_letter_tr2, sensor_digit_tr2
            );
            i +=2;
        } else {
            new_printf(cout, 0, "\0337\033[%d;1H\033[KTrain %d-->%c%d    \0338", row, 
                tr1, sensor_letter_tr1, sensor_digit_tr1
            );
            i +=1;
        }

        row +=1;
    }
}

int will_collide(char track, TrainState* ts1, TrainState* ts2) {
    int location_sensor_distance_diff = sensor_distance_between(track, ts1->train_location, ts2->train_location);
    int predicted_sensor_distance_diff = sensor_distance_between(track, ts1->new_sensor_new.next_sensor, ts2->new_sensor_new.next_sensor);

    // head on collision
    if (ts1->reversed != ts2->reversed) {
        // TODO:

    // same direction collision
    } else {
        // find the sensor where a collision will occur
        int collision_sensor = -1;

        if (ts1->new_sensor_new.next_next_sensor == ts2->new_sensor_new.next_next_sensor || 
            ts1->new_sensor_new.next_next_sensor == ts2->new_sensor_new.next_sensor
        ) {
            collision_sensor = ts1->new_sensor_new.next_next_sensor;

        } else if (ts1->new_sensor_new.next_sensor == ts2->new_sensor_new.next_next_sensor || 
            ts1->new_sensor_new.next_sensor == ts2->new_sensor_new.next_sensor
        ) {
            collision_sensor = ts1->new_sensor_new.next_sensor;
        } else if (ts1->new_sensor_new.next_sensor == ts2->train_location || 
            ts1->new_sensor_new.next_sensor == ts2->train_location
        ) {
            collision_sensor = ts1->new_sensor_new.next_sensor;
        } else if (ts1->train_location == ts2->new_sensor_new.next_next_sensor || 
            ts1->train_location == ts2->new_sensor_new.next_sensor
        ) {
            collision_sensor = ts1->train_location;
        } else if (ts1->train_location == ts2->train_location) {
            collision_sensor = ts1->train_location;
        }
    }

    return 0;
}

void trainserver(){
    RegisterAs("trainserver\0");
    int mio = WhoIs("mio\0");
    int clock = WhoIs("clock\0");
    int cout = WhoIs("cout\0");
    int sensor_tid = WhoIs("sensor\0");
    int pathfind_tid = WhoIs("pathfind\0");
    int reverse_tid = WhoIs("reverse\0");
    int switch_tid = WhoIs("switch\0");
    int delay_stop_tid = WhoIs("delaystop\0");

    int tid;
    TrainServerMsg tsm;
    int msg_len;
    int intended_reply_len;

    ReverseMsg rm;
    PathMessage pm;
    SwitchChange scs[2];
    DelayStopMsg dsm;
    char track = 'a';


    // train state data
    TrainState train_states[MAX_NUM_TRAINS];
    train_states_init(&train_states, MAX_NUM_TRAINS);
    TrainState* next_free_train_state = train_states;
    int num_available_trains = MAX_NUM_TRAINS;

    // train accessor hashmap + array
    TrainState* trains[100] = {NULL};

    int valid_trains[] = VALID_TRAINS;
    int num_valid_trains = sizeof(valid_trains)/sizeof(valid_trains[0]);
    print_starting_train_locations(cout, track, valid_trains, num_valid_trains);

  for(;;){
    msg_len = Receive(&tid, &tsm, sizeof(TrainServerMsg));

    if(tsm.type==TRAIN_SERVER_NEW_SENSOR){
        uint32_t sensor_query_time = tsm.arg3;
        Reply(tid, NULL, 0);
        int sensors[MAX_NUM_TRIGGERED_SENSORS] = {255};
        sensors[0] = tsm.arg1;
        sensors[1] = tsm.arg2;
        new_printf(cout, 0, "\0337\033[50;1H\033[K sensor 1: %d\0338", sensors[0]);
        new_printf(cout, 0, "\0337\033[51;1H\033[K sensor 2: %d\0338", sensors[1]);

        // POTENTIALLY MULTIPLE SENSORS TRIGGERED, so we iterate and handle each sensor 
        for (int s = 0; s < MAX_NUM_TRIGGERED_SENSORS; s++) {
            int sensor = sensors[s];
            if (sensor == 255) {
                continue;
            }

            // POTENTIALLY MULTIPLE TRAINS, must attempt to handle sensor for each train (sensor attribution)
            for (int t = 0; t < sizeof(trains)/sizeof(trains[0]); t++) {
                TrainState* ts = trains[t];
                if (trains[t] == NULL) {
                    continue;
                }

                uint8_t expected_sensor = isExpectedSensor(ts, sensor);
                new_printf(cout, 0, "\0337\033[%d;1H\033[K sensor trigger %d, train %d, next: %d, nextnext: %d, alt next: %d, alt nextnext: %d\0338",
                                20 + (ts->train_print_start_col/4) + ts->train_print_start_row,
                                sensor, ts->train_id, ts->new_sensor.next_sensor, ts->new_sensor.next_next_sensor, ts->new_sensor_err.next_sensor, ts->new_sensor_err.next_next_sensor);
                
                if (!expected_sensor) {
                    continue;
                }

                // COLLISION AVOIDANCE, new_sensor_new was just set from previous loop at the bottom
                // i.e new_sensor and new_senor are also correct (dervied from new_sensor_new) but new_sensor_new is the truth value
                    // case 1: 
                        // --> <--
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->train_location || 
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->new_sensor_new.next_sensor || 
                                // tr id 15 for these trains
                    // case 2:
                        // --> \
                        // --> /
                    // case 3:
                        // / <--
                        // \ <--
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->new_sensor_new.next_sensor || 
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->new_sensor_new.next_next_sensor || 
                            // ts->new_sensor_new.next_next_sensor == nested_loop_ts->new_sensor_new.next_sensor
                            // ts->new_sensor_new.next_next_sensor == nested_loop_ts->new_sensor_new.next_next_sensor
                    // case 4:
                        // ---> ->
                    // case 5:
                        // <- <--
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->new_sensor_new.next_sensor || 
                            // ts->new_sensor_new.next_sensor == nested_loop_ts->new_sensor_new.next_next_sensor || 
                            // ts->new_sensor_new.next_next_sensor == nested_loop_ts->new_sensor_new.next_sensor
                            // ts->new_sensor_new.next_next_sensor == nested_loop_ts->new_sensor_new.next_next_sensor

                // SENSOR PROCESSING
                if(ts->last_triggered_sensor!=sensor) {
                    
                        ts->train_location = sensor;

                        // TODO: since all trains have starting locations now, we shouldn't need this check
                        // in fact we maybe could initialize last_triggered to be the starting point
                        
                        if (ts->last_triggered_sensor != 255) {
                            
                            ts->distance_between_sensors = sensor_distance_between(track, ts->last_triggered_sensor, sensor); // train_location <--> tsm.arg1 in millimeters
                            if (ts->distance_between_sensors == -1) {
                                
                            }else {
                                // get time delta
                                uint32_t delta_new = sensor_query_time - ts->last_new_sensor_time; // ticks
                                ts->cur_physical_speed = calculate_new_current_speed(&(ts->train_speed_state), ts->cur_physical_speed, ts->terminal_physical_speed, ts->distance_between_sensors, delta_new, ts->offset);
                            }
                        }

                        new_printf(cout, 0, "\0337\033[%d;%dHTrain %d, terminal speed %d    \0338", ts->train_print_start_row,  ts->train_print_start_col, ts->train_id, ts->terminal_physical_speed);
                        new_printf(cout, 0, "\0337\033[%d;%dHSpeed state %d, train speed %d, speed %d     \0338", 1 + ts->train_print_start_row, ts->train_print_start_col, ts->train_speed_state, ts->cur_train_speed, ts->cur_physical_speed);

                        if(ts->train_dest!=255 && ts->got_sensor_path){

                            uint32_t stopping_acceleration = train_stopping_acceleration(ts->train_id, ts->cur_train_speed) * ts->offset / 100; //mm/s^2

                            uint32_t stopping_distance = (ts->cur_physical_speed*ts->cur_physical_speed)/(2*stopping_acceleration); // mm
                            int last_index = ts->train_sensor_path.num_sensors-1;
                            for (int i = last_index; i >= 0; i--) {
                                if (ts->train_sensor_path.dists[last_index] - ts->train_sensor_path.dists[i] > stopping_distance) {

                                    ts->sensor_to_stop = ts->train_sensor_path.sensors[i];
                                    uint32_t stopping_distance_difference = (ts->train_sensor_path.dists[ts->train_sensor_path.num_sensors-1] - ts->train_sensor_path.dists[i]) - stopping_distance;

                                    ts->delay_time = (stopping_distance_difference*100)/ts->cur_physical_speed; // 10ms for system ticks
                                    new_printf(cout, 0, "\0337\033[%d;%dHLast dist - cur: %d - %d = %d, delay_time %d     \0338", 8 + ts->train_print_start_row,  ts->train_print_start_col, 
                                        ts->train_sensor_path.dists[ts->train_sensor_path.num_sensors-1], ts->train_sensor_path.dists[i], ts->train_sensor_path.dists[ts->train_sensor_path.num_sensors-1] - ts->train_sensor_path.dists[i], ts->delay_time
                                    );

                                    // new_printf(cout, 0, "\0337\033[60;1H\033[KTrain sensor path dist %d, delay_time %d, last index %d, cur dist %d, diff %d \0338", ts->train_sensor_path.dists[i], ts->delay_time, ts->train_sensor_path.dists[ts->train_sensor_path.num_sensors-1], ts->train_sensor_path.dists[i], ts->train_sensor_path.dists[ts->train_sensor_path.num_sensors-1] - ts->train_sensor_path.dists[i]);
                                    // uart_printf(CONSOLE, "\0337\033[60;1H\033[KTrain sensor path dist %d, delay_time %d, last index %d, cur dist %d, diff %d \0338", train_sensor_path.dists[i], delay_time, train_sensor_path.dists[train_sensor_path.num_sensors-1], train_sensor_path.dists[i], train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]);
                                    
                                    break;
                                }
                            }

                            // new_printf(cout, 0, "\0337\033[61;1H\033[KEstimated stopping distance %d, sensor to stop %d \0338", stopping_distance, ts->sensor_to_stop);

                            if(ts->train_sensor_path.scs[0][ts->cur_sensor_index].switch_num!=255){
                                // TODO: should scs be per train?
                                memcpy(scs, ts->train_sensor_path.scs[0] + ts->cur_sensor_index, sizeof(SwitchChange));
                                int num_scs = 1;
                                if(ts->train_sensor_path.scs[1][ts->cur_sensor_index].switch_num!=255){
                                    num_scs = 2;
                                    memcpy(scs+1, ts->train_sensor_path.scs[1] + ts->cur_sensor_index, sizeof(SwitchChange));
                                }
                                
                                // new_printf(cout, 0, "\0337\033[67;1H\033[Ksensor: %u, switch to change: %u %u, num_scs: %u\0338", ts->train_sensor_path.sensors[ts->cur_sensor_index], scs[0].switch_num, scs[0].dir, num_scs);

                                int res = change_switches_cmd(switch_tid, scs, num_scs);
                                if(res<0){
                                    // uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
                                }
                            }
                            
                            ts->cur_sensor_index += 1;
                        }

                        // TODO: what if distance was invalid i.e. invalid sensor reading

                        if(ts->train_location==ts->sensor_to_stop){
                            ts->last_speed = 0;
                            dsm.train_number = ts->train_id;
                            dsm.delay_until = (int)(tsm.arg2) + ts->delay_time;
                            intended_reply_len = Send(delay_stop_tid, &dsm, sizeof(DelayStopMsg), NULL, 0);
                            if(intended_reply_len!=0){
                                // new_printf(cout, 0, "\0337\033[30;1H\033[Ktrainserver delay stop unexpected reply\0338");
                            }
                            ts->train_dest = 255;
                            ts->sensor_to_stop = 255;
                            ts->got_sensor_path = 0;
                        }



                    pm.type = PATH_NEXT_SENSOR;
                    pm.arg1 = sensor;
                    intended_reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t), &(ts->new_sensor_new), sizeof(NewSensorInfo));
                    
                    if(intended_reply_len!=sizeof(NewSensorInfo)) {
                        // uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver get next sensor unexpected reply\0338");
                        continue;
                    }
                    
                    if(expected_sensor && ts->predicted_next_sensor_time!=0){
                        // print_estimation(cout, sensor_query_time, ts->last_new_sensor_time, ts->last_distance_between_sensors, ts->predicted_next_sensor_time, ts->cur_physical_speed);
                        print_estimation(cout, sensor_query_time, ts);
                    }

                    if(!expected_sensor){
                        print_sensor(cout, sensor);
                    }else if(ts->new_sensor_new.next_sensor == -2){
                        ts->predicted_next_sensor_time = 0;
                        // Puts(cout, 0, "\0337\033[50;1H\033[Knext sensor query failed\0338");
                        print_sensor(cout, sensor);
                    } else{
                        // uart_printf(CONSOLE, "\0337\033[50;1H\033[Kprints:\0338");
                        int next_sensor_distance = sensor_distance_between(track, sensor, ts->new_sensor_new.next_sensor);
                        ts->predicted_next_sensor_time = next_sensor_distance*1000/ts->cur_physical_speed + sensor_query_time*10;
                        // Puts(cout, 0, "\0337\033[30;1H\033[K\0338");
                        // print_sensor_and_prediction(cout, sensor, ts->new_sensor_new.next_sensor, sensor_query_time, ts->predicted_next_sensor_time);
                        print_sensor_and_prediction(cout, sensor, sensor_query_time, ts);
                    }
                    
                    // new_printf(cout, 0, "\0337\033[65;1H\033[Knext: %u, next next: %u, switch err: %u, switch: %u\0338", ts->new_sensor_new.next_sensor, ts->new_sensor_new.next_next_sensor, ts->new_sensor_new.next_sensor_switch_err, ts->new_sensor_new.switch_after_next_sensor);
                    loc_err_handling(sensor, &(ts->new_sensor), &(ts->new_sensor_err), &(ts->new_sensor_new), &(ts->last_triggered_sensor));
                    // new_printf(cout, 0,  "\0337\033[66;1H\033[Knew_sensor: %u, new_sensor_err: %u, last trig: %u\0338", ts->new_sensor.next_sensor, ts->new_sensor_err.next_sensor, ts->last_triggered_sensor);

                    
                    ts->last_new_sensor_time = sensor_query_time;
                    ts->last_distance_between_sensors = ts->distance_between_sensors;

                    ts->last_triggered_sensor = sensor;
                }
            }
        }
    } else if(tsm.type==TRAIN_SERVER_TR){
        Reply(tid, NULL, 0);

        TrainState* ts = getTrainState(track, tsm.arg1, trains, &next_free_train_state, &num_available_trains);
        if (ts == NULL) {
            // TODO: show error
            continue;
        }

        ts->cur_train_speed = tsm.arg2;
        ts->offset = train_velocity_offset(tsm.arg1, tsm.arg2);
        ts->terminal_physical_speed = train_terminal_speed(tsm.arg1, tsm.arg2);

        // set train state
        // other states should be managed by sensor data processing to avoid errors when we go from 0 to 14, then 
        // before acceleration is done, we set the train speed to 14 again (should not be a constant_speed state)
        if (ts->last_speed < tsm.arg2) {
            ts->train_speed_state = ACCELERATING;
        } else if (ts->last_speed> tsm.arg2) {
            ts->train_speed_state = DECELERATING;
        }
        uart_printf(CONSOLE, "\0337\033[45;1H\033[K tr id: %d, tr speed: %d \0338", ts->train_id, ts->cur_train_speed);

        tr(mio, ts);
    }else if(tsm.type==TRAIN_SERVER_RV && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        TrainState* ts = getTrainState(track, tsm.arg1, trains, &next_free_train_state, &num_available_trains);
        if (ts == NULL) {
            // TODO: show error
            continue;
        }
        rm.train_number = tsm.arg1;
        rm.last_speed = ts->last_speed;
        intended_reply_len = Send(reverse_tid, &rm, sizeof(ReverseMsg), NULL, 0);
        if(intended_reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
        }
        ts->reversed = !ts->reversed;
    }else if(tsm.type==TRAIN_SERVER_SW && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        scs[0].switch_num = tsm.arg1;
        scs[0].dir = (char)tsm.arg2;
        int res = change_switches_cmd(switch_tid, scs, 1);
        if(res<0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_SWITCH_RESET && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        int res = reset_switches(switch_tid);
        if(res<0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_PF && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        pm.type = PATH_PF;
        pm.arg1 = tsm.arg1;
        pm.dest = tsm.arg2;
        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver pf cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_NAV && msg_len==sizeof(TrainServerMsgSimple)){
        // Reply(tid, NULL, 0);
        // if(tsm.arg1 != train_id){
        //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Knav unexpected train number\0338");
        //     continue;
        // }
        // if(tsm.arg3!=0){
        //     offset = tsm.arg3;
        // }
        // // offset = tsm.arg3;
        // train_dest = tsm.arg2;
        // pm.type = PATH_NAV;
        // pm.arg1 = train_location;
        // pm.dest = tsm.arg2;
        // // uart_printf(CONSOLE, "\0337\033[54;1H\033[Ktrain server before send %d\0338", Time(clock));
        // int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        // if(reply_len!=0){
        //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
        // }
        // got_sensor_path = 0;

        // uart_printf(CONSOLE, "\0337\033[34;1H\033[Koffset %d\0338", offset);
    }else if(tsm.type==TRAIN_SERVER_GO && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);

        TrainState* ts = getTrainState(track, tsm.arg1, trains, &next_free_train_state, &num_available_trains);
        if (ts == NULL) {
            // TODO: show error
            continue;
        }
        
        ts->cur_train_speed = tsm.arg2;
        ts->offset = train_velocity_offset(tsm.arg1, tsm.arg3);
        ts->terminal_physical_speed = train_terminal_speed(tsm.arg1, tsm.arg3);
        if (ts->last_speed < tsm.arg3) {
            ts->train_speed_state = ACCELERATING;
        } else if (ts->last_speed> tsm.arg3) {
            ts->train_speed_state = DECELERATING;
        }

        tr(mio, ts);
        ts->train_dest = tsm.arg2;

        pm.type = PATH_NAV;
        pm.arg1 = ts->train_location;
        pm.dest = tsm.arg2;
        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
        }
        ts->got_sensor_path = 0;
    }else if(tsm.type==TRAIN_SERVER_TRACK_CHANGE && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        track = tsm.arg1;
        pm.type = PATH_TRACK_CHANGE;
        pm.arg1 = track;
        print_starting_train_locations(cout, track, valid_trains, num_valid_trains);

        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver tarck cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_NAV_PATH && msg_len==sizeof(TrainServerMsg)){
        //TODO: maybe this should be a reply?
        // Reply(tid, NULL, 0);
        // memcpy(&train_sensor_path, tsm.data, sizeof(SensorPath));
        // // SensorPath *sp = tsm.data;

        // cur_sensor_index = 0;
        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K initial sc: %u %u\0338", 39, train_sensor_path.initial_scs[0].switch_num, train_sensor_path.initial_scs[0].dir);
        // for(int i = 0; i<train_sensor_path.num_sensors; i++){
        //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u, dist: %u, reverse: %u, switch: %u %u\0338", 40+i, train_sensor_path.sensors[i], train_sensor_path.dists[i], train_sensor_path.does_reverse[i], train_sensor_path.scs[0][i].switch_num, train_sensor_path.scs[0][i].dir);
        // }
        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K num_switches: %u \0338", 30, train_nav_path.num_switches );
        // for(int i = 0; i<train_nav_path.num_switches; i++){
        //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K switch: %u, dir: %u\0338", 31+i, train_nav_path.switches[i].switch_num, train_nav_path.switches[i].dir );
        // }

        // if first switch is upcoming, change it now
        // if(train_nav_path.num_switches>0 && train_nav_path.switches[0].switch_num==new_sensor.switch_after_next_sensor){
        //     sc.switch_num = train_nav_path.switches[0].switch_num;
        //     sc.dir = train_nav_path.switches[0].dir;
        //     int res = change_switches_cmd(switch_tid, &sc, 1);
        //     if(res<0){
        //         uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
        //     }
        //     next_nav_switch_change = 1;
        // }else{
        //     next_nav_switch_change = 0;
        // }
        // if(train_sensor_path.initial_scs[0].switch_num!=255){
        //     memcpy(scs, train_sensor_path.initial_scs, sizeof(SwitchChange));
        //     int num_scs = 1;
        //     if(train_sensor_path.initial_scs[1].switch_num!=255){
        //         num_scs = 2;
        //         memcpy(scs+1, train_sensor_path.initial_scs+1, sizeof(SwitchChange));
        //     }
        //     int res = change_switches_cmd(switch_tid, scs, num_scs);
        //     if(res<0){
        //         uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
        //     }
        // }

        /* 
        TODO: 

        calculate stop distance using velocity
        total_dist = train_sensor_path.sensor_path.dists[train_sensor_path.num_sensors-1];
        if(total_dist<stop_distance){
            tr(train_id, 0);
        }
        */

        // got_sensor_path = 1;
        // uart_printf(CONSOLE, "\0337\033[70;1H\033[Kcheck nav path %u %u %u\0338", train_sensor_path.num_sensors, train_sensor_path.sensors[0],  train_sensor_path.sensors[train_sensor_path.num_sensors-1]);
        // E6 14 32 69
        // sensor_to_stop = -1;
    }else{
        Reply(tid, NULL, 0);
        uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver unknown cmd %d %u\0338", tsm.type, msg_len);
    }
  }
}
