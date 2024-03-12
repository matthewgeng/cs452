#include <stddef.h>
#include "trainserver.h"
#include "syscall.h"
#include "rpi.h"
#include "reverse.h"
#include "switches.h"
#include "pathfinding.h"
#include "util.h"

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]){
  char cmd[3];
  cmd[0] = trainSpeed;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  last_speed[trainNumber] = trainSpeed;
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

  int tid;
  TrainServerMsg tsm;
  int msg_len;
  int intended_reply_len;

  ReverseMsg rm;
  PathMessage pm;
  SwitchChange sc;

  uint32_t last_speed[100];
  for(int i = 0; i<100; i++){
    last_speed[i] = 0;
  }
  int train_location = -1;
  SensorPath train_sensor_path;
  SensorPath *received_sp;

  for(;;){
    msg_len = Receive(&tid, &tsm, sizeof(TrainServerMsg));
    if(tsm.type==TRAIN_SERVER_NEW_SENSOR && msg_len==sizeof(TrainServerMsgSimple)){
      Reply(tid, NULL, 0);
      train_location = tsm.arg1;
    }else if(tsm.type==TRAIN_SERVER_TR){
      Reply(tid, NULL, 0);
      tr(mio, tsm.arg1, tsm.arg2, last_speed);
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
    }else if(tsm.type==TRAIN_SERVER_PF && msg_len==sizeof(TrainServerMsgSimple)){
      Reply(tid, NULL, 0);
      pm.type = 'P';
      pm.arg1 = tsm.arg1;
      pm.dest = tsm.arg2;
      int reply_len = Send(pathfind_tid, &pm, sizeof(pm), NULL, 0);
      if(reply_len!=0){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver pf cmd unexpected reply\0338");
      }
    }else if(tsm.type==TRAIN_SERVER_NAV && msg_len==sizeof(TrainServerMsgSimple)){
      Reply(tid, NULL, 0);
      pm.type = 'T';
      pm.arg1 = train_location;
      pm.dest = tsm.arg2;
      int reply_len = Send(pathfind_tid, &pm, sizeof(pm), NULL, 0);
      if(reply_len!=0){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver nav cmd unexpected reply\0338");
      }
    }else if(tsm.type==TRAIN_SERVER_NAV_PATH && msg_len==sizeof(TrainServerMsg)){
      Reply(tid, NULL, 0);
      memcpy(&train_sensor_path, tsm.data, sizeof(SensorPath));
      // uart_printf(CONSOLE, "\0337\033[30;1H\033[Kcheck memcpy %u %u %u\0338", train_sensor_path.num_sensors, train_sensor_path.sensors[0], train_sensor_path.dists[0]);
    }else{
      Reply(tid, NULL, 0);
      uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver unknown cmd %d %u\0338", tsm.type, msg_len);
    }
  }
}