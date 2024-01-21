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

/*
obj types:

priority stack
- list of tasks

task: 
- tid
- parent tid
- x0-x31
- program counter
- stack base pointer
- maybe stack size?
- fr (frame register)?
- lr (link register)?
- function


all these functions should call SVC in order to context switch 

sp of a task starts as NULL and when it runs, if NULL, run function; if not null, pop those registers 
*/



// TODO: prob change all these once we start syscall

uint32_t curTid;
uint32_t curPriority;
uint32_t curParentTid;


// void SystemCall(int operand){
  
// }

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
  // TODO: sp?

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
}

void otherTask(){
  uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
  // Yield();
  // uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d", MyTid(), MyParentTid());
  Exit();
}

void rootTask(){
  uart_puts(CONSOLE, "Root Task\r\n");
  Create(1, otherTask);
  Create(1, otherTask);
  Create(2, otherTask);
  Create(2, otherTask);
  Exit();
}

struct TaskFrame *schedule(){
  return popNextTaskFrame();
}

void activate(struct TaskFrame *tf){
  // TODO: change this to assembly
  void (*function)() = tf->function;
  curTid = tf->tid;
  curParentTid = tf->parentTid;
  reclaimTaskFrame(tf);
  function();
}


int kmain() {
  // 590000
  uart_puts(CONSOLE, "Started\r\n");

  struct TaskFrame tfs[NUM_FREE_TASK_FRAMES];

  initializeTasks(tfs);
  if(Create(1,rootTask)<0){
    return 0;
  }
  for(;;){
    struct TaskFrame *curtask = schedule();
    if(curtask != NULL) {
      activate(curtask);
    }
  }
  return 0;
}




/*
#include "memory.h"

static uint32_t stackSize = 1000;
static uint32_t heapSize = 1000;
static uint32_t kernelHeapSize = 5000;

static uint32_t nextUserStackBaseAddr = 580000;
static uint32_t nextUserHeapBaseAddr = 565000;
static uint32_t kernelHeapBaseAddr = 570000;

uint32_t getKernelHeapPointer(){
    return kernelHeapBaseAddr;
}

uint32_t getNextUserStackPointer(){
    nextUserStackBaseAddr-=stackSize;
    return nextUserHeapBaseAddr + stackSize;
}

uint32_t getNextUserHeapPointer(){
    nextUserHeapBaseAddr-=heapSize;
    return nextUserHeapBaseAddr + heapSize;
}


struct list {
  struct list *next;
}

struct item {
  list items;
  int val;
}

item i1;
item i2;
i1->items = i2->items;


i2->val = i1->items->items+sizeof(list)

*/