#include "rpi.h"
#include "tasks.h"

/*
questions:

*/

/*
things to store in register for running task
- current tid
- parent tid

*/

// TODO: prob change all these once we start syscall

uint32_t curTid;
uint32_t curPriority;
uint32_t curParentTid;


// void SystemCall(int operand){
  
// }

struct TaskFrame *currentTask;

int Create(int priority, void (*function)()){
  // allocate memory
  // generate tid for task
  // generate task object that includes function and memory and other relevent info
  // put object in call stack with priority
  // call svc
  
  struct TaskFrame *tf = getNextFreeTaskFrame();
  if(tf==NULL){
    return 0;
  }
  tf->tid = getNextTid();
  // TODO: change this prob
  tf->parentTid = curTid;
  tf->function = function;
  tf->priority = priority;
  tf->sp = getNextUserStackPointer();

  insertTaskFrame(tf);

  uart_printf(CONSOLE, "Created: %u\r\n", tf->tid);
  return 1;
}

int MyTid(){
  // return register storing current task tid
  // call svc

  // TODO: change
  return curTid;
}

int MyParentTid(){
  // return register storing current parent task tid
  // call svc

  // TODO: change
  return curParentTid;
}

void Yield(){
  // put current task in callstack with all info
  // call svc
}

void Exit(){
  // reclaim stack memory
  // maybe reclaim task frame?
  // call svc
  reclaimTaskFrame(currentTask);
}

void otherTask(){
  uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
  // Yield();
  // uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d", MyTid(), MyParentTid());
  Exit();
}

void rootTask(){
  uart_puts(CONSOLE, "Root Task\r\n");
  Create(1, &otherTask);
  Create(1, &otherTask);
  Create(2, &otherTask);
  Create(2, &otherTask);
  Exit();
}

struct TaskFrame *schedule(){
  uart_puts(CONSOLE, "Schedule\r\n");
  return popNextTaskFrame();
}

void activate(struct TaskFrame *tf){
  // TODO: change this to assembly
  void (*functionPtr)() = tf->function;
  curTid = tf->tid;
  curParentTid = tf->parentTid;
  // reclaimTaskFrame(tf);
  currentTask = tf;

  uart_puts(CONSOLE, "activate start\r\n");
  
  // functionPtr();

  // void (*tmp)() = &otherTask;

   asm volatile (
      "LDR x1, %0"
      : 
      : "m" (functionPtr)
      : "x1"
  );

  asm volatile("mov sp, %0" : : "r"(tf->sp));
  asm volatile (
      "BLR x1"
  );

  uart_puts(CONSOLE, "activate end\r\n");
}


int kmain() {
  // 590000
  uart_puts(CONSOLE, "Started\r\n");

  struct TaskFrame tfs[NUM_FREE_TASK_FRAMES];

  initializeTasks(tfs);
  if(Create(1,&rootTask)<0){
    return 0;
  }
  for(;;){
    struct TaskFrame *curtask = schedule();
    if(curtask != NULL) {
      activate(curtask);
      uart_puts(CONSOLE, "after activate\r\n");
    }
  }
  return 0;
}
