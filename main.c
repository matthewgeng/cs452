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

struct TaskFrame *kernelTaskFrame;
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
  // assume system call and exited into kernel code here

  return currentTask->tid;
}

int MyParentTid(){
  // assume system call and exited into kernel code here

  return currentTask->parentTid;
}

void Yield(){
  // assume system call and exited into kernel code here
  // TODO: figure out if system call changes values and how to deal with that

  // save user task frame
  // asm volatile("mov %0, sp" : "=r"(currentTask->sp));
  // asm volatile("mov %0, lr" : "=r"(currentTask->lr));
  // asm volatile("mov %0, x0" : "=r"(currentTask->x[0]));
  // asm volatile("mov %0, x1" : "=r"(currentTask->x[1]));
  // asm volatile("mov %0, x2" : "=r"(currentTask->x[2]));
  // asm volatile("mov %0, x3" : "=r"(currentTask->x[3]));
  // asm volatile("mov %0, x4" : "=r"(currentTask->x[4]));
  // asm volatile("mov %0, x5" : "=r"(currentTask->x[5]));
  // asm volatile("mov %0, x6" : "=r"(currentTask->x[6]));
  // asm volatile("mov %0, x7" : "=r"(currentTask->x[7]));
  // asm volatile("mov %0, x8" : "=r"(currentTask->x[8]));
  // asm volatile("mov %0, x9" : "=r"(currentTask->x[9]));
  // asm volatile("mov %0, x10" : "=r"(currentTask->x[10]));
  // asm volatile("mov %0, x11" : "=r"(currentTask->x[11]));
  // asm volatile("mov %0, x12" : "=r"(currentTask->x[12]));
  // asm volatile("mov %0, x13" : "=r"(currentTask->x[13]));
  // asm volatile("mov %0, x14" : "=r"(currentTask->x[14]));
  // asm volatile("mov %0, x15" : "=r"(currentTask->x[15]));
  // asm volatile("mov %0, x16" : "=r"(currentTask->x[16]));
  // asm volatile("mov %0, x17" : "=r"(currentTask->x[17]));
  // asm volatile("mov %0, x18" : "=r"(currentTask->x[18]));
  // asm volatile("mov %0, x19" : "=r"(currentTask->x[19]));
  // asm volatile("mov %0, x20" : "=r"(currentTask->x[20]));
  // asm volatile("mov %0, x21" : "=r"(currentTask->x[21]));
  // asm volatile("mov %0, x22" : "=r"(currentTask->x[22]));
  // asm volatile("mov %0, x23" : "=r"(currentTask->x[23]));
  // asm volatile("mov %0, x24" : "=r"(currentTask->x[24]));
  // asm volatile("mov %0, x25" : "=r"(currentTask->x[25]));
  // asm volatile("mov %0, x26" : "=r"(currentTask->x[26]));
  // asm volatile("mov %0, x27" : "=r"(currentTask->x[27]));
  // asm volatile("mov %0, x28" : "=r"(currentTask->x[28]));
  // asm volatile("mov %0, x29" : "=r"(currentTask->x[29]));
  // asm volatile("mov %0, x30" : "=r"(currentTask->x[30]));

  // add task frame back to scheduler
  // load kernel task frame
  // switch program counter to main loop
}

