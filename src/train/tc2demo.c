#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tc2demo.h"
#include "rpi.h"
#include "pathfinding.h"
#include "trainserver.h"
// #include "io.h"

// int get_random_dest(){
//     srand(10);
//     return rand() % 81;
// }

void tc2demo(){
    RegisterAs("tc2demo\0");

    int clock_tid = WhoIs("clock\0");
    int cout = WhoIs("cout\0");
    int mio = WhoIs("mio\0");
    int pathfind_tid = WhoIs("pathfind\0");
    int train_server_tid = WhoIs("trainserver\0");

    DemoMessage dm;
    int tid;
    int train_number, last_speed;
    char cmd[4];

    uint8_t train1 = 1;
    uint8_t start_dest1 = 73;
    uint8_t train2 = 2;
    uint8_t start_dest2 = 30;

    PathMessage pm;
    uint8_t dest;
    int intended_reply_len;

    TrainServerMsgSimple tsm;
    uint32_t train_id;

    int dests[] = {66,70, 52,78,46,24,3};
    uint8_t dest_index = 0;

    for(;;){
        Receive(&tid, &dm, sizeof(DemoMessage));
        Reply(tid, NULL, 0);
        if(dm.type==DEMO_START){
            // pm.type = PATH_GET_RANDOM_DEST;
            // pm.arg1 = train1;
            // intended_reply_len = Send(pathfind_tid, &pm, sizeof(PathMessage), &dest, sizeof(uint8_t));
            // if(intended_reply_len!=sizeof(uint8_t)){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            // }
            tsm.type = TRAIN_SERVER_NAV;
            tsm.arg1 = train1;
            tsm.arg2 = start_dest1;
            tsm.arg3 = 0;
            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            }

            Delay(clock_tid, 1500);
            // pm.type = PATH_GET_RANDOM_DEST;
            // pm.arg1 = train2;
            // intended_reply_len = Send(pathfind_tid, &pm, sizeof(PathMessage), &dest, sizeof(uint8_t));
            // if(intended_reply_len!=sizeof(uint8_t)){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            // }
            tsm.type = TRAIN_SERVER_NAV;
            tsm.arg1 = train2;
            tsm.arg2 = start_dest2;
            tsm.arg3 = 0;
            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            }
        }else if(dm.type==DEMO_NAV_END){
            Delay(clock_tid, 300);
            train_id = dm.arg1;

            // pm.type = PATH_GET_RANDOM_DEST;
            // pm.arg1 = train_id;
            // intended_reply_len = Send(pathfind_tid, &pm, sizeof(PathMessage), &dest, sizeof(uint8_t));
            // if(intended_reply_len!=sizeof(uint8_t)){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            // }
            // dest = get_random_dest();
            dest = dests[dest_index];
            dest_index += 1;
            tsm.type = TRAIN_SERVER_NAV;
            tsm.arg1 = train_id;
            tsm.arg2 = dest;
            tsm.arg3 = 0;
            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            }

        }else if(dm.type==DEMO_NAV_RETRY){
            train_id = dm.arg1;

            // pm.type = PATH_GET_RANDOM_DEST;
            // pm.arg1 = train_id;
            // intended_reply_len = Send(pathfind_tid, &pm, sizeof(PathMessage), &dest, sizeof(uint8_t));
            // if(intended_reply_len!=sizeof(uint8_t)){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            // }
            // dest = get_random_dest();
            
            dest = dests[dest_index];
            dest_index += 1;
            tsm.type = TRAIN_SERVER_NAV;
            tsm.arg1 = train_id;
            tsm.arg2 = dest;
            tsm.arg3 = 0;
            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Ktrainserver reverse cmd unexpected reply\0338");
            }

        }
    }
}