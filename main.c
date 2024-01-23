#include "rpi.h"
#include "exception.h"
#include "tasks.h"

#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5

struct TaskFrame *kernelTaskFrame;

int Create(int priority, void (*function)()){
    uart_printf(CONSOLE, "creating task \r\n");
  // allocate memory
  // generate tid for task
  // generate task object that includes function and memory and other relevent info
  // put object in call stack with priority
  // call svc
  
    
//   uart_printf(CONSOLE, "Created: %u\r\n", tf->tid);
  return 1;
}

int MyTid(){
    uart_printf(CONSOLE, "fetching tid \r\n");
    // TODO: make sys codes global
    int tid = -1;

    // volatile removes potentially bad compiler optimizations
    // "i" before SYS_CODE indicates to compiler that this is an integer constant
    // these are called asm operand constraints
    asm volatile(
        "svc %[aSYS_CODE]\n"
        "mov %[tid], x0\n"
        : [tid] "=r"(tid)
        : [aSYS_CODE] "i"(MY_TID) 
    );

    return tid;
}

int MyParentTid(){
    // TODO: make sys codes global
    uart_printf(CONSOLE, "fetching parent tid \r\n");

    int tid = -1;

    // volatile removes potentially bad compiler optimizations
    // "i" before SYS_CODE indicates to compiler that this is an integer constant
    // these are called asm operand constraints
    asm volatile(
        "svc %[SYS_CODE]\n"
        "mov %[tid], x0\n"
        : [tid] "=r"(tid)
        : [SYS_CODE] "" (MY_PARENT_TID) 
    );

    return tid;
}

void Yield(){
    // TODO: make sys codes global
    uart_printf(CONSOLE, "task yielding \r\n");

    // volatile removes potentially bad compiler optimizations
    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(YIELD) 
    );
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
    // TODO: make sys codes global
    uart_printf(CONSOLE, "task exiting \r\n");

    // volatile removes potentially bad compiler optimizations
    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(EXIT) 
    );

//   // assume system call and exited into kernel code here
//   reclaimTaskFrame(currentTask);
//   uart_puts(CONSOLE, "exit\r\n");

//   asm volatile("mov x0, %0" : : "r"(kernelTaskFrame->x[0]));
//   asm volatile("mov x1, %0" : : "r"(kernelTaskFrame->x[1]));
//   asm volatile("mov x2, %0" : : "r"(kernelTaskFrame->x[2]));
//   asm volatile("mov x3, %0" : : "r"(kernelTaskFrame->x[3]));
//   asm volatile("mov x4, %0" : : "r"(kernelTaskFrame->x[4]));
//   asm volatile("mov x5, %0" : : "r"(kernelTaskFrame->x[5]));
//   asm volatile("mov x6, %0" : : "r"(kernelTaskFrame->x[6]));
//   asm volatile("mov x7, %0" : : "r"(kernelTaskFrame->x[7]));
//   asm volatile("mov x8, %0" : : "r"(kernelTaskFrame->x[8]));
//   asm volatile("mov x9, %0" : : "r"(kernelTaskFrame->x[9]));
//   asm volatile("mov x10, %0" : : "r"(kernelTaskFrame->x[10]));
//   asm volatile("mov x11, %0" : : "r"(kernelTaskFrame->x[11]));
//   asm volatile("mov x12, %0" : : "r"(kernelTaskFrame->x[12]));
//   asm volatile("mov x13, %0" : : "r"(kernelTaskFrame->x[13]));
//   asm volatile("mov x14, %0" : : "r"(kernelTaskFrame->x[14]));
//   asm volatile("mov x15, %0" : : "r"(kernelTaskFrame->x[15]));
//   asm volatile("mov x16, %0" : : "r"(kernelTaskFrame->x[16]));
//   asm volatile("mov x17, %0" : : "r"(kernelTaskFrame->x[17]));
//   asm volatile("mov x18, %0" : : "r"(kernelTaskFrame->x[18]));
//   asm volatile("mov x19, %0" : : "r"(kernelTaskFrame->x[19]));
//   asm volatile("mov x20, %0" : : "r"(kernelTaskFrame->x[20]));
//   asm volatile("mov x21, %0" : : "r"(kernelTaskFrame->x[21]));
//   asm volatile("mov x22, %0" : : "r"(kernelTaskFrame->x[22]));
//   asm volatile("mov x23, %0" : : "r"(kernelTaskFrame->x[23]));
//   asm volatile("mov x24, %0" : : "r"(kernelTaskFrame->x[24]));
//   asm volatile("mov x25, %0" : : "r"(kernelTaskFrame->x[25]));
//   asm volatile("mov x26, %0" : : "r"(kernelTaskFrame->x[26]));
//   asm volatile("mov x27, %0" : : "r"(kernelTaskFrame->x[27]));
//   asm volatile("mov x28, %0" : : "r"(kernelTaskFrame->x[28]));
//   asm volatile("mov x29, %0" : : "r"(kernelTaskFrame->x[29]));
//   asm volatile("mov x30, %0" : : "r"(kernelTaskFrame->x[30]));
//   uart_puts(CONSOLE, "after kernel tf load\r\n");
//   asm volatile (
//     "LDR x1, %0"
//     : 
//     : "m" (kernelTaskFrame->pc)
//     : "x1"
//   );
//   asm volatile("mov fp, %0" : : "r"(kernelTaskFrame->fp));
//   asm volatile("mov sp, %0" : : "r"(kernelTaskFrame->sp));
//   asm volatile("mov lr, %0" : : "r"(kernelTaskFrame->lr));