void Exit(){
  // assume system call and exited into kernel code here
  reclaimTaskFrame(currentTask);
  uart_puts(CONSOLE, "exit\r\n");

  asm volatile("mov x0, %0" : : "r"(kernelTaskFrame->x[0]));
  asm volatile("mov x1, %0" : : "r"(kernelTaskFrame->x[1]));
  asm volatile("mov x2, %0" : : "r"(kernelTaskFrame->x[2]));
  asm volatile("mov x3, %0" : : "r"(kernelTaskFrame->x[3]));
  asm volatile("mov x4, %0" : : "r"(kernelTaskFrame->x[4]));
  asm volatile("mov x5, %0" : : "r"(kernelTaskFrame->x[5]));
  asm volatile("mov x6, %0" : : "r"(kernelTaskFrame->x[6]));
  asm volatile("mov x7, %0" : : "r"(kernelTaskFrame->x[7]));
  asm volatile("mov x8, %0" : : "r"(kernelTaskFrame->x[8]));
  asm volatile("mov x9, %0" : : "r"(kernelTaskFrame->x[9]));
  asm volatile("mov x10, %0" : : "r"(kernelTaskFrame->x[10]));
  asm volatile("mov x11, %0" : : "r"(kernelTaskFrame->x[11]));
  asm volatile("mov x12, %0" : : "r"(kernelTaskFrame->x[12]));
  asm volatile("mov x13, %0" : : "r"(kernelTaskFrame->x[13]));
  asm volatile("mov x14, %0" : : "r"(kernelTaskFrame->x[14]));
  asm volatile("mov x15, %0" : : "r"(kernelTaskFrame->x[15]));
  asm volatile("mov x16, %0" : : "r"(kernelTaskFrame->x[16]));
  asm volatile("mov x17, %0" : : "r"(kernelTaskFrame->x[17]));
  asm volatile("mov x18, %0" : : "r"(kernelTaskFrame->x[18]));
  asm volatile("mov x19, %0" : : "r"(kernelTaskFrame->x[19]));
  asm volatile("mov x20, %0" : : "r"(kernelTaskFrame->x[20]));
  asm volatile("mov x21, %0" : : "r"(kernelTaskFrame->x[21]));
  asm volatile("mov x22, %0" : : "r"(kernelTaskFrame->x[22]));
  asm volatile("mov x23, %0" : : "r"(kernelTaskFrame->x[23]));
  asm volatile("mov x24, %0" : : "r"(kernelTaskFrame->x[24]));
  asm volatile("mov x25, %0" : : "r"(kernelTaskFrame->x[25]));
  asm volatile("mov x26, %0" : : "r"(kernelTaskFrame->x[26]));
  asm volatile("mov x27, %0" : : "r"(kernelTaskFrame->x[27]));
  asm volatile("mov x28, %0" : : "r"(kernelTaskFrame->x[28]));
  asm volatile("mov x29, %0" : : "r"(kernelTaskFrame->x[29]));
  asm volatile("mov x30, %0" : : "r"(kernelTaskFrame->x[30]));
  uart_puts(CONSOLE, "after kernel tf load\r\n");
  asm volatile (
    "LDR x1, %0"
    : 
    : "m" (kernelTaskFrame->pc)
    : "x1"
  );
  asm volatile("mov sp, %0" : : "r"(kernelTaskFrame->sp));
  asm volatile("mov lr, %0" : : "r"(kernelTaskFrame->lr));

  asm volatile (
      "BLR x1"
  );

  // load kernel task frame
  // switch program counter to main loop
}

void otherTask(){
  uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
  // Yield();
  // uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
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
  return popNextTaskFrame();
}

void load_sp_and_lr(uint64_t sp, uint64_t lr){
  asm volatile("mov sp, %0" : : "r"(sp));
  asm volatile("mov lr, %0" : : "r"(lr));
}

