#include "rpi.h"
#include "exception.h"

/*
questions:

*/

/*
things to store in register for running task
- current tid
- parent tid

*/

/*
obj types:

priority stack
- list of tasks

task: 
- tid
- parent tid
- stack base pointer
- maybe stack size?


all these functions should call SVC in order to context switch 
*/

int Create(int priority, void (*function)()){
  // allocate memory
  // generate tid for task
  // generate task object that includes function and memory and other relevent info
  // put object in call stack with priority
  uart_printf(CONSOLE, "Created: %u", 0);
  // call svc
  return 0;
}

int MyTid(){
  // return register storing current task tid
  // TODO: change
  // call svc
  return 0;
}

int MyParentTid(){
  // return register storing current parent task tid
  // TODO: change
  // call svc
  return 0;
}

void Yield(){
  // put current task in callstack with all info
  // call svc
}

void Exit(){
  // reclaim stack memory
  // call svc
}

void otherTask(){
  uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d", MyTid(), MyParentTid());
  Yield();
  uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d", MyTid(), MyParentTid());
  Exit();
}

void rootTask(){
  Create(1, otherTask);
  Create(1, otherTask);
  Create(2, otherTask);
  Create(2, otherTask);
}

static void mainloop() {

    // TODO: for the future, initialize idle task 

    // create root user task

    for (;;) {
        // pop from scheduler        

        


    }
}

void invalid_exception(uint32_t exception) {
    uart_printf(CONSOLE, "Invalid exception %u", exception);
    for (;;) {

    }
}

int kmain() {

    init_exception_vector(); 

    // set up GPIO pins for both console and marklin uarts
    gpio_init();

    // not strictly necessary, since line 1 is configured during boot
    // but we'll configure the line anyway, so we know what state it is in
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    uart_printf(CONSOLE, "Program starting\r\n");
    uart_printf(CONSOLE, "\r\n");
    
    int* p = 0;
    *p = &kmain;
    uart_printf(CONSOLE, "main function addr: %x \r\n", *p);

    *p = &exception_vector;
    uart_printf(CONSOLE, "Exception vector addr: %x \r\n", *p);

    *p = &invalid_exception;
    uart_printf(CONSOLE, "invalid exception addr: %x \r\n", *p);

    int el = get_el();
    uart_printf(CONSOLE, "Exception level: %u \r\n", el);
    uart_printf(CONSOLE, "\r\n");

    mainloop();
    uart_printf(CONSOLE, "Restarting \r\n");
    // exit to boot loader
    return 0;
}
