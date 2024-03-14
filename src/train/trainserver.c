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
#include "io.h"

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]){
  char cmd[3];
  cmd[0] = trainSpeed;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  last_speed[trainNumber] = trainSpeed;
}


uint8_t loc_err_handling(int train_location, uint8_t *next_sensor, uint8_t *next_sensor_err, uint8_t next_sensor_new){
  if(*next_sensor == 255 || train_location == *next_sensor){
    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kloc 2\0338");
    *next_sensor = next_sensor_new;
    *next_sensor_err = 255;
    return 0;
  }else if(*next_sensor_err != 255 && train_location == *next_sensor_err){
    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kloc 3\0338");

    *next_sensor = next_sensor_new;
    *next_sensor_err = 255;
    return 0;
  }else if(*next_sensor_err == 255){
    // skipped a sensor
    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kloc 4\0338");
    *next_sensor_err = next_sensor_new;
    return 0;
  }
    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kloc 5\0338");
//   next_sensor_err = next_sensor_new;
  return 1;
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
                    // new_cur_speed = old_speed;
                    new_cur_speed = average_new_speed;
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
    new_speed = new_speed * offset / 100;
    // uart_printf(CONSOLE, "\0337\033[30;1H\033[Knew speed state: %d, avg speed: %u, new speed: %u\0338", train_speed_state, average_new_speed, new_cur_speed);
    return new_speed;
}

