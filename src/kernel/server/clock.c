#include "clock.h"
#include "nameserver.h"
#include "heap.h"
#include "rpi.h"
#include "util.h"
#include "syscall.h"

int delayed_task_cmp(const DelayedTask* df1, const DelayedTask* df2) {
    if (df1->delay_until < df2->delay_until) {
        return -1;
    } else if (df1->delay_until < df2->delay_until) {
        return 1;
    }
    return 0;
}

DelayedTask *dts_init(DelayedTask* dts, size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            dts[i].next = dts+i+1;
        }else{
            dts[i].next = NULL;
        }
    }
    return dts;
}
DelayedTask *getNextFreeDelayedTask(DelayedTask **nextFreeDelayedTask){
    if(*nextFreeDelayedTask==NULL){
        return NULL;
    }
    DelayedTask *dt = *nextFreeDelayedTask;
    *nextFreeDelayedTask = (*nextFreeDelayedTask)->next;
    return dt;
}

void reclaimDelayedTask(DelayedTask **nextFreeDelayedTask, DelayedTask *dt){
    dt->next = *nextFreeDelayedTask;
    *nextFreeDelayedTask = dt;
}


int Time(int tid){
    #if DEBUG
        uart_dprintf(CONSOLE, "Time called\r\n");
    #endif 
    char msg[1] = {'t'};
    uint32_t tick;
    int intended_reply_len = Send(tid, msg, 1, (char*)&tick, sizeof(tick));
    if(intended_reply_len < 0){
        return -1;
    }
    if(intended_reply_len != 4){
        uart_printf(CONSOLE, "\x1b[31mTime unexpected behaviour %d\x1b[0m\r\n", intended_reply_len);
        for(;;){}
    }
    return tick;
}

int Delay(int tid, int ticks){
    if(ticks<0){
        return -2;
    }
    char msg[5];
    msg[0] = 'd';
    // time_to_char_arr(ticks, msg+1);
    // uart_printf(CONSOLE, "%u, %u, %u, %u, cast val= %u\r\n", msg[1], msg[2], msg[3], msg[4], *(uint32_t*)(msg+1));
    memcpy(msg + 1, (char*)&ticks, sizeof(ticks));
    int tick;

    int intended_reply_len = Send(tid, msg, sizeof(msg), (char*)&tick, sizeof(tick));

    // invalid send 
    if(intended_reply_len < 0){
        return -1;
    }

    // negative delay code
    if (tick == -2) {
        return tick;
    }

    if(intended_reply_len != 4){
        uart_printf(CONSOLE, "\x1b[31mTime unexpected behaviour %d\x1b[0m\r\n", intended_reply_len);
        for(;;){}
    }
    return tick;
}

int DelayUntil(int tid, int ticks){
    if(ticks<0){
        return -2;
    }
    char msg[5];
    msg[0] = 'u';
    // time_to_char_arr(ticks, msg+1);
    memcpy(msg + 1, (char*)&ticks, sizeof(ticks));
    int tick;

    int intended_reply_len = Send(tid, msg, sizeof(msg), (char*)&tick, sizeof(tick));
    
    // invalid send 
    if(intended_reply_len < 0){
        return -1;
    }

    // negative delay code
    if (tick == -2) {
        return tick;
    }

    if(intended_reply_len != 4){
        uart_printf(CONSOLE, "\x1b[31mTime unexpected behaviour %d\x1b[0m\r\n", intended_reply_len);
        for(;;){}
    }
    return tick;
}

void clock_notifier(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Running notifier server \r\n");
    #endif 
    RegisterAs("notifier\0");
    int clock_server_tid = WhoIs("clock\0");
    for(;;){
        AwaitEvent(CLOCK);
        int intended_reply_len = Send(clock_server_tid, NULL, 0, NULL, 0);
        if(intended_reply_len!=0){
            uart_printf(CONSOLE, "\x1b[31mClock notifier unexpected behaviour %d\x1b[0m\r\n", intended_reply_len);
            for(;;){}
        }
    }
}

