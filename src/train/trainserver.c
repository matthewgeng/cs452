#include <stddef.h>
#include "trainserver.h"
#include "syscall.h"

void trainserver(){
  RegisterAs("trainserver\0");
  int mio = WhoIs("mio\0");
  int clock = WhoIs("clock\0");
  int cout = WhoIs("cout\0");
  int sensor_tid = WhoIs("sensor\0");
  int pathfind_tid = WhoIs("pathfind\0");

  int tid;
  TrainServerMsg msg;
  int msg_len;
  char cmd[4];

  int train_location = -1;

  for(;;){
    msg_len = Receive(&tid, &msg, sizeof(TrainServerMsg));

    if(tid==sensor_tid && msg.type=='S'){
      Reply(tid, NULL, 0);
      train_location = msg.arg;
    }else if(tid==pathfind_tid && msg.type=='P'){
      Reply(tid, train_location, sizeof(int));
    }
  }
}