#include "sensors.h"
#include "int_cb.h"
#include "rpi.h"
#include "nameserver.h"
#include "trainserver.h"
#include "timer.h"

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

  int new_sensor_triggered = 0;
  char sensors_str[] = "\033[8;1H\033[KMost recent sensors:                                                           ";
  int sensors_str_index;

  // TODO: assuming only 1 new sensor per query rn
  int triggered_sensor;
  int intended_reply_len;
  TrainServerMsgSimple tsm;
  tsm.type = TRAIN_SERVER_NEW_SENSOR;

  for(;;){
    // uart_printf(CONSOLE, "\033[30;1Hstart %d", time);
    Putc(mio, MARKLIN, 0x85);
    triggered_sensor = -1;

    for(int i = 0; i<10; i++){
      sensor_byte = Getc(mio, MARKLIN);
      for (int u = 0; u < 8; u++) {
        if (sensor_byte & (1 << u)) {
          int sensorNum = i*8+7-u;
          if(sensorNum!=last_sensor_triggered){
            if(new_sensor_triggered==1){
                // two sensors got triggered in one query
                // error for now assuming there's only one train on the tracks
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktwo sensors triggered in one query %d %d\0338", sensorNum, triggered_sensor);
            }
            push_intcb(&sensor_cb, sensorNum);
            last_sensor_triggered = sensorNum;
            new_sensor_triggered = 1;
          }
          triggered_sensor = sensorNum;
        }
      }
    }
    
    // uart_printf(CONSOLE, "\0337\033[25;1H\033[Knew sensor %d\0338", new_sensor);
    if(triggered_sensor!=-1){
      // send whenever we get a sensor
      tsm.arg1 = triggered_sensor;
      tsm.arg2 = time;
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
    time += 10;
    DelayUntil(clock_tid, time);
  }
}
