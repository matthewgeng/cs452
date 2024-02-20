#include "rpi.h"
#include "util.h"
#include "exception.h"
#include "clock.h"
#include "taskframe.h"
#include "heap.h"
#include "syscall.h"
#include "nameserver.h"
#include "gameserver.h"
#include "gameclient.h"
#include "timer.h"
#include "interrupt.h"
#include "clock.h"
#include "k_handler.h"
#include "tasks.h"

TaskFrame* kf = 0;
TaskFrame* currentTaskFrame = 0;
uint64_t* p_idle_task_total = 0;
uint64_t* p_program_start = 0;


// not necessarily needed
void kernel_frame_init(TaskFrame* kernel_frame) {
    kernel_frame->tid = 0;
    kernel_frame->parentTid = 0;
    kernel_frame->next = NULL;
    kernel_frame->priority = 0;
}

int run_task(){
    int exception_type = context_switch_to_task();
    // exit from exception

    // return interrupt if irq
    if (exception_type == 1) {
        return IRQ;
    }

    // return exception
    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint64_t exception_code = esr & 0xFULL;

    return exception_code;
}

int kmain() {

    // set up GPIO pins for both console and marklin uarts
    gpio_init();

    // not strictly necessary, since line 1 is configured during boot
    // but we'll configure the line anyway, so we know what state it is in
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    // below is initialized after above due to use of print statements

    // EXCEPTIONS
    init_exception_vector(); 

    // INTERRUPTS
    enable_irqs();

    // clear
    uart_printf(CONSOLE, "\033[2J");
    // reset cursor to top left
    uart_printf(CONSOLE, "\033[H");
    // program start
    uart_printf(CONSOLE, "Program starting: \r\n\r\n");
    uart_printf(CONSOLE, "Idle percentage:   \r\n");

    // IDLE TASK MEASUREMENT SET UP
    uint64_t idle_task_total = 0, idle_task_start = 0, program_start = 0;
    program_start = sys_time();
    p_program_start = &program_start;
    p_idle_task_total = &idle_task_total;

    // set some quality of life defaults, not important to functionality
    TaskFrame kernel_frame;
    kernel_frame_init(&kernel_frame);
    kf = &kernel_frame;

    // IRQ event type to blocked task map
    TaskFrame* blocked_on_irq[NUM_IRQ_EVENTS] = {NULL};

    // USER TASK INITIALIZATION
    TaskFrame *nextFreeTaskFrame;
    TaskFrame user_tasks[MAX_NUM_TASKS];
    nextFreeTaskFrame = tasks_init(user_tasks, USER_STACK_START, USER_STACK_SIZE, MAX_NUM_TASKS);

    // TASK DESCRIPTOR INITIALIZATION
    Heap heap; // min heap
    TaskFrame* tfs_heap[MAX_NUM_TASKS];
    heap_init(&heap, tfs_heap, MAX_NUM_TASKS, task_cmp);

    // SEND RECEIVE REPLY INITIALIZATION
    SendData *nextFreeSendData;
    SendData send_datas[MAX_NUM_TASKS];
    nextFreeSendData = sds_init(send_datas, MAX_NUM_TASKS);
    ReceiveData *nextFreeReceiveData;
    ReceiveData receive_datas[MAX_NUM_TASKS];
    nextFreeReceiveData = rds_init(receive_datas, MAX_NUM_TASKS);

    // FIRST TASKS INITIALIZATION
    TaskFrame* root_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
    task_init(root_task, 100, sys_time(), &rootTask, 0, (uint64_t)&Exit, 0x60000240, READY); // 1001 bits for DAIF
    heap_push(&heap, root_task);

    // timer
    timer_init();

    for(;;){
        // #if DEBUG
        //     uart_dprintf(CONSOLE, "start of loop\r\n");
        // #endif 
        currentTaskFrame = heap_pop(&heap);

        if (currentTaskFrame == NULL) {
            #if DEBUG
                uart_dprintf(CONSOLE, "Finished tasks\r\n");
            #endif 
            for (;;){}
        }

        // idle task time calculation
        if(currentTaskFrame->tid == 2){
            idle_task_start = sys_time();
        }

        int exception_code = run_task();

        // idle task time calculation
        if(currentTaskFrame->tid == 2){
            idle_task_total += (sys_time()-idle_task_start);
        }

        if(exception_code==CREATE){
            handle_create(&heap, &nextFreeTaskFrame);
        } else if(exception_code==EXIT){
            handle_exit(&nextFreeTaskFrame);
        } else if(exception_code==MY_TID){
            handle_my_tid(&heap);
        } else if(exception_code==MY_PARENT_TID){
            handle_parent_tid(&heap);
        } else if(exception_code==YIELD){
            handle_yield(&heap);
        } else if(exception_code==SEND){
            handle_send(&heap, user_tasks, &nextFreeSendData, &nextFreeReceiveData);
        } else if(exception_code==RECEIVE){
            handle_receive(&heap, user_tasks, &nextFreeReceiveData);
        } else if(exception_code==REPLY){
            handle_reply(&heap, user_tasks, &nextFreeSendData);
        } else if (exception_code == AWAIT_EVENT) {
            handle_await_event(&heap, blocked_on_irq);
        } else if (exception_code == IRQ) {
            handle_irq(&heap, blocked_on_irq);
        } else {
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }
    }

    return 0;
}

