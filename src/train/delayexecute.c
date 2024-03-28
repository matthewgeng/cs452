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
    int train_number, last_speed;
    char cmd[4];

    for(;;){
        Receive(&tid, &dsm, sizeof(DelayExecuteMsg));
        // uart_printf(CONSOLE, "\0337\033[36;1H\033[Kdelayuntil received %d\0338", Time(clock_tid));
        Reply(tid, NULL, 0);
        if(dsm.type==DELAY_STOP){
            if(dsm.delay!=0){
                // DelayUntil(clock_tid, dsm.delay);
                Delay(clock_tid, dsm.delay);
            }
            // uart_printf(CONSOLE, "\0337\033[36;1H\033[Kdelayuntil stopped %d\0338", Time(clock_tid));
            stop(mio, dsm.train_number);
        }else if(dsm.type==DELAY_RV){
            train_number = (int)(dsm.train_number);
            last_speed = (int)(dsm.last_speed);
            if(dsm.delay!=0){
                Delay(clock_tid, dsm.delay);
                // Delay(clock_tid, 300);
            }
            cmd[0] = 0;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            // TODO: appropriately handle when speeds are > 16

            if(last_speed<=10){
                Delay(clock_tid, 400);
            }else{
                Delay(clock_tid, 500);
            }
            cmd[0] = 15;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            Delay(clock_tid, 100);
            cmd[0] = last_speed;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);

            // str_cpy_w0(func_res+10, "Train reversed");
            // Puts(cout, CONSOLE, func_res);
        }else if(dsm.type==DELAY_RV_STOP){
            train_number = (int)(dsm.train_number);
            last_speed = (int)(dsm.last_speed);
            if(dsm.delay!=0){
                Delay(clock_tid, dsm.delay);
            }
            cmd[0] = 0;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            // TODO: appropriately handle when speeds are > 16

            if(last_speed<=10){
                Delay(clock_tid, 400);
            }else{
                Delay(clock_tid, 500);
            }
            cmd[0] = 15;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            Delay(clock_tid, 100);
            cmd[0] = last_speed;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);

            Delay(clock_tid, dsm.stop_delay);
            stop(mio, dsm.train_number);
            // str_cpy_w0(func_res+10, "Train reversed");
            // Puts(cout, CONSOLE, func_res);
        }else if(dsm.type==DELAY_STOP_START){
            train_number = (int)(dsm.train_number);
            last_speed = (int)(dsm.last_speed);
            cmd[0] = 0;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            // TODO: appropriately handle when speeds are > 16

            if(dsm.delay!=0){
                Delay(clock_tid, dsm.delay);
            }
            cmd[0] = last_speed;
            cmd[1] = train_number;
            Puts_len(mio, MARKLIN, cmd, 2);
            // str_cpy_w0(func_res+10, "Train reversed");
            // Puts(cout, CONSOLE, func_res);
        }
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