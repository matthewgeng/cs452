#include "rpi.h"
#include "exception.h"
#include "tasks.h"
#include "heap.h"

#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5

TaskFrame *kf = 0;
TaskFrame *currentTaskFrame = 0;
int message;

void invalid_exception(uint32_t exception) {
    uart_printf(CONSOLE, "Invalid exception %u", exception);
    for (;;) {}
}

void exception_handler(uint32_t exception){

    // uint32_t test;
    // asm volatile("mov %0, x12" : "=r"(test));
    // uart_printf(CONSOLE, "\r\nhandler x12: %u\r\n", test);
    // asm volatile("mov %0, x11" : "=r"(test));
    // uart_printf(CONSOLE, "handler x11: %u\r\n", test);

    // store user task registers
    asm volatile("mov %0, x0" : "=r"(currentTaskFrame->x[0]));
    asm volatile("mov %0, x1" : "=r"(currentTaskFrame->x[1]));
    asm volatile("mov %0, x2" : "=r"(currentTaskFrame->x[2]));
    asm volatile("mov %0, x3" : "=r"(currentTaskFrame->x[3]));
    asm volatile("mov %0, x4" : "=r"(currentTaskFrame->x[4]));
    asm volatile("mov %0, x5" : "=r"(currentTaskFrame->x[5]));
    asm volatile("mov %0, x6" : "=r"(currentTaskFrame->x[6]));
    asm volatile("mov %0, x7" : "=r"(currentTaskFrame->x[7]));
    asm volatile("mov %0, x8" : "=r"(currentTaskFrame->x[8]));
    asm volatile("mov %0, x9" : "=r"(currentTaskFrame->x[9]));
    asm volatile("mov %0, x10" : "=r"(currentTaskFrame->x[10]));
    asm volatile("mov %0, x11" : "=r"(currentTaskFrame->x[11]));
    asm volatile("mov %0, x12" : "=r"(currentTaskFrame->x[12]));
    asm volatile("mov %0, x13" : "=r"(currentTaskFrame->x[13]));
    asm volatile("mov %0, x14" : "=r"(currentTaskFrame->x[14]));
    asm volatile("mov %0, x15" : "=r"(currentTaskFrame->x[15]));
    asm volatile("mov %0, x16" : "=r"(currentTaskFrame->x[16]));
    asm volatile("mov %0, x17" : "=r"(currentTaskFrame->x[17]));
    asm volatile("mov %0, x18" : "=r"(currentTaskFrame->x[18]));
    asm volatile("mov %0, x19" : "=r"(currentTaskFrame->x[19]));
    asm volatile("mov %0, x20" : "=r"(currentTaskFrame->x[20]));
    asm volatile("mov %0, x21" : "=r"(currentTaskFrame->x[21]));
    asm volatile("mov %0, x22" : "=r"(currentTaskFrame->x[22]));
    asm volatile("mov %0, x23" : "=r"(currentTaskFrame->x[23]));
    asm volatile("mov %0, x24" : "=r"(currentTaskFrame->x[24]));
    asm volatile("mov %0, x25" : "=r"(currentTaskFrame->x[25]));
    asm volatile("mov %0, x26" : "=r"(currentTaskFrame->x[26]));
    asm volatile("mov %0, x27" : "=r"(currentTaskFrame->x[27]));
    asm volatile("mov %0, x28" : "=r"(currentTaskFrame->x[28]));
    asm volatile("mov %0, x29" : "=r"(currentTaskFrame->x[29]));
    // asm volatile("mov %0, x30" : "=r"(currentTaskFrame->x[30]));
    asm volatile("mrs %0, elr_el1" : "=r"(currentTaskFrame->x[30]));

    // asm volatile("mov %0, fp" : "=r"(currentTaskFrame->fp));
    asm volatile("mrs %0, sp_el0" : "=r"(currentTaskFrame->sp));
    // asm volatile("mrs %0, elr_el1" : "=r"(currentTaskFrame->lr));
    asm volatile("mrs %0, elr_el1" : "=r"(currentTaskFrame->pc));
    asm volatile("mrs %0, spsr_el1" : "=r"(currentTaskFrame->spsr));



    // uart_printf(CONSOLE, "Exception handler %u\r\n", exception);


    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint64_t operand = esr & 0xFULL;
    // uart_printf(CONSOLE, "operand: %u\r\n", operand);

    // load kernel registers
    asm volatile("mov x0, %0" : : "r"(kf->x[0]));
    asm volatile("mov x1, %0" : : "r"(kf->x[1]));
    asm volatile("mov x2, %0" : : "r"(kf->x[2]));
    asm volatile("mov x3, %0" : : "r"(kf->x[3]));
    asm volatile("mov x4, %0" : : "r"(kf->x[4]));
    asm volatile("mov x5, %0" : : "r"(kf->x[5]));
    asm volatile("mov x6, %0" : : "r"(kf->x[6]));
    asm volatile("mov x7, %0" : : "r"(kf->x[7]));
    asm volatile("mov x8, %0" : : "r"(kf->x[8]));
    asm volatile("mov x9, %0" : : "r"(kf->x[9]));
    asm volatile("mov x10, %0" : : "r"(kf->x[10]));
    asm volatile("mov x11, %0" : : "r"(kf->x[11]));
    asm volatile("mov x12, %0" : : "r"(kf->x[12]));
    asm volatile("mov x13, %0" : : "r"(kf->x[13]));
    asm volatile("mov x14, %0" : : "r"(kf->x[14]));
    asm volatile("mov x15, %0" : : "r"(kf->x[15]));
    asm volatile("mov x16, %0" : : "r"(kf->x[16]));
    asm volatile("mov x17, %0" : : "r"(kf->x[17]));
    asm volatile("mov x18, %0" : : "r"(kf->x[18]));
    
    // TODO: store operand in x19 for now.. not sure maybe a variable instead
    asm volatile("mov x19, %0" : : "r"(operand));

    asm volatile("mov x20, %0" : : "r"(kf->x[20]));
    asm volatile("mov x21, %0" : : "r"(kf->x[21]));
    asm volatile("mov x22, %0" : : "r"(kf->x[22]));
    asm volatile("mov x23, %0" : : "r"(kf->x[23]));
    asm volatile("mov x24, %0" : : "r"(kf->x[24]));
    asm volatile("mov x25, %0" : : "r"(kf->x[25]));
    asm volatile("mov x26, %0" : : "r"(kf->x[26]));
    asm volatile("mov x27, %0" : : "r"(kf->x[27]));
    asm volatile("mov x28, %0" : : "r"(kf->x[28]));
    asm volatile("mov x29, %0" : : "r"(kf->x[29]));
    asm volatile("mov x30, %0" : : "r"(kf->x[30]));
    asm volatile("mov sp, %0" : : "r"(kf->sp));
    // asm volatile("mov lr, %0" : : "r"(kf->lr));
    // asm volatile("mov fp, %0" : : "r"(kf->fp));

    // uart_printf(CONSOLE, "kernel frame lr: %x\r\n", kf->lr);
    // uart_printf(CONSOLE, "kernel frame pc: %x\r\n", kf->pc);
    // uart_printf(CONSOLE, "kernel frame sp: %x\r\n", kf->sp);
}

