#include <stddef.h>
#include "delayexecute.h"
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

void delay_execute_loop(){
    // int cout = WhoIs("cout\0");
    int clock_tid = WhoIs("clock\0");
    int mio = WhoIs("mio\0");

    DelayExecuteMsg dsm;
    int tid;

    for(;;){
        Receive(&tid, &dsm, sizeof(DelayExecuteMsg));
        // uart_printf(CONSOLE, "\0337\033[36;1H\033[Kdelayuntil received %d\0338", Time(clock_tid));
        Reply(tid, NULL, 0);
        DelayUntil(clock_tid, dsm.delay_until);
        // uart_printf(CONSOLE, "\0337\033[36;1H\033[Kdelayuntil stopped %d\0338", Time(clock_tid));
        stop(mio, dsm.train_number);
    }
}

void delay_execute1(){
    RegisterAs("delayexecute1\0");
    delay_execute_loop();
}

void delay_execute2(){
    RegisterAs("delayexecute2\0");
    delay_execute_loop();
}

void delay_execute3(){
    RegisterAs("delayexecute3\0");
    delay_execute_loop();
}

void delay_execute4(){
    RegisterAs("delayexecute4\0");
    delay_execute_loop();
}

void delay_execute5(){
    RegisterAs("delayexecute5\0");
    delay_execute_loop();
}