#include "rpi.h"
#include "exception.h"
#include "tasks.h"
#include "heap.h"

#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5

extern TaskFrame *kf = 0;
extern TaskFrame *currentTaskFrame = 0;

void invalid_exception(uint32_t exception) {
    uart_printf(CONSOLE, "Invalid exception %u", exception);
    for (;;) {}
}

int Create(int priority, void (*function)()){
    uart_dprintf(CONSOLE, "creating task: %d %x \r\n", priority, function);
    
    int tid;

    asm volatile(
        "mov x0, %[priority]\n"
        "mov x1, %[function]\n"
        "svc %[SYS_CODE]"
        :
        : [priority] "r" (priority),
        [function] "r" (function),
        [SYS_CODE] "i"(CREATE) 
    );

    asm volatile("mov %0, x0" : "=r"(tid));
    return tid;
}

int MyTid(){
    uart_dprintf(CONSOLE, "fetching tid \r\n");
    int tid;

    asm volatile(
        "svc %[SYS_CODE]"
        :
        : [SYS_CODE] "i"(MY_TID) 
    );
    asm volatile("mov %0, x0" : "=r"(tid));
    uart_dprintf(CONSOLE, "MyTid: %d\r\n", tid);

    return tid;
}

int MyParentTid(){
    uart_dprintf(CONSOLE, "fetching parent tid \r\n");
    int parent_tid;

    asm volatile(
        "svc %[SYS_CODE]"
        :
        : [SYS_CODE] "i"(MY_PARENT_TID) 
    );
    asm volatile("mov %0, x0" : "=r"(parent_tid));
    uart_dprintf(CONSOLE, "MyParentTid: %d\r\n", parent_tid);

    return parent_tid;
}

void Yield(){
    // TODO: make sys codes global
    uart_dprintf(CONSOLE, "task yielding \r\n");

    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(YIELD) 
    );
}

void Exit(){
    uart_dprintf(CONSOLE, "task exiting \r\n");

    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(EXIT) 
    );
}



void user_task(){
    uart_dprintf(CONSOLE, "User Task\r\n");
    int tid = MyTid();
    int parent_tid = MyParentTid();
    uart_printf(CONSOLE, "\x1b[32mMyTid: %d, MyParentTid: %d\x1b[0m\r\n", tid, parent_tid);
    Yield();
    uart_printf(CONSOLE, "\x1b[32mMyTid: %d, MyParentTid: %d\x1b[0m\r\n", tid, parent_tid);
}

void rootTask(){
    uart_dprintf(CONSOLE, "Root Task\r\n");
    int tid = MyTid();
    int parent_tid = MyParentTid();
    // lower value priority --> higher higher
    int tid1 = Create(3, &user_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid1);
    int tid2 = Create(3, &user_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid2);
    int tid3 = Create(1, &user_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid3);
    int tid4 = Create(1, &user_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid4);
    
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}

int run_task(TaskFrame *tf){
    uart_dprintf(CONSOLE, "running task tid: %u\r\n", tf->tid);

    context_switch_to_task();

    // exit from exception
    uart_dprintf(CONSOLE, "back in kernel from exception tid: %u\r\n", tf->tid);

    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint64_t exception_code = esr & 0xFULL;

    return exception_code;
}

// not necessarily needed
void kernel_frame_init(TaskFrame* kernel_frame) {
    kernel_frame->tid = 0;
    kernel_frame->parentTid = 0;
    kernel_frame->next = NULL;
    kernel_frame->priority = 0;
}

uint32_t get_time(){
    return *(volatile uint32_t *)((char*) 0xFe003000 + 0x04);
}

void task_init(TaskFrame* tf, uint32_t priority, uint32_t time, void (*function)(), uint32_t tid, uint32_t parent_tid, uint64_t lr, uint64_t spsr) {
    tf->priority = priority;
    tf->pc = function;
    tf->tid = tid;
    tf->parentTid = parent_tid;
    tf->x[30] = lr;
    tf->spsr = spsr;
    tf->added_time = time;
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
    tasks_init(user_tasks, USER_STACK_START, USER_STACK_SIZE, NUM_FREE_TASK_FRAMES);

    // SCHEDULER INITIALIZATION
    Heap heap; // min heap
    TaskFrame* tfs_heap[NUM_FREE_TASK_FRAMES];
    heap_init(&heap, tfs_heap, NUM_FREE_TASK_FRAMES, task_cmp);

    int next_user_tid = 0;

    // FIRST TASK INITIALIZATION
    TaskFrame* first_task = getNextFreeTaskFrame();
    // bits 3-0 must be 0 for ELOt stack pointer
    task_init(first_task, 2, get_time(), &rootTask, next_user_tid, kf->tid, (uint64_t)&Exit, 0x600002C0);
    next_user_tid+=1;
    heap_push(&heap, first_task);

    for(;;){
        currentTaskFrame = heap_pop(&heap);

        if (currentTaskFrame == NULL) {
            uart_dprintf(CONSOLE, "Finished tasks\r\n");
            for (;;){}
        }

        int exception_code = run_task(currentTaskFrame);
        uart_dprintf(CONSOLE, "after run_task\r\n");

        if(exception_code==CREATE){
            TaskFrame* created_task = getNextFreeTaskFrame();
            int priority;
            void (*function)();
            int test;
            int test2;

            asm volatile(
                "mov %[priority], x0\n"
                "mov %[function], x1"
                : 
                [test]"=r"(test),
                [test2]"=r"(test2),
                [priority]"=r"(priority),
                [function]"=r"(function)
            );
            task_init(created_task, priority, get_time(),function, next_user_tid, currentTaskFrame->tid, (uint64_t)&Exit, 0x600002C0);
            heap_push(&heap, created_task);
            currentTaskFrame->added_time = get_time();
            currentTaskFrame->x[0] = next_user_tid;
            heap_push(&heap, currentTaskFrame);
            next_user_tid +=1;
        } else if(exception_code==EXIT){
            reclaimTaskFrame(currentTaskFrame);
        } else if(exception_code==MY_TID){
            // SET RETURN REGISTER TO BE TF TID
            currentTaskFrame->x[0] = currentTaskFrame->tid;
            currentTaskFrame->added_time = get_time();
            heap_push(&heap, currentTaskFrame);
        } else if(exception_code==MY_PARENT_TID){
            // SET RETURN REGISTER TO BE TF TID
            currentTaskFrame->x[0] = currentTaskFrame->parentTid;
            currentTaskFrame->added_time = get_time();
            heap_push(&heap, currentTaskFrame);
        } else if(exception_code==YIELD){
            currentTaskFrame->added_time = get_time();
            heap_push(&heap, currentTaskFrame);
        } else{
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }
    }

    return 0;
}