void print_estimation(int cout, int sensor_query_time, int predicted_next_sensor_time, int cur_physical_speed){

    char s1[] = "\0337\033[22;1H\033[KTriggered time (ms):                    Predicted triggered time (ms):               \0338";
    char s2[] = "\0337\033[23;1H\033[K     Time diff (ms):                               Distance diff (mm):          \0338";

    i2a_no0(sensor_query_time*10, s1+33);
    i2a_no0(predicted_next_sensor_time, s1+84);
    Puts(cout, 0, s1);

    i2a_no0(sensor_query_time*10-predicted_next_sensor_time, s2+33);
    i2a_no0((sensor_query_time*10-predicted_next_sensor_time)*(cur_physical_speed)/1000, s2+84);
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

void print_sensor_and_prediction(int cout, uint32_t sensor, uint8_t next_sensor, int predicted_next_sensor_time){
    // uart_printf(CONSOLE, "\0337\033[21;1H\033[KTriggered sensor: %u         Next sensor: %u         Predicted next trigger time (ms): %u\0338", sensor, next_sensor, predicted_next_sensor_time);
    // char s1[] = "\0337\033[22;1H\033[KTriggered time (ms):                    Predicted triggered time (ms):               \0338";
    char s1[] = "\0337\033[21;1H\033[K   Triggered sensor:                                      Next sensor:            Predicted next trigger time (ms):         \0338";
    ui2a_no0(sensor, 10, s1+33); 
    ui2a_no0(next_sensor, 10, s1+84);
    i2a_no0(predicted_next_sensor_time, s1+130);
    Puts(cout, 0, s1);
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
  SwitchChange sc;
  DelayStopMsg dsm;

  uint32_t last_speed[100];
  for(int i = 0; i<100; i++){
    last_speed[i] = 0;
  }
  uint8_t train_id;
  uint8_t train_dest = 255;
  int train_location = -1;

  SensorPath train_sensor_path;
  uint8_t got_sensor_path = 0;
  uint8_t sensor_to_stop = 255;
  int delay_time;

  uint8_t next_sensor = 255;
  uint8_t next_sensor_err = 255;
  uint8_t next_sensor_new = 253;
  uint8_t last_triggered_sensor = 255;
  uint8_t does_reset = 0;
  char track = 'a';

  uint8_t demo_started = 0;
  // char next_sensor_str = "\0337\033[18;1H\033[KNext sensor:   \0338";

  int cur_train_speed = 0; // 0 - 14
  int cur_physical_speed = 0; // mm/s 
  uint32_t distance_between_sensors;
  uint32_t terminal_physical_speed = 0; // mm/s
  uint32_t last_new_sensor_time = 0; // us since last new sensor
  TrainSpeedState train_speed_state = STOPPED; // accelerating, deccelerating, constant speed, stopped?
  int sensor_query_time = -1; 
  int predicted_next_sensor_time = 0; // TODO; not sure if this should be set to 0

  uint32_t offset = 0;

  int w =0;

  for(;;){
    msg_len = Receive(&tid, &tsm, sizeof(TrainServerMsg));

    if(tsm.type==TRAIN_SERVER_NEW_SENSOR){
        sensor_query_time = tsm.arg2;
        Reply(tid, NULL, 0);
        // if(demo_started==1){
            // uart_printf(CONSOLE, "\0337\033[38;1H\033[KSpeed state: %d, train speed: %d, speed: %d, current sensor: %d, next sensor: %d, \0338", train_speed_state, cur_train_speed, cur_physical_speed, cur_physical_accel, terminal_physical_speed);

            if(last_triggered_sensor!=tsm.arg1) {
                train_location = tsm.arg1;

                uint8_t unexpected_sensor = next_sensor!=255 && tsm.arg1!=next_sensor;

                if(unexpected_sensor){
                    // TODO: use puts
                    Puts(cout, 0, "\0337\033[45;1H\033[KUnexpected sensor trigger\0338");
                }else{
                    Puts(cout, 0, "\0337\033[45;1H\033[K\0338");
                    // first sensor hit, we shouldn't do any speed calculations
                    if (last_triggered_sensor != 255) {

                        distance_between_sensors = sensor_distance_between(track, last_triggered_sensor, tsm.arg1); // train_location <--> tsm.arg1 in millimeters
                        if (distance_between_sensors == -1) {
                            
                        }{
                            // get time delta
                            uint32_t delta_new = sensor_query_time - last_new_sensor_time; // ticks
                            cur_physical_speed = calculate_new_current_speed(&train_speed_state, cur_physical_speed, terminal_physical_speed, distance_between_sensors, delta_new, offset);
                    
                        }
                    }

                    new_printf(cout, 0, "\0337\033[20;1H\033[KSpeed state: %d, train speed: %d, speed: %d, terminal: %d\0338", train_speed_state, cur_train_speed, cur_physical_speed, terminal_physical_speed);


                    if(train_dest!=255 && got_sensor_path){
                            // sensor_to_stop = train_sensor_path.sensors[train_sensor_path.num_sensors-2];
                        
                        uint32_t stopping_acceleration = train_stopping_acceleration(train_id, cur_train_speed); //mm/s^2

                        // (Vf)^2 = (Vo)^2 + 2ad
                        uint32_t stopping_distance = (cur_physical_speed*cur_physical_speed)/(2*stopping_acceleration); // mm

                        int last_index = train_sensor_path.num_sensors-1;
                        for (int i = last_index; i >= 0; i--) {
                            if (train_sensor_path.dists[last_index] - train_sensor_path.dists[i] > stopping_distance) {

                                sensor_to_stop = train_sensor_path.sensors[i];
                                uint32_t stopping_distance_difference = (train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]) - stopping_distance;

                                delay_time = (stopping_distance_difference*100)/cur_physical_speed; // 10ms for system ticks
                                // delay_time = 0;
                                new_printf(cout, 0, "\0337\033[60;1H\033[KTrain sensor path dist %d, delay_time %d, last index %d, cur dist %d, diff %d \0338", train_sensor_path.dists[i], delay_time, train_sensor_path.dists[train_sensor_path.num_sensors-1], train_sensor_path.dists[i], train_sensor_path.dists[train_sensor_path.num_sensors-1] - train_sensor_path.dists[i]);
                                break;
                            }
                        }

                        // TODO: use puts
                        new_printf(cout, 0, "\0337\033[61;1H\033[KEstimated stopping distance %d, sensor to stop %d \0338", stopping_distance, sensor_to_stop);
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
                intended_reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t), &next_sensor_new, sizeof(uint8_t));
                
                if(intended_reply_len!=sizeof(uint8_t)){
                    uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver get next sensor unexpected reply\0338");
                    continue;
                }
                
                if(unexpected_sensor==0 && predicted_next_sensor_time!=0){
                    // TODO: use puts
                    print_estimation(cout, sensor_query_time, predicted_next_sensor_time, cur_physical_speed);

                    // if (offset % 2 == 1) {
                    //     int adjustment_factor = distance_between_sensors/((sensor_query_time - last_new_sensor_time)/100) - cur_physical_speed;

                    //     cur_physical_speed -= adjustment_factor;

                    //     uart_printf(CONSOLE, "\0337\033[35;1H\033[Kadjustment factor: %d, old speed: %d, new speed: %d\0338", adjustment_factor, cur_physical_speed + adjustment_factor, cur_physical_speed);
                    // }
                }else{
                    // Puts(cout, 0, "\0337\033[22;1H\033[KPrevious predicted sensor time 0\0338");
                }

                if(next_sensor_new == 255){
                    // will hit an exit soon, reverse halt
                    // uart_printf(CONSOLE, "\0337\033[50;1H\033[Khalt before exit\0338");

                    tr(mio, train_id, 15, last_speed);
                    does_reset = 1;
                    predicted_next_sensor_time = 0;
                    Puts(cout, 0, "\0337\033[30;1H\033[K\0338");
                    print_sensor(cout, tsm.arg1);
                } else if(next_sensor_new == 254){
                    // will hit an exit soon, reverse halt
                    predicted_next_sensor_time = 0;
                    Puts(cout, 0, "\0337\033[50;1H\033[Knext sensor query failed\0338");
                    print_sensor(cout, tsm.arg1);
                } else {
                    // uart_printf(CONSOLE, "\0337\033[50;1H\033[Kprints:\0338");
                    int next_sensor_distance = sensor_distance_between(track, tsm.arg1, next_sensor_new);
                    predicted_next_sensor_time = (next_sensor_distance/cur_physical_speed)*1000 + sensor_query_time*10;
                    Puts(cout, 0, "\0337\033[30;1H\033[K\0338");
                    print_sensor_and_prediction(cout, tsm.arg1, next_sensor_new, predicted_next_sensor_time);

                    does_reset = loc_err_handling(train_location, &next_sensor, &next_sensor_err, next_sensor_new);
                    
                }
                
                // uart_printf(CONSOLE, "\0337\033[55;1H\033[K this %d\0338", train_location);
                // uart_printf(CONSOLE, "\0337\033[56;1H\033[K                      next_sensor %u, next_sensor_err %u\0338", next_sensor, next_sensor_err);

                // uart_printf(CONSOLE, "\0337\033[18;1H\033[KNext sensors: %u %u\0338", next_sensor, next_sensor_err);
                if(does_reset){
                    tr(mio, train_id, 0, last_speed);

                    train_dest = 255;
                    train_location = -1;

                    got_sensor_path = 0;
                    sensor_to_stop = 255;
                    delay_time;

                    next_sensor = 255;
                    next_sensor_err = 255;
                    next_sensor_new = 253;
                    last_triggered_sensor = 255;
                    does_reset = 0;
                    track = 'a';

                    demo_started = 0;

                    cur_train_speed = 0; // 0 - 14
                    cur_physical_speed = 0; // mm/s 
                    distance_between_sensors;
                    terminal_physical_speed = 0; // mm/s
                    last_new_sensor_time = 0; // us since last new sensor
                    train_speed_state = STOPPED; // accelerating, deccelerating, constant speed, stopped?
                    sensor_query_time = -1; 
                    predicted_next_sensor_time = 0; // TODO; not sure if this should be set to 0

                    offset = 0;

                    new_printf(cout, 0, "\0337\033[42;1H\033[Kreset\0338");
                }else{
                    new_printf(cout, 0, "\0337\033[42;1H\033[K\0338");
                }
                last_new_sensor_time = sensor_query_time;
            }
            last_triggered_sensor = tsm.arg1;
        // }
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
            int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint8_t), NULL, 0);
            if(reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver pf cmd unexpected reply\0338");
            }
        }else if(tsm.type==TRAIN_SERVER_NAV && msg_len==sizeof(TrainServerMsgSimple)){
            Reply(tid, NULL, 0);
            if(tsm.arg1 != train_id){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Knav unexpected train number\0338");
                continue;
            }
            // offset = tsm.arg3;
            train_dest = tsm.arg2;
            pm.type = PATH_NAV;
            pm.arg1 = train_location;
            pm.dest = tsm.arg2;
            // uart_printf(CONSOLE, "\0337\033[54;1H\033[Ktrain server before send %d\0338", Time(clock));
            int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint8_t), NULL, 0);
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
            int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint8_t), NULL, 0);
            if(reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
            }
            got_sensor_path = 0;
        }else if(tsm.type==TRAIN_SERVER_TRACK_CHANGE && msg_len==sizeof(TrainServerMsgSimple)){
            Reply(tid, NULL, 0);
            pm.type = PATH_TRACK_CHANGE;
            pm.arg1 = tsm.arg1;
            int reply_len = Send(pathfind_tid, &pm, sizeof(path_arg_type)+sizeof(uint32_t)+sizeof(uint8_t), NULL, 0);
            if(reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver tarck cmd unexpected reply\0338");
            }
        }else if(tsm.type==TRAIN_SERVER_NAV_PATH && msg_len==sizeof(TrainServerMsg)){
            //TODO: maybe this should be a reply?
            Reply(tid, NULL, 0);
            memcpy(&train_sensor_path, tsm.data, sizeof(SensorPath));
            SensorPath *sp = tsm.data;

            // for(int i = 0; i<sp->num_sensors; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u %u\0338", 40+i, sp->sensors[i], sp->dists[i]);
            // }
            /* 
            TODO: 

            calculate stop distance using velocity
            total_dist = train_sensor_path.sensor_path.dists[train_sensor_path.num_sensors-1];
            if(total_dist<stop_distance){
                tr(train_id, 0);
            }
            */

            got_sensor_path = 1;
            uart_printf(CONSOLE, "\0337\033[70;1H\033[Kcheck nav path %u %u %u\0338", train_sensor_path.num_sensors, train_sensor_path.sensors[0],  train_sensor_path.sensors[train_sensor_path.num_sensors-1]);
            // E6 14 32 69
            sensor_to_stop = -1;
        }else{
            Reply(tid, NULL, 0);
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver unknown cmd %d %u\0338", tsm.type, msg_len);
        }
    }
}
