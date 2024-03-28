#include "sensors.h"
#include "int_cb.h"
#include "rpi.h"
#include "nameserver.h"
#include "trainserver.h"
#include "timer.h"
#include "util.h"
#include "io.h"

void sensor_update(){
  RegisterAs("sensor\0");
  int clock_tid = WhoIs("clock\0");
  int cout = WhoIs("cout\0");
  int mio = WhoIs("mio\0");
  int train_server_tid = WhoIs("trainserver\0");

  int time = Time(clock_tid);
  int last_sensor_triggered = -1;
  char sensor_byte;

  int sensors[12];
  IntCB sensor_cb;
  initialize_intcb(&sensor_cb, sensors, 12, 1);

  char sensors_str[] = "\033[8;40H\033[KMost recent sensors:                                                           ";
  int sensors_str_index;

  // allows 2 sensor triggers
  int triggered_sensors[MAX_NUM_TRIGGERED_SENSORS];
  IntCB triggered_sensor_cb;
  initialize_intcb(&triggered_sensor_cb, triggered_sensors, MAX_NUM_TRIGGERED_SENSORS, 0);

  int intended_reply_len;
  TrainServerMsgSimple tsm;
  tsm.type = TRAIN_SERVER_NEW_SENSOR;
    int new_sensor_triggered = 0;

  for(;;){
    // uart_printf(CONSOLE, "\033[30;1Hstart %d", time);
    int time = Time(clock_tid);
    Putc(mio, MARKLIN, 0x85);
    // memset(last_sensors_triggered, -1, MAX_NUM_TRIGGERED_SENSORS);

    for(int i = 0; i<10; i++){
      sensor_byte = Getc(mio, MARKLIN);
      for (int u = 0; u < 8; u++) {
        if (sensor_byte & (1 << u)) {
          int sensorNum = i*8+7-u;
            if (sensorNum != last_sensor_triggered) {
                push_intcb(&sensor_cb, sensorNum);
                last_sensor_triggered = sensorNum;
            }

            if (is_full_intcb(&triggered_sensor_cb)) {
                // > 2 sensors got triggered in one query, shouldn't happen unless we have more than 2 trains
                new_printf(cout, 0, "\0337\033[30;1H\033[K>=3 sensors triggered in one query\0338");
            }
            push_intcb(&triggered_sensor_cb, sensorNum);
            new_sensor_triggered = 1;
        }
      }
    }

    if(!is_empty_intcb(&triggered_sensor_cb)){

        // first sensor is guaranteed to exist (not empty cb)
        int first_sensor = pop_intcb(&triggered_sensor_cb);
        tsm.arg1 = (first_sensor == -1) ? 255 : first_sensor;

        // second sensor
        int second_sensor = pop_intcb(&triggered_sensor_cb);
        tsm.arg2 = (second_sensor == -1) ? 255 : second_sensor;
        tsm.arg3 = time;

        // TODO: currently don't support third sensor
        intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
        if(intended_reply_len!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Ksensor task unexpected reply from train server %d\0338", intended_reply_len);
        }
    }
    
    if(new_sensor_triggered==1){
      // update the console output whenever we get a *new* sensor
        sensors_str_index = 31;
        int cb_count = 0;
        int cb_index = decrement_intcb(sensor_cb.capacity, sensor_cb.end);
        int i;
        char sensor_band;
        int sensor_num;
        while(cb_count < sensor_cb.count){
            i = sensor_cb.queue[cb_index];
            sensor_band = i/16 + 'A';
            sensors_str[sensors_str_index] = sensor_band;
            sensor_num = i%16 + 1;
            ui2a( sensor_num, 10, sensors_str+sensors_str_index+1 );
            if(sensor_num<10){
                sensors_str[sensors_str_index+2] = ' ';
                sensors_str_index = sensors_str_index+3;
            }else{
                sensors_str[sensors_str_index+3] = ' ';
                sensors_str_index = sensors_str_index+4;
            }
            cb_index = decrement_intcb(sensor_cb.capacity, cb_index);
            cb_count += 1;
        }
        sensors_str[sensors_str_index] = '\0';
        Puts(cout, CONSOLE, sensors_str);
        new_sensor_triggered = 0;
    }
    Delay(clock_tid, 6);
  }
}
