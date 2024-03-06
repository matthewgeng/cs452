#include "pathfinding.h"
#include "syscall.h"
#include "rpi.h"
#include "traincontrol.h"
#include "nameserver.h"

int get_switches_setup(PathMessage *pm, uint8_t *switches_setup){

    //TODO: start pathfinding maybe 2/3 sensors after so the train doesn't go off course

    

}

void change_switches(int cout, int mio, int num_switch_changes, uint8_t *switches_setup){
    int switch_num;
    for(int i = 0; i<num_switch_changes; i++){
        switch_num = switches_setup[i];
        if(switch_num<=20){
            if(switch_num>16){
                switch_num += 136;
            }
            sw(cout, mio, switch_num, 'S');
        }else{
            switch_num /= 2;
            if(switch_num>32){
                switch_num += 136;
            }
            sw(cout, mio, switch_num, 'C');
        }
    }
}

void path_finding(){

    int cout = WhoIs("cout");
    int mio = WhoIs("mio");

    PathMessage pm;
    int tid;
    int intended_send_len;
    uint8_t switches_setup[21];
    int num_switch_changes;
    int switch_num;
    for(;;){
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        if(intended_send_len!=sizeof(pm)){
            uart_printf(CONSOLE, "path finding received unknown object");
        }
        Reply(tid, NULL, 0);

        num_switch_changes = get_switches_setup(&pm, switches_setup);

        change_switches(cout, mio, num_switch_changes, switches_setup);
    }
}