void clock(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Running clock server \r\n");
    #endif 
    RegisterAs("clock\0");
    int notifier_tid = WhoIs("notifier\0");
    if(notifier_tid<0){
        uart_printf(CONSOLE, "\x1b[31mCannot get notifier tid\x1b[0m\r\n");
        for(;;){}
    }

    DelayedTask free_dts[MAX_NUM_TASKS];
    DelayedTask *nextFreeDelayedTask = dts_init(free_dts, MAX_NUM_TASKS);

    Heap sorted_delayed_tasks;
    DelayedTask* dts[MAX_NUM_TASKS];
    heap_init(&sorted_delayed_tasks, dts, MAX_NUM_TASKS, delayed_task_cmp);
    int neg_delay_code = -2;

    char msg[5];
    int tid;
    uint32_t tick = 0;
    for(;;){
        int msglen = Receive(&tid, msg, 5);

        if(tid == notifier_tid){
            tick++;
            DelayedTask *next_dt = heap_peek(&sorted_delayed_tasks);
            if (next_dt->tid == 15) {
                uart_printf(CONSOLE, "\0337\033[48;1H\033[K sensor tid wait until %d, curtick %d\0338", next_dt->delay_until, tick);
            }

            while(next_dt!=NULL && next_dt->delay_until<=tick){
                heap_pop(&sorted_delayed_tasks);
                // DelayedTask *popped = heap_pop(&sorted_delayed_tasks);
                // uart_printf(CONSOLE, "Popped tid %u with delay until %u\r\n", popped->tid, popped->delay_until);
                // uart_printf(CONSOLE, "Clock replying to %u with delay until %u\r\n", next_dt->tid, next_dt->delay_until);
                Reply(next_dt->tid, (char*)&tick, sizeof(tick));
                reclaimDelayedTask(&nextFreeDelayedTask, next_dt);
                next_dt = heap_peek(&sorted_delayed_tasks);
            }
            Reply(notifier_tid, NULL, 0);
        }else{
            if(msg[0]=='t'){
                if(msglen!=1){
                    uart_printf(CONSOLE, "\x1b[Invalid time request to clock server\x1b[0m\r\n");
                    for(;;){}
                }
                Reply(tid, (char*)&tick, sizeof(tick));
            }else if(msg[0]=='d'){
                if(msglen!=5){
                    uart_printf(CONSOLE, "\x1b[Invalid delay request to clock server\x1b[0m\r\n");
                    Reply(tid, (char*)&tick, sizeof(tick));
                    // for(;;){}
                }
                // uint32_t delay = char_array_to_time(msg+1);
                uint32_t delay = *(uint32_t*)(msg+1);
                if (delay < 0) {
                    Reply(tid, (char*)&neg_delay_code, sizeof(neg_delay_code));
                    continue;
                }

                DelayedTask *dt = getNextFreeDelayedTask(&nextFreeDelayedTask);
                dt->tid = tid;
                dt->delay_until = tick+delay;
                heap_push(&sorted_delayed_tasks, dt);
            }else if(msg[0]=='u'){
                if(msglen!=5){
                    uart_printf(CONSOLE, "\x1b[Invalid delay_until request to clock server\x1b[0m\r\n");
                    Reply(tid, (char*)&tick, sizeof(tick));
                    // for(;;){}
                }
                uint32_t time = *(uint32_t*)(msg+1);

                if (time < 0) {
                    Reply(tid, (char*)&neg_delay_code, sizeof(neg_delay_code));
                    continue;
                }

                DelayedTask *dt = getNextFreeDelayedTask(&nextFreeDelayedTask);
                dt->tid = tid;
                dt->delay_until = time;
                heap_push(&sorted_delayed_tasks, dt);
            }else{
                uart_printf(CONSOLE, "\x1b[31mInvalid msg received by clock server\x1b[0m\r\n");
                for(;;){}
            }
        }
        
    }
}