//   asm volatile (
//       "BLR x1"
//   );

  // load kernel task frame
  // switch program counter to main loop
}

// void otherTask(){
//   uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
//   // Yield();
//   // uart_printf(CONSOLE, "MyTid: %d, MyParentTid: %d\r\n", MyTid(), MyParentTid());
// //   Exit();
// }

void rootTask(){
    uart_puts(CONSOLE, "Root Task\r\n");
    // int el = get_el();
    // uart_printf(CONSOLE, "Exception level: %u \r\n", el);
    // uart_printf(CONSOLE, "\r\n");
    

//   Create(1, &otherTask);
//   Create(1, &otherTask);
//   Create(2, &otherTask);
//   Create(2, &otherTask);
//   Exit();
}

struct TaskFrame *schedule(){
    // uart_printf(CONSOLE, "scheduling \r\n");
  return popNextTaskFrame();
}

void context_switch_to_task(struct TaskFrame *tf) {
    void (*functionPtr)() = tf->function;

    // save kernel task frame
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

    asm volatile("mov %0, fp" : "=r"(kernelTaskFrame->fp));
    asm volatile("mov %0, sp" : "=r"(kernelTaskFrame->sp));
    asm volatile("mov %0, lr" : "=r"(kernelTaskFrame->lr));
    asm volatile("mov %0, lr" : "=r"(kernelTaskFrame->pc)); // allows kernel resumption to resume at the end of the function

    uart_printf(CONSOLE, "TASK CONTEXT\r\n");
    // TODO: save ESR_EL1, ELR_EL1, SPSR_EL1 for kernel task

    // restore task context
    // asm volatile("mov x0, %0" : : "r"(tf->x[0]));
    // asm volatile("mov x1, %0" : : "r"(tf->x[1]));
    // asm volatile("mov x2, %0" : : "r"(tf->x[2]));
    // asm volatile("mov x3, %0" : : "r"(tf->x[3]));
    // asm volatile("mov x4, %0" : : "r"(tf->x[4]));
    // asm volatile("mov x5, %0" : : "r"(tf->x[5]));
    // asm volatile("mov x6, %0" : : "r"(tf->x[6]));
    // asm volatile("mov x7, %0" : : "r"(tf->x[7]));
    // asm volatile("mov x8, %0" : : "r"(tf->x[8]));
    // asm volatile("mov x9, %0" : : "r"(tf->x[9]));
    // asm volatile("mov x10, %0" : : "r"(tf->x[10]));
    // asm volatile("mov x11, %0" : : "r"(tf->x[11]));
    // asm volatile("mov x12, %0" : : "r"(tf->x[12]));
    // asm volatile("mov x13, %0" : : "r"(tf->x[13]));
    // asm volatile("mov x14, %0" : : "r"(tf->x[14]));
    // asm volatile("mov x15, %0" : : "r"(tf->x[15]));
    // asm volatile("mov x16, %0" : : "r"(tf->x[16]));
    // asm volatile("mov x17, %0" : : "r"(tf->x[17]));
    // asm volatile("mov x18, %0" : : "r"(tf->x[18]));
    // asm volatile("mov x19, %0" : : "r"(tf->x[19]));
    // asm volatile("mov x20, %0" : : "r"(tf->x[20]));
    // asm volatile("mov x21, %0" : : "r"(tf->x[21]));
    // asm volatile("mov x22, %0" : : "r"(tf->x[22]));
    // asm volatile("mov x23, %0" : : "r"(tf->x[23]));
    // asm volatile("mov x24, %0" : : "r"(tf->x[24]));
    // asm volatile("mov x25, %0" : : "r"(tf->x[25]));
    // asm volatile("mov x26, %0" : : "r"(tf->x[26]));
    // asm volatile("mov x27, %0" : : "r"(tf->x[27]));
    // asm volatile("mov x28, %0" : : "r"(tf->x[28]));
    // asm volatile("mov x29, %0" : : "r"(tf->x[29]));
    // asm volatile("mov x30, %0" : : "r"(tf->x[30]));
    // asm volatile("mov lr, %0" : : "r"(tf->lr));
    // asm volatile("mov fp, %0" : : "r"(tf->fp));
    // asm volatile("mov sp, %0" : : "r"(tf->sp));

    uint32_t test;

    // set SPSR?

    // switch pc to user task function
    asm volatile (
        "LDR x1, %0"
        : 
        : "m" (functionPtr)
    );
    asm volatile("msr elr_el1, x1");

    // setting sp_el0
    asm volatile("msr sp_el0, %0" : : "r"(tf->sp));

    // setting spsr_el1
    asm volatile("mov x0, %0" : : "r"(tf->spsr));
    asm volatile("msr spsr_el1, x0");

    asm volatile("mov lr, %0" : : "r"(tf->lr));
    asm volatile("mov fp, %0" : : "r"(tf->fp));
    asm volatile("mov sp, %0" : : "r"(tf->sp)); // sp here might not be sp_el0
    asm volatile("eret");
}