int Create(int priority, void (*function)()){
    uart_printf(CONSOLE, "creating task \r\n");
  // allocate memory
  // generate tid for task
  // generate task object that includes function and memory and other relevent info
  // put object in call stack with priority
  // call svc
  
    
    // asm volatile(
    //     "svc %[aSYS_CODE]\n"
    //     "mov %[tid], x0\n"
    //     : [tid] "=r"(tid)
    //     : [aSYS_CODE] "i"(MY_TID) 
    // );
//   uart_printf(CONSOLE, "Created: %u\r\n", tf->tid);
  return 1;
}

int MyTid(){
    uart_printf(CONSOLE, "fetching tid \r\n");
    int tid;

    uint32_t lr;
    asm volatile("mov %0, lr" : "=r"(lr));
    uart_printf(CONSOLE, "local stored lr: %u\r\n", lr);

    uint32_t test;

    asm volatile("mov %0, lr" : "=r"(test));
    uart_printf(CONSOLE, "\r\n\r\nMyTid lr before: %u\r\n", test);
    // asm volatile("mov %0, sp" : "=r"(test));
    // uart_printf(CONSOLE, "MyTid sp before: %u\r\n", test);
    // asm volatile("mov %0, fp" : "=r"(test));
    // uart_printf(CONSOLE, "MyTid fp before: %u\r\n\r\n\r\n", test);

    // volatile removes potentially bad compiler optimizations
    // "i" before SYS_CODE indicates to compiler that this is an integer constant
    // these are called asm operand constraints


    // TODO: storing lr in x10 for now
    asm volatile("mov x12, lr");
    asm volatile("mov x11, fp");

    // asm volatile(
    //     "svc %[SYS_CODE]"
    //     :
    //     : [SYS_CODE] "i"(MY_TID) 
    // );

    uart_puts(CONSOLE, "Back to MyTid\r\n");
    uart_printf(CONSOLE, "test after: %u\r\n", test);
    uart_printf(CONSOLE, "local stored lr: %u\r\n", lr);
    asm volatile(
        "mov %[tid], x19\n"
        : [tid] "=r"(tid)
    );
    uart_printf(CONSOLE, "Inside MyTid tid: %d\r\n", tid);

    asm volatile("mov lr, x12");
    asm volatile("mov fp, x11");


    asm volatile("mov %0, lr" : "=r"(test));
    uart_printf(CONSOLE, "MyTid lr after: %u\r\n", test);
    asm volatile("mov %0, sp" : "=r"(test));
    uart_printf(CONSOLE, "MyTid sp after: %u\r\n", test);
    asm volatile("mov %0, fp" : "=r"(test));
    uart_printf(CONSOLE, "MyTid fp after: %u\r\n\r\n\r\n", test);

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

    uint32_t test;
    asm volatile("mov %0, x30" : "=r"(test));
    uart_printf(CONSOLE, "x30: %x\r\n", test);
    asm volatile("mov %0, lr" : "=r"(test));
    uart_printf(CONSOLE, "lr: %x\r\n", test);
    asm volatile("mov %0, sp" : "=r"(test));
    uart_printf(CONSOLE, "sp: %x\r\n", test);
    // asm volatile (
    //     // "ldp lr, x29, [sp], #16  \n"  // Load lr and x29 from the stack and increment sp
    //     "b x30                    \n"  // Branch to x30 (lr)
    // );

    // // volatile removes potentially bad compiler optimizations
    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(YIELD) 
    );

    // asm volatile("sub lr, lr, #28");
    // asm volatile("mov %0, lr" : "=r"(lr));
    // uart_printf(CONSOLE, "local stored lr after: %u\r\n", lr);

    uart_printf(CONSOLE, "back to yield \r\n");
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
    uart_printf(CONSOLE, "Root Task\r\n");

    // uint32_t fp, lr;
    uint32_t test;
    asm volatile("mov %0, x30" : "=r"(test));
    uart_printf(CONSOLE, "x30: %x\r\n", test);
    asm volatile("mov %0, lr" : "=r"(test));
    uart_printf(CONSOLE, "lr: %x\r\n", test);
    asm volatile("mov %0, sp" : "=r"(test));
    uart_printf(CONSOLE, "sp: %x\r\n", test);

    // asm volatile("ldp	%0, %1, [sp], #32" : "=r"(fp),"=r"(lr));
    // uart_printf(CONSOLE, "sp stored fp, lr : %x %x\r\n", fp, lr);

    Yield();

    // uint32_t test;
    // asm volatile("mov %0, lr" : "=r"(test));
    // uart_printf(CONSOLE, "local stored lr after: %x\r\n", test);
    // uart_puts(CONSOLE, "Root Task after yield\r\n");

    // uint32_t tid = MyTid();
    // uart_printf(CONSOLE, "Root task tid: %u\r\n", tid);

    // int el = get_el();
    // uart_printf(CONSOLE, "Exception level: %u \r\n", el);
    // uart_printf(CONSOLE, "\r\n");
    

