#include "rpi.h"
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
  // unsigned int counter = 1;
  for (;;) {
  //   uart_printf(CONSOLE, "PI[%u]> ", counter++);
  //   for (;;) {
  //     char c = uart_getc(CONSOLE);
  //     uart_putc(CONSOLE, c);
  //     if (c == '\r') {
  //       uart_putc(CONSOLE, '\n');
  //       break;
  //     } else if (c == 'q') {
  //       uart_puts(CONSOLE, "\r\n");
  //       return;
  //     }
  //   }
  }
}

int kmain() {
  if(Create(1,rootTask)<0){
    return 0;
  }
  mainloop();

  // exit to boot loader
  return 0;
}