int run_task(struct TaskFrame *tf){
    uart_printf(CONSOLE, "running task tid: %u\r\n", tf->tid);

    context_switch_to_task(tf);
    // exit from exception
    uart_printf(CONSOLE, "back in kernel from exception tid: %u\r\n", tf->tid);

    // read exception code
    uint32_t syscall_operand;
    asm volatile("mov %0, x19" : "=r"(syscall_operand));

    // return exception code
    return syscall_operand;
}


void invalid_exception(uint32_t exception) {
    uart_printf(CONSOLE, "Invalid exception %u", exception);
    for (;;) {

    }
}

void exception_handler(uint32_t exception){
    uart_printf(CONSOLE, "Exception handler %u\r\n", exception);
    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint64_t operand = esr & 0xFULL;
    uart_printf(CONSOLE, "operand: %u\r\n", operand);
    if(operand==EXIT){
        // load kernel task frame
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
        
        // TODO: store operand in x19 for now.. not sure maybe a variable instead
        asm volatile("mov x19, %0" : : "r"(operand));

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
        asm volatile("mov sp, %0" : : "r"(kernelTaskFrame->sp));
        asm volatile("mov lr, %0" : : "r"(kernelTaskFrame->lr));
        asm volatile("mov fp, %0" : : "r"(kernelTaskFrame->fp));
    }
    // for (;;) {

    // }
}

int create_first_task(uint32_t priority, void (*function)()) {
    struct TaskFrame *tf = getNextFreeTaskFrame();
    if(tf==NULL){
        return 0;
    }
    tf->tid = getNextTid();
    // bits 3-0 must be 0 for ELOt stack pointer
    tf->spsr = 0x600002C0;
    // TODO: change this prob
    tf->parentTid = 0;
    tf->function = function;
    tf->priority = priority;
    tf->lr = &Exit;
    tf->x[30] = tf->lr;
    tf->sp = getNextUserStackPointer();
    tf->fp = tf->sp;
    tf->x[29] = tf->fp;

    insertTaskFrame(tf);
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
    
    // int el = get_el();
    // uart_printf(CONSOLE, "Exception level: %u \r\n", el);
    // uart_printf(CONSOLE, "\r\n");

    struct TaskFrame tfs[NUM_FREE_TASK_FRAMES+1];

    kernelTaskFrame = tfs;
    initializeTasks(tfs+1);

    if(create_first_task(1, &rootTask)<0){
        return 0;
    }

    for(;;){
        struct TaskFrame *cur_task = schedule();

        if (cur_task == NULL) {
            // TODO: low power state OR we should never return null --> idle task
            continue;
        }

        uart_printf(CONSOLE, "scheduled tid: %u\r\n", cur_task->tid);
        int exception_code = run_task(cur_task);
        // handle exception type
        uart_printf(CONSOLE, "EXCEPTION CODE %u\r\n", exception_code);
        if(exception_code==EXIT){
            reclaimTaskFrame(cur_task);
        }
    }
    return 0;
}