//   Create(1, &otherTask);
//   Create(1, &otherTask);
//   Create(2, &otherTask);
//   Create(2, &otherTask);
}

void context_switch_to_task(TaskFrame *tf) {
    // save kernel task frame
    // asm volatile("stp x0, x1, [%0, #16 * 0]" : : "r"(&kf));
    // asm volatile("stp x2, x3, [%0, #16 * 1]" : : "r"(&kf));
    // asm volatile("stp x3, x4, [%0, #16 * 2]" : : "r"(&kf));
    // asm volatile("stp x4, x5, [%0, #16 * 3]" : : "r"(&kf));

    asm volatile("mov %0, x0" : "=r"(kf->x[0]));
    asm volatile("mov %0, x1" : "=r"(kf->x[1]));
    asm volatile("mov %0, x2" : "=r"(kf->x[2]));
    asm volatile("mov %0, x3" : "=r"(kf->x[3]));
    asm volatile("mov %0, x4" : "=r"(kf->x[4]));
    asm volatile("mov %0, x5" : "=r"(kf->x[5]));
    asm volatile("mov %0, x6" : "=r"(kf->x[6]));
    asm volatile("mov %0, x7" : "=r"(kf->x[7]));
    asm volatile("mov %0, x8" : "=r"(kf->x[8]));
    asm volatile("mov %0, x9" : "=r"(kf->x[9]));
    asm volatile("mov %0, x10" : "=r"(kf->x[10]));
    asm volatile("mov %0, x11" : "=r"(kf->x[11]));
    asm volatile("mov %0, x12" : "=r"(kf->x[12]));
    asm volatile("mov %0, x13" : "=r"(kf->x[13]));
    asm volatile("mov %0, x14" : "=r"(kf->x[14]));
    asm volatile("mov %0, x15" : "=r"(kf->x[15]));
    asm volatile("mov %0, x16" : "=r"(kf->x[16]));
    asm volatile("mov %0, x17" : "=r"(kf->x[17]));
    asm volatile("mov %0, x18" : "=r"(kf->x[18]));
    asm volatile("mov %0, x19" : "=r"(kf->x[19]));
    asm volatile("mov %0, x20" : "=r"(kf->x[20]));
    asm volatile("mov %0, x21" : "=r"(kf->x[21]));
    asm volatile("mov %0, x22" : "=r"(kf->x[22]));
    asm volatile("mov %0, x23" : "=r"(kf->x[23]));
    asm volatile("mov %0, x24" : "=r"(kf->x[24]));
    asm volatile("mov %0, x25" : "=r"(kf->x[25]));
    asm volatile("mov %0, x26" : "=r"(kf->x[26]));
    asm volatile("mov %0, x27" : "=r"(kf->x[27]));
    asm volatile("mov %0, x28" : "=r"(kf->x[28]));
    asm volatile("mov %0, x29" : "=r"(kf->x[29]));
    asm volatile("mov %0, x30" : "=r"(kf->x[30]));
    asm volatile("mov %0, sp" : "=r"(kf->sp));
    // asm volatile("mov %0, lr" : "=r"(kf->lr));
    // asm volatile("mov %0, lr" : "=r"(kf->pc)); // allows kernel resumption to resume at the end of the function
    // uart_printf(CONSOLE, "kernel frame lr: %x\r\n", kf->lr);
    // uart_printf(CONSOLE, "kernel frame pc: %x\r\n", kf->pc);
    // uart_printf(CONSOLE, "kernel frame sp: %x\r\n", kf->sp);

    // TODO: save ESR_EL1, ELR_EL1, SPSR_EL1 for kernel task

    // restore task context 
    asm volatile("mov x0, %0" : : "r"(tf->x[0]));
    asm volatile("mov x1, %0" : : "r"(tf->x[1]));
    asm volatile("mov x2, %0" : : "r"(tf->x[2]));
    asm volatile("mov x3, %0" : : "r"(tf->x[3]));
    asm volatile("mov x4, %0" : : "r"(tf->x[4]));
    asm volatile("mov x5, %0" : : "r"(tf->x[5]));
    asm volatile("mov x6, %0" : : "r"(tf->x[6]));
    asm volatile("mov x7, %0" : : "r"(tf->x[7]));
    asm volatile("mov x8, %0" : : "r"(tf->x[8]));
    asm volatile("mov x9, %0" : : "r"(tf->x[9]));
    asm volatile("mov x10, %0" : : "r"(tf->x[10]));
    asm volatile("mov x11, %0" : : "r"(tf->x[11]));
    asm volatile("mov x12, %0" : : "r"(tf->x[12]));
    asm volatile("mov x13, %0" : : "r"(tf->x[13]));
    asm volatile("mov x14, %0" : : "r"(tf->x[14]));
    asm volatile("mov x15, %0" : : "r"(tf->x[15]));
    asm volatile("mov x16, %0" : : "r"(tf->x[16]));
    asm volatile("mov x17, %0" : : "r"(tf->x[17]));
    asm volatile("mov x18, %0" : : "r"(tf->x[18]));

    // TODO: storing message in x19 for now..
    // if(message!=-1){
    //     asm volatile("mov x19, %0" : : "r"(message));
    // }

    asm volatile("mov x20, %0" : : "r"(tf->x[20]));
    asm volatile("mov x21, %0" : : "r"(tf->x[21]));
    asm volatile("mov x22, %0" : : "r"(tf->x[22]));
    asm volatile("mov x23, %0" : : "r"(tf->x[23]));
    asm volatile("mov x24, %0" : : "r"(tf->x[24]));
    asm volatile("mov x25, %0" : : "r"(tf->x[25]));
    asm volatile("mov x26, %0" : : "r"(tf->x[26]));
    asm volatile("mov x27, %0" : : "r"(tf->x[27]));
    asm volatile("mov x28, %0" : : "r"(tf->x[28]));
    asm volatile("mov x29, %0" : : "r"(tf->x[29]));
    asm volatile("mov x30, %0" : : "r"(tf->x[30]));
    // asm volatile("mov %0, fp" : "=r"(tf->fp));
    // asm volatile("mrs %0, sp_el0" : "=r"(tf->sp));
    // asm volatile("mov %0, lr" : "=r"(tf->lr));
    // asm volatile("mrs %0, elr_el1" : "=r"(tf->pc));
    // asm volatile("mrs %0, spsr_el1" : "=r"(tf->spsr));


    // switch pc to user task function
    void (*pc)() = tf->pc;
    asm volatile (
        "LDR x1, %0"
        : 
        : "m" (pc)
    );
    asm volatile("msr elr_el1, x1");

    // setting sp_el0
    asm volatile("msr sp_el0, %0" : : "r"(tf->sp));

    // setting spsr_el1
    asm volatile("mov x0, %0" : : "r"(tf->spsr));
    asm volatile("msr spsr_el1, x0");

    // uint32_t test;
    // asm volatile("mov %0, lr" : "=r"(test));
    // uart_printf(CONSOLE, "WTF lr %x\r\n", test);

    // // asm volatile("mov lr, %0" : : "r"(tf->lr));
    // asm volatile("mov %0, lr" : "=r"(test));
    // uart_printf(CONSOLE, "WTF lr %x\r\n", test);

    asm volatile("mov lr, %0" : : "r"(tf->x[30]));
    // asm volatile("mov fp, %0" : : "r"(tf->fp));
    asm volatile("eret");
}

