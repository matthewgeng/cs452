#include <stddef.h>
#include "delaystop.h"
#include "syscall.h"
#include "clock.h"
#include "rpi.h"

void stop(int marklin_tid, unsigned int trainNumber){
  char cmd[3];
  cmd[0] = 0;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);
}

void delay_stop(){
    RegisterAs("delaystop\0");
    int clock_tid = WhoIs("clock\0");
    int mio = WhoIs("mio\0");

    DelayStopMsg dsm;
    int tid;

    for(;;){
        Receive(&tid, &dsm, sizeof(DelayStopMsg));
        Reply(tid, NULL, 0);
        Delay(clock_tid, dsm.delay);
        stop(mio, dsm.train_number);
    }
}