void activate(struct TaskFrame *tf){
  // TODO: change this to assembly
  void (*functionPtr)() = tf->function;
  curTid = tf->tid;
  curParentTid = tf->parentTid;
  // reclaimTaskFrame(tf);
  currentTask = tf;

  uart_puts(CONSOLE, "activate start\r\n");

  // save kernel task frame
  asm volatile("mov %0, sp" : "=r"(kernelTaskFrame->sp));
  asm volatile("mov %0, lr" : "=r"(kernelTaskFrame->lr));
  asm volatile("mov %0, x0" : "=r"(kernelTaskFrame->x[0]));
  asm volatile("mov %0, x1" : "=r"(kernelTaskFrame->x[1]));
  asm volatile("mov %0, x2" : "=r"(kernelTaskFrame->x[2]));
  asm volatile("mov %0, x3" : "=r"(kernelTaskFrame->x[3]));
  asm volatile("mov %0, x4" : "=r"(kernelTaskFrame->x[4]));
  asm volatile("mov %0, x5" : "=r"(kernelTaskFrame->x[5]));
  asm volatile("mov %0, x6" : "=r"(kernelTaskFrame->x[6]));
  asm volatile("mov %0, x7" : "=r"(kernelTaskFrame->x[7]));
  asm volatile("mov %0, x8" : "=r"(kernelTaskFrame->x[8]));
  asm volatile("mov %0, x9" : "=r"(kernelTaskFrame->x[9]));
  asm volatile("mov %0, x10" : "=r"(kernelTaskFrame->x[10]));
  asm volatile("mov %0, x11" : "=r"(kernelTaskFrame->x[11]));
  asm volatile("mov %0, x12" : "=r"(kernelTaskFrame->x[12]));
  asm volatile("mov %0, x13" : "=r"(kernelTaskFrame->x[13]));
  asm volatile("mov %0, x14" : "=r"(kernelTaskFrame->x[14]));
  asm volatile("mov %0, x15" : "=r"(kernelTaskFrame->x[15]));
  asm volatile("mov %0, x16" : "=r"(kernelTaskFrame->x[16]));
  asm volatile("mov %0, x17" : "=r"(kernelTaskFrame->x[17]));
  asm volatile("mov %0, x18" : "=r"(kernelTaskFrame->x[18]));
  asm volatile("mov %0, x19" : "=r"(kernelTaskFrame->x[19]));
  asm volatile("mov %0, x20" : "=r"(kernelTaskFrame->x[20]));
  asm volatile("mov %0, x21" : "=r"(kernelTaskFrame->x[21]));
  asm volatile("mov %0, x22" : "=r"(kernelTaskFrame->x[22]));
  asm volatile("mov %0, x23" : "=r"(kernelTaskFrame->x[23]));
  asm volatile("mov %0, x24" : "=r"(kernelTaskFrame->x[24]));
  asm volatile("mov %0, x25" : "=r"(kernelTaskFrame->x[25]));
  asm volatile("mov %0, x26" : "=r"(kernelTaskFrame->x[26]));
  asm volatile("mov %0, x27" : "=r"(kernelTaskFrame->x[27]));
  asm volatile("mov %0, x28" : "=r"(kernelTaskFrame->x[28]));
  asm volatile("mov %0, x29" : "=r"(kernelTaskFrame->x[29]));
  asm volatile("mov %0, x30" : "=r"(kernelTaskFrame->x[30]));
  asm volatile(
    "BL 1f \n\t"
    "1: \n\t"
    "MOV %0, LR"
    : "=r" (kernelTaskFrame->pc)
  );
  uart_puts(CONSOLE, "after kernel tf saved\r\n");


  // switch pc to user task function
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
  uart_puts(CONSOLE, "back to activate\r\n");

  // load kernel task frame
  asm volatile("mov sp, %0" : : "r"(kernelTaskFrame->sp));
  asm volatile("mov lr, %0" : : "r"(kernelTaskFrame->lr));
  asm volatile("mov x0, %0" : : "r"(kernelTaskFrame->x[0]));
  asm volatile("mov x1, %0" : : "r"(kernelTaskFrame->x[1]));
  asm volatile("mov x2, %0" : : "r"(kernelTaskFrame->x[2]));
  asm volatile("mov x3, %0" : : "r"(kernelTaskFrame->x[3]));
  asm volatile("mov x4, %0" : : "r"(kernelTaskFrame->x[4]));
  asm volatile("mov x5, %0" : : "r"(kernelTaskFrame->x[5]));
  asm volatile("mov x6, %0" : : "r"(kernelTaskFrame->x[6]));
  asm volatile("mov x7, %0" : : "r"(kernelTaskFrame->x[7]));
  asm volatile("mov x8, %0" : : "r"(kernelTaskFrame->x[8]));
  asm volatile("mov x9, %0" : : "r"(kernelTaskFrame->x[9]));
  asm volatile("mov x10, %0" : : "r"(kernelTaskFrame->x[10]));
  asm volatile("mov x11, %0" : : "r"(kernelTaskFrame->x[11]));
  asm volatile("mov x12, %0" : : "r"(kernelTaskFrame->x[12]));
  asm volatile("mov x13, %0" : : "r"(kernelTaskFrame->x[13]));
  asm volatile("mov x14, %0" : : "r"(kernelTaskFrame->x[14]));
  asm volatile("mov x15, %0" : : "r"(kernelTaskFrame->x[15]));
  asm volatile("mov x16, %0" : : "r"(kernelTaskFrame->x[16]));
  asm volatile("mov x17, %0" : : "r"(kernelTaskFrame->x[17]));
  asm volatile("mov x18, %0" : : "r"(kernelTaskFrame->x[18]));
  asm volatile("mov x19, %0" : : "r"(kernelTaskFrame->x[19]));
  asm volatile("mov x20, %0" : : "r"(kernelTaskFrame->x[20]));
  asm volatile("mov x21, %0" : : "r"(kernelTaskFrame->x[21]));
  asm volatile("mov x22, %0" : : "r"(kernelTaskFrame->x[22]));
  asm volatile("mov x23, %0" : : "r"(kernelTaskFrame->x[23]));
  asm volatile("mov x24, %0" : : "r"(kernelTaskFrame->x[24]));
  asm volatile("mov x25, %0" : : "r"(kernelTaskFrame->x[25]));
  asm volatile("mov x26, %0" : : "r"(kernelTaskFrame->x[26]));
  asm volatile("mov x27, %0" : : "r"(kernelTaskFrame->x[27]));
  asm volatile("mov x28, %0" : : "r"(kernelTaskFrame->x[28]));
  asm volatile("mov x29, %0" : : "r"(kernelTaskFrame->x[29]));
  asm volatile("mov x30, %0" : : "r"(kernelTaskFrame->x[30]));

  uart_puts(CONSOLE, "activate end\r\n");
}


int kmain() {
  // 590000
  uart_puts(CONSOLE, "Started\r\n");

  struct TaskFrame tfs[NUM_FREE_TASK_FRAMES+1];

  kernelTaskFrame = tfs;
  initializeTasks(tfs+1);
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