int run_task(TaskFrame *tf){
    uart_printf(CONSOLE, "running task tid: %u\r\n", tf->tid);

    context_switch_to_task(tf);
    // exit from exception
    uart_printf(CONSOLE, "back in kernel from exception tid: %u\r\n", tf->tid);
    message = -1;

    // read exception code
    uint32_t exception_code;
    asm volatile("mov %0, x19" : "=r"(exception_code));

    return exception_code;
}

int handle_exception_code(uint32_t exception_code, TaskFrame* tf) {
    if(exception_code==EXIT){
        uart_printf(CONSOLE, "run_task exit exception, currentTask->pc: %x\r\n", tf->pc);
        return 0;
    } else if(exception_code==MY_TID){
        uart_printf(CONSOLE, "run_task my tid exception, currentTask->pc: %x\r\n", tf->pc);
        // currentTaskFrame->priority = tf->priority;
        // currentTaskFrame->tid = tf->tid;
        // currentTaskFrame->parentTid = tf->parentTid;
        // message = tf->tid;
        // setNextTaskToBeScheduled(currentTaskFrame);
        
        // SET RETURN REGISTER TO BE TF TID
        tf->x[0] = tf->tid;
        return 1;
    } else if(exception_code==YIELD){
        uart_printf(CONSOLE, "run_task yield exception, currentTask->pc: %x\r\n", tf->pc);
        // currentTaskFrame->priority = tf->priority;
        // currentTaskFrame->tid = tf->tid;
        // currentTaskFrame->tid = 10;
        // currentTaskFrame->parentTid = tf->parentTid;
        return 1;
    } else{
        uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
        for(;;){}
    }
}

