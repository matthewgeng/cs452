#include "tasks.h"
#include "rpi.h"
#include "nameserver.h"
#include "clock.h"
#include "syscall.h"
#include "taskframe.h"
#include "timer.h"
#include "io.h"

void idle_task(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Idle Task\r\n");
    #endif 
    for(;;){
        uint32_t idle_time_percent = (*p_idle_task_total*100)/(sys_time() - *p_program_start);
        uart_printf(CONSOLE, "\0337\033[3;1HIdle percentage: %u%% \0338", idle_time_percent);
        // uart_printf(CONSOLE, "Idle percentage: %u%%\r\n", idle_time_percent);
        asm volatile("wfi");
    }
}

void time_user() {
    int tid = MyTid();
    int parent = MyParentTid();
    int clock = WhoIs("clock"); // TODO: make every server have a function to get name

    char data[2];
    Send(parent, NULL, 0, data, sizeof(data));
    char delay = data[0];
    char num_delay = data[1];

    for (int i = 0; i < num_delay; i++) {
        Delay(clock, delay);
        uart_printf(CONSOLE, "tid: %u, delay: %u, completed: %u\r\n", tid, delay, i+1);
    }
}

void k3(){
    int user1 = Create(3, &time_user);
    int user2 = Create(4, &time_user);
    int user3 = Create(5, &time_user);
    int user4 = Create(6, &time_user);

    Receive(NULL, NULL, 0);
    char user1_data[2] = {10, 20};
    Reply(user1, user1_data, sizeof(user1_data));

    Receive(NULL, NULL, 0);
    char user2_data[2] = {23, 9};
    Reply(user2, user2_data, sizeof(user2_data));

    Receive(NULL, NULL, 0);
    char user3_data[2] = {33, 6};
    Reply(user3, user3_data, sizeof(user3_data));

    Receive(NULL, NULL, 0);
    char user4_data[2] = {71, 3};
    Reply(user4, user4_data, sizeof(user4_data));
}

void k4(){
    int cout = WhoIs("cout");
    int clock = WhoIs("clock"); // TODO: make every server have a function to get name
    // Puts(cout, 0, "running k4\r\n");

    // Putc(cout, 0, 'h');
    // Puts(cout, 0, "afafafafaf\r\n");
    // Delay(clock, 5900);
    // Puts(cout, 0, "after 59 seconds\r\n");
    // Delay(clock, 100);
    // Puts(cout, 0, "after 60 seconds\r\n");
}


void rootTask(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Root Task\r\n");
    #endif 
    // order or nameserver and idle_task matters since their tid values are assumed in implementation
    Create(2, &nameserver);
    Create(1000, &idle_task);
    Create(0, &notifier);
    Create(1, &clock);

    Create(3, &console_out_notifier);
    // Create(3, &console_in_notifier);
    Create(3, &console_out);
    // Create(3, &console_in);

    // Create(3, &marklin_out_notifier);
    // Create(3, &marklin_in_notifier);
    // Create(3, &marklin_io);
    
    Create(4, &k4);
    
    // uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}
