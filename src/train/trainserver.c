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

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]){
  char cmd[3];
  cmd[0] = trainSpeed;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  last_speed[trainNumber] = trainSpeed;
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

void print_estimation(int cout, int sensor_query_time, int last_new_sensor_time, uint32_t last_distance_between_sensors, int predicted_next_sensor_time, int cur_physical_speed){
// void print_estimation(int cout, int sensor_query_time, int last_new_sensor_time, uint32_t last_distance_between_sensors, int predicted_next_sensor_time){

    char s1[] = "\0337\033[23;1H\033[KTriggered time (ms):                              Predicted time (ms):               \0338";
    char s2[] = "\0337\033[24;1H\033[K     Time diff (ms):                               Distance diff (mm):          \0338";

    i2a_no0(sensor_query_time*10, s1+33);
    i2a_no0(predicted_next_sensor_time, s1+84);
    Puts(cout, 0, s1);

    i2a_no0(sensor_query_time*10-predicted_next_sensor_time, s2+33);
    // i2a_no0((sensor_query_time*10-predicted_next_sensor_time)*(cur_physical_spmakeeed)/1000, s2+84);
    int res = (sensor_query_time*10-predicted_next_sensor_time)*(int)(last_distance_between_sensors)/(sensor_query_time*10 - last_new_sensor_time*10);
    i2a_no0(res, s2+84);
    // i2a_no0((sensor_query_time*10-predicted_next_sensor_time)*(last_distance_between_sensors/(sensor_query_time*10 - last_new_sensor_time*10)), s2+84);
    Puts(cout, 0, s2);
    // uart_printf(CONSOLE, "\0337\033[22;1H\033[KTriggered time (ms): %u            Predicted triggered time (ms): %u\0338", sensor_query_time*10, predicted_next_sensor_time);
    // uart_printf(CONSOLE, "\0337\033[23;1H\033[KTime diff (ms): %d                 Distance diff (mm): %d\0338", sensor_query_time*10-predicted_next_sensor_time, );
}

void print_sensor(int cout, uint32_t sensor){
    char s1[] = "\0337\033[21;1H\033[K   Triggered sensor:                    \0338";
    ui2a_no0(sensor, 10, s1+33);
    Puts(cout, 0, s1);
    // uart_printf(CONSOLE, "\0337\033[21;1H\033[KTriggered sensor: %u\0338", sensor);
}

void print_sensor_and_prediction(int cout, uint32_t sensor, uint8_t next_sensor, int sensor_query_time, int predicted_next_sensor_time){
    // uart_printf(CONSOLE, "\0337\033[21;1H\033[KTriggered sensor: %u         Next sensor: %u         Predicted next trigger time (ms): %u\0338", sensor, next_sensor, predicted_next_sensor_time);
    // char s1[] = "\0337\033[22;1H\033[KTriggered time (ms):                    Predicted triggered time (ms):               \0338";
    char s1[] = "\0337\033[21;1H\033[K   Triggered sensor:                                      Next sensor:            \0338";
    char s2[] = "\0337\033[22;1H\033[K                                     Predicted next trigger time (ms):         \0338";
    ui2a_no0(sensor, 10, s1+33); 
    ui2a_no0(next_sensor, 10, s1+84);
    Puts(cout, 0, s1);

    i2a_no0(predicted_next_sensor_time, s2+84);
    Puts(cout, 0, s2);
}

void train_states_init(TrainState* trains, uint8_t num_trains) {
    for (int i = 0; i < num_trains; i++) {
        TrainState* ts  = trains + i;

        ts->train_id;
        ts->last_speed = 0;
        ts->train_dest = 255;
        ts->train_location = -1;
        ts->train_sensor_path;
        ts->got_sensor_path = 0;
        ts->sensor_to_stop = 25;;
        ts->delay_time;
        ts->next_sensor = 255;
        ts->next_sensor_err = 255;
        ts->next_sensor_new = 253;
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
        // ts->next = NULL;
        // if (i + 1 < num_trains) {
            // ts->next = trains + i + 1;
        // }
    }
}

// TrainState* getNextFreeTrainState(TrainState** ts) {
//     if(*ts==NULL){
//         return NULL;
//     }
//     TrainState *free = *ts;
//     *ts = (*ts)->next;
//     return free;
// }

// void reclaimTrainState(TrainState** nextFreeTrainState, TrainState* ts) {
//     ts->next = *nextFreeTrainState;
//     *nextFreeTrainState = ts;
// }

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
  SwitchChange sc;
  DelayStopMsg dsm;
  char track = 'a';
  uint32_t offset = 100;


    // train state data
    TrainState train_states[MAX_NUM_TRAINS];
    train_states_init(train_states, MAX_NUM_TRAINS);
    TrainState* train_state_buffer[MAX_NUM_TRAINS];

    // train state buffer for allocating trains
    cb train_cb;
    cb_init(&train_cb, train_state_buffer, MAX_NUM_TRAINS, sizeof(TrainState), 0);
    for (int i = 0; i < MAX_NUM_TRAINS; i++) {
        cb_push_back(&train_cb, &train_states[i]);
    }

    // train accessor hashmap + array
    TrainState* trains[100];

    // to be replace below
  uint32_t last_speed[100];
  for(int i = 0; i<100; i++){
    last_speed[i] = 0;
  }

  uint8_t train_id;
  uint8_t train_dest = 255;
  int train_location = -1;

  NavPath train_nav_path;
  uint8_t got_sensor_path = 0;
  uint8_t sensor_to_stop = 255;
  int delay_time;
  uint8_t next_nav_switch_change;

  NewSensorInfo new_sensor_new;
  NewSensorInfo new_sensor;
  new_sensor.next_sensor = 255;
  NewSensorInfo new_sensor_err;
  new_sensor_err.next_sensor = 255;

  uint8_t last_triggered_sensor = 255;
  uint8_t does_reset = 0;

  uint8_t demo_started = 0;

  int cur_train_speed = 0; // 0 - 14
  int cur_physical_speed = 0; // mm/s 
  uint32_t distance_between_sensors = 0;
  uint32_t last_distance_between_sensors = 0;
  uint32_t terminal_physical_speed = 0; // mm/s
  uint32_t last_new_sensor_time = 0; // us since last new sensor
  TrainSpeedState train_speed_state = STOPPED; // accelerating, deccelerating, constant speed, stopped?
  int sensor_query_time = -1; 
  int predicted_next_sensor_time = 0; // TODO; not sure if this should be set to 0

  for(;;){
    msg_len = Receive(&tid, &tsm, sizeof(TrainServerMsg));

    if(tsm.type==TRAIN_SERVER_NEW_SENSOR){
        sensor_query_time = tsm.arg2;
        Reply(tid, NULL, 0);
        
        if(last_triggered_sensor!=tsm.arg1) {
            train_location = tsm.arg1;

            uint8_t unexpected_sensor = new_sensor.next_sensor!=255 && tsm.arg1!=new_sensor.next_sensor && tsm.arg1!=new_sensor_err.next_sensor;

            if(unexpected_sensor){
                // TODO: use puts
                Puts(cout, 0, "\0337\033[45;1H\033[KUnexpected sensor trigger\0338");
            }else{
                Puts(cout, 0, "\0337\033[45;1H\033[K\0338");
                // first sensor hit, we shouldn't do any speed calculations
                if (last_triggered_sensor != 255) {
                    
                    distance_between_sensors = sensor_distance_between(track, last_triggered_sensor, tsm.arg1); // train_location <--> tsm.arg1 in millimeters
                    if (distance_between_sensors == -1) {
                        
                    }else {
                        // get time delta
                        uint32_t delta_new = sensor_query_time - last_new_sensor_time; // ticks
                        cur_physical_speed = calculate_new_current_speed(&train_speed_state, cur_physical_speed, terminal_physical_speed, distance_between_sensors, delta_new, offset);
                
                    }
                }

                new_printf(cout, 0, "\0337\033[18;1H\033[KSpeed state: %d, train speed: %d, speed: %d, terminal: %d\0338", train_speed_state, cur_train_speed, cur_physical_speed, terminal_physical_speed);
                // uart_printf(CONSOLE, "\0337\033[18;1H\033[KSpeed state: %d, train speed: %d, speed: %d, terminal: %d, TEST %d\0338", train_speed_state, cur_train_speed, cur_physical_speed, terminal_physical_speed, -1);


                if(train_dest!=255 && got_sensor_path){

                    uint32_t stopping_acceleration = train_stopping_acceleration(train_id, cur_train_speed) *  offset / 100; //mm/s^2

                    // (Vf)^2 = (Vo)^2 + 2ad
                    uint32_t stopping_distance = (cur_physical_speed*cur_physical_speed)/(2*stopping_acceleration); // mm
                    SensorPath train_sensor_path = train_nav_path.sensor_path;
                    int last_index = train_sensor_path.num_sensors-1;
                    for (int i = last_index; i >= 0; i--) {
                        if (train_sensor_path.dists[last_index] - train_sensor_path.dists[i] > stopping_distance) {

                            sensor_to_stop = train_sensor_path.sensors[i];
                            uint32_t stopping_distance_difference = (train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]) - stopping_distance;

                            delay_time = (stopping_distance_difference*100)/cur_physical_speed; // 10ms for system ticks
                            // delay_time = 0;
                            new_printf(cout, 0, "\0337\033[60;1H\033[KTrain sensor path dist %d, delay_time %d, last index %d, cur dist %d, diff %d \0338", train_sensor_path.dists[i], delay_time, train_sensor_path.dists[train_sensor_path.num_sensors-1], train_sensor_path.dists[i], train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]);
                            // uart_printf(CONSOLE, "\0337\033[60;1H\033[KTrain sensor path dist %d, delay_time %d, last index %d, cur dist %d, diff %d \0338", train_sensor_path.dists[i], delay_time, train_sensor_path.dists[train_sensor_path.num_sensors-1], train_sensor_path.dists[i], train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]);
                              
                            break;
                        }
                    }

                    // TODO: use puts
                    new_printf(cout, 0, "\0337\033[61;1H\033[KEstimated stopping distance %d, sensor to stop %d \0338", stopping_distance, sensor_to_stop);
                    // uart_printf(CONSOLE, "\0337\033[61;1H\033[KEstimated stopping distance %d, sensor to stop %d \0338", stopping_distance, sensor_to_stop);
                }

                // TODO: what if distance was invalid i.e. invalid sensor reading

                if(train_location==sensor_to_stop){
                    last_speed[train_id] = 0;
                    dsm.train_number = train_id;
                    dsm.delay_until = (int)(tsm.arg2) + delay_time;
                    intended_reply_len = Send(delay_stop_tid, &dsm, sizeof(DelayStopMsg), NULL, 0);
                    if(intended_reply_len!=0){
                        (CONSOLE, "\0337\033[30;1H\033[Ktrainserver delay stop unexpected reply\0338");
                    }
                    // tr(mio, train_id, 0, last_speed);
                    train_dest = 255;
                    sensor_to_stop = 255;
                    got_sensor_path = 0;
                }
            }


            pm.type = PATH_NEXT_SENSOR;
            pm.arg1 = train_location;
            intended_reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t), &new_sensor_new, sizeof(NewSensorInfo));
            
            if(intended_reply_len!=sizeof(NewSensorInfo)){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver get next sensor unexpected reply\0338");
                continue;
            }
            
            if(unexpected_sensor==0 && predicted_next_sensor_time!=0){
                print_estimation(cout, sensor_query_time, last_new_sensor_time, last_distance_between_sensors, predicted_next_sensor_time, cur_physical_speed);
            }

            if(unexpected_sensor){
                print_sensor(cout, tsm.arg1);
            }else if(new_sensor_new.next_sensor == -2){
                predicted_next_sensor_time = 0;
                Puts(cout, 0, "\0337\033[50;1H\033[Knext sensor query failed\0338");
                print_sensor(cout, tsm.arg1);
            } else{
                // uart_printf(CONSOLE, "\0337\033[50;1H\033[Kprints:\0338");
                int next_sensor_distance = sensor_distance_between(track, tsm.arg1, new_sensor_new.next_sensor);
                predicted_next_sensor_time = next_sensor_distance*1000/cur_physical_speed + sensor_query_time*10;
                Puts(cout, 0, "\0337\033[30;1H\033[K\0338");
                print_sensor_and_prediction(cout, tsm.arg1, new_sensor_new.next_sensor, sensor_query_time, predicted_next_sensor_time);
            }
            
            new_printf(cout, 0, "\0337\033[65;1H\033[Knext: %u, next next: %u, switch err: %u, switch: %u\0338", new_sensor_new.next_sensor, new_sensor_new.next_next_sensor, new_sensor_new.next_sensor_switch_err, new_sensor_new.switch_after_next_sensor);
            loc_err_handling(train_location, &new_sensor, &new_sensor_err, &new_sensor_new, &last_triggered_sensor);
            new_printf(cout, 0,  "\0337\033[66;1H\033[Knew_sensor: %u, new_sensor_err: %u, last trig: %u\0338", new_sensor.next_sensor, new_sensor_err.next_sensor, last_triggered_sensor);

            if(unexpected_sensor==0){
                new_printf(cout, 0, "\0337\033[67;1H\033[Knum_switches: %u, next_switch_ind: %u, next_switch: %u, upcoming_switch: %u\0338", train_nav_path.num_switches, next_nav_switch_change, train_nav_path.switches[next_nav_switch_change].switch_num, new_sensor.switch_after_next_sensor);
                if(train_nav_path.num_switches>next_nav_switch_change && train_nav_path.switches[next_nav_switch_change].switch_num==new_sensor.switch_after_next_sensor){
                    // new_printf(cout, 0, "\0337\033[67;1H\033[Kswitch chang: %u\0338", train_nav_path.switches[next_nav_switch_change]);
                    sc.switch_num = train_nav_path.switches[next_nav_switch_change].switch_num;
                    sc.dir = train_nav_path.switches[next_nav_switch_change].dir;
                    int res = change_switches_cmd(switch_tid, &sc, 1);
                    if(res<0){
                        uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
                    }
                    next_nav_switch_change += 1;
                }
            }
            
            last_new_sensor_time = sensor_query_time;
            last_distance_between_sensors = distance_between_sensors;
        }
    } else if(tsm.type==TRAIN_SERVER_TR){
        Reply(tid, NULL, 0);
        train_id = tsm.arg1;

        // uart_printf(CONSOLE, "\0337\033[30;1H\033[KTrain command received train: %u, last speed: %u, speed: %u\0338", tsm.arg1, last_speed[tsm.arg1], tsm.arg2);
        // set current train speed
        cur_train_speed = tsm.arg2;

        // TODO: set terminal speed
        offset = train_velocity_offset(tsm.arg1, tsm.arg2);
        terminal_physical_speed = train_terminal_speed(tsm.arg1, tsm.arg2);

        // set train state
        // other states should be managed by sensor data processing to avoid errors when we go from 0 to 14, then 
        // before acceleration is done, we set the train speed to 14 again (should not be a constant_speed state)
        if (last_speed[tsm.arg1] < tsm.arg2) {
            train_speed_state = ACCELERATING;
        } else if (last_speed[tsm.arg1] > tsm.arg2) {
            train_speed_state = DECELERATING;
        }

        tr(mio, tsm.arg1, tsm.arg2, last_speed);
        demo_started = 1;

    }else if(tsm.type==TRAIN_SERVER_RV && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        rm.train_number = tsm.arg1;
        rm.last_speed = last_speed[rm.train_number];
        intended_reply_len = Send(reverse_tid, &rm, sizeof(ReverseMsg), NULL, 0);
        if(intended_reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_SW && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        sc.switch_num = tsm.arg1;
        sc.dir = (char)tsm.arg2;
        int res = change_switches_cmd(switch_tid, &sc, 1);
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
        Reply(tid, NULL, 0);
        if(tsm.arg1 != train_id){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Knav unexpected train number\0338");
            continue;
        }
        if(tsm.arg3!=0){
            offset = tsm.arg3;
        }
        // offset = tsm.arg3;
        train_dest = tsm.arg2;
        pm.type = PATH_NAV;
        pm.arg1 = train_location;
        pm.dest = tsm.arg2;
        // uart_printf(CONSOLE, "\0337\033[54;1H\033[Ktrain server before send %d\0338", Time(clock));
        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
        }
        got_sensor_path = 0;

        uart_printf(CONSOLE, "\0337\033[34;1H\033[Koffset %d\0338", offset);
    }else if(tsm.type==TRAIN_SERVER_GO && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);

        cur_train_speed = tsm.arg3;
        
        offset = train_velocity_offset(tsm.arg1, tsm.arg2);
        terminal_physical_speed = train_terminal_speed(tsm.arg1, tsm.arg3);
        if (last_speed[tsm.arg1] < tsm.arg3) {
            train_speed_state = ACCELERATING;
        } else if (last_speed[tsm.arg1] > tsm.arg3) {
            train_speed_state = DECELERATING;
        }

        train_id = tsm.arg1;
        tr(mio, tsm.arg1, tsm.arg3, last_speed);
        demo_started = 1;

        train_dest = tsm.arg2;
        pm.type = PATH_NAV;
        pm.arg1 = train_location;
        pm.dest = tsm.arg2;
        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
        }
        got_sensor_path = 0;
    }else if(tsm.type==TRAIN_SERVER_TRACK_CHANGE && msg_len==sizeof(TrainServerMsgSimple)){
        Reply(tid, NULL, 0);
        pm.type = PATH_TRACK_CHANGE;
        pm.arg1 = tsm.arg1;
        int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint32_t), NULL, 0);
        if(reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver tarck cmd unexpected reply\0338");
        }
    }else if(tsm.type==TRAIN_SERVER_NAV_PATH && msg_len==sizeof(TrainServerMsg)){
        //TODO: maybe this should be a reply?
        Reply(tid, NULL, 0);
        memcpy(&train_nav_path, tsm.data, sizeof(NavPath));
        // SensorPath *sp = tsm.data;

        for(int i = 0; i<train_nav_path.sensor_path.num_sensors; i++){
            uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u %u\0338", 40+i, train_nav_path.sensor_path.sensors[i], train_nav_path.sensor_path.dists[i]);
        }
        uart_printf(CONSOLE, "\0337\033[%u;1H\033[K num_switches: %u \0338", 30, train_nav_path.num_switches );
        for(int i = 0; i<train_nav_path.num_switches; i++){
            uart_printf(CONSOLE, "\0337\033[%u;1H\033[K switch: %u, dir: %u\0338", 31+i, train_nav_path.switches[i].switch_num, train_nav_path.switches[i].dir );
        }

        // if first switch is upcoming, change it now
        if(train_nav_path.num_switches>0 && train_nav_path.switches[0].switch_num==new_sensor.switch_after_next_sensor){
            sc.switch_num = train_nav_path.switches[0].switch_num;
            sc.dir = train_nav_path.switches[0].dir;
            int res = change_switches_cmd(switch_tid, &sc, 1);
            if(res<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver sw cmd unexpected reply\0338");
            }
            next_nav_switch_change = 1;
        }else{
            next_nav_switch_change = 0;
        }

        /* 
        TODO: 

        calculate stop distance using velocity
        total_dist = train_sensor_path.sensor_path.dists[train_sensor_path.num_sensors-1];
        if(total_dist<stop_distance){
            tr(train_id, 0);
        }
        */

        got_sensor_path = 1;
        // uart_printf(CONSOLE, "\0337\033[70;1H\033[Kcheck nav path %u %u %u\0338", train_sensor_path.num_sensors, train_sensor_path.sensors[0],  train_sensor_path.sensors[train_sensor_path.num_sensors-1]);
        // E6 14 32 69
        sensor_to_stop = -1;
    }else{
        Reply(tid, NULL, 0);
        uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver unknown cmd %d %u\0338", tsm.type, msg_len);
    }
  }
}