// not necessarily needed
void kernel_frame_init(TaskFrame* kernel_frame) {
    kernel_frame->tid = 0;
    kernel_frame->parentTid = 0;
    kernel_frame->next = NULL;
    kernel_frame->priority = 0;
}

void first_task_init(TaskFrame* tf, uint32_t priority, void (*function)(), uint32_t parent_tid, uint64_t lr, uint64_t spsr) {
    tf->priority = priority;
    tf->pc = function;
    tf->parentTid = parent_tid;
    tf->x[30] = lr;
    tf->spsr = spsr;
}

int kmain() {

    init_exception_vector(); 

    // set up GPIO pins for both console and marklin uarts
    gpio_init();

    // not strictly necessary, since line 1 is configured during boot
    // but we'll configure the line anyway, so we know what state it is in
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    uart_printf(CONSOLE, "Program starting\r\n\r\n");

    // set some quality of life defaults, not important to functionality
    TaskFrame kernel_frame;
    kernel_frame_init(&kernel_frame);
    kf = &kernel_frame;

    // USER TASK INITIALIZATION
    TaskFrame user_tasks[NUM_FREE_TASK_FRAMES];
    tasks_init(user_tasks, USER_STACK_START, USER_STACK_SIZE, 0, NUM_FREE_TASK_FRAMES);

    // SCHEDULER INITIALIZATION
    Heap heap; // min heap
    TaskFrame* tfs_heap[NUM_FREE_TASK_FRAMES];
    heap_init(&heap, tfs_heap, NUM_FREE_TASK_FRAMES, task_cmp);

    // FIRST TASK INITIALIZATION
    TaskFrame* first_task = getNextFreeTaskFrame();
    // bits 3-0 must be 0 for ELOt stack pointer
    first_task_init(first_task, 2, &rootTask, kf->tid, (uint64_t)&Exit, 0x600002C0);
    heap_push(&heap, first_task);

    message = -1;
    for(;;){
        currentTaskFrame = heap_pop(&heap);
        uart_printf(CONSOLE, "before scheduled tid: %u \r\n", currentTaskFrame->tid);

        if (currentTaskFrame == NULL) {
            uart_printf(CONSOLE, "NO TASKS\r\n");
            for (;;){}
        }

        int exception_code = run_task(currentTaskFrame);

        int reschedule = handle_exception_code(exception_code, currentTaskFrame);

        if (reschedule) {
            heap_push(&heap, currentTaskFrame);
        } else {
            reclaimTaskFrame(currentTaskFrame);
        }
    }

    return 0;
}

