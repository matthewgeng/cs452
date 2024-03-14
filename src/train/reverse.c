#include <stddef.h>
#include "reverse.h"
#include "rpi.h"
#include "util.h"

void reverse(){

    RegisterAs("reverse\0");
    int marklin_tid = WhoIs("mio\0");
    int clock = WhoIs("clock\0");
    int cout = WhoIs("cout\0");

    char func_res[30];
    str_cpy(func_res, "\033[12;1H\033[K");

    int tid;
    ReverseMsg rm;
    int train_number, last_speed;
    int msg_len;
    char cmd[4];
    for(;;){
        msg_len = Receive(&tid, &rm, sizeof(ReverseMsg));
        Reply(tid, NULL, 0);
        if(msg_len!=sizeof(ReverseMsg)){
            str_cpy_w0(func_res+10, "rv msglen err");
            Puts(cout, CONSOLE, func_res);
            continue;
        }
        train_number = (int)(rm.train_number);
        last_speed = (int)(rm.last_speed);

        cmd[0] = 0;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);
        // TODO: appropriately handle when speeds are > 16

        if(last_speed<=10){
          Delay(clock, 500);
        }else{
          Delay(clock, 600);
        }
        cmd[0] = 15;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);
        cmd[0] = last_speed;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);

        str_cpy_w0(func_res+10, "Train reversed");
        Puts(cout, CONSOLE, func_res);
    }

}
