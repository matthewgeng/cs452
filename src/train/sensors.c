#include "sensors.h"
#include "int_cb.h"
#include "rpi.h"
#include "nameserver.h"
#include "trainserver.h"

void sensor_update(){
  RegisterAs("sensor\0");
  int clock_tid = WhoIs("clock\0");
  int cout = WhoIs("cout\0");
  int mio = WhoIs("mio\0");
  int train_server_tid = WhoIs("trainserver\0");

  int time = Time(clock_tid);
  int last_sensor_triggered = 1000;
  char sensor_byte;

  int sensors[12];
  IntCB sensor_cb;
  initialize_intcb(&sensor_cb, sensors, 12, 1);

  int sensors_changed = 0;
  char sensors_str[] = "\033[8;1H\033[KMost recent sensors:                                                           ";
  int sensors_str_index;

  // TODO: assuming only 1 new sensor per query rn
  int new_sensor;
  int intended_reply_len;
  TrainServerMsg msg;
  msg.type = TRAIN_SERVER_NEW_SENSOR;

  for(;;){
    Putc(mio, MARKLIN, 0x85);

    for(int i = 0; i<10; i++){
      sensor_byte = Getc(mio, MARKLIN);
      for (int u = 0; u < 8; u++) {
        if (sensor_byte & (1 << u)) {
          int sensorNum = i*8+7-u;
          if(sensorNum!=last_sensor_triggered){
            if(sensors_changed==1){
                // two sensors got triggered in one query
                // error for now assuming there's only one train on the tracks
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktwo sensors triggered in one query %d %d\0338", sensorNum, new_sensor);
            }
            push_intcb(&sensor_cb, sensorNum);
            last_sensor_triggered = sensorNum;
            sensors_changed = 1;
            new_sensor = sensorNum;
          }
        }
      }
    }
    
    msg.arg1 = new_sensor;
    intended_reply_len = Send(train_server_tid, new_sensor, sizeof(TrainServerMsg));
    if(intended_reply_len!=0){
      uart_printf(CONSOLE, "\0337\033[30;1H\033[Ksensor task unexpected reply from train server %d\0338", intended_reply_len);
    }
    
    if(sensors_changed){
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
        sensors_changed = 0;
    }
    time += 10;
    DelayUntil(clock_tid, time);
  }
}