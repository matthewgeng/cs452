#include "rpi.h"
#include "util.h"
#include "exception.h"
#include "clock.h"
#include "task.h"
#include "heap.h"
#include "syscall.h"
#include "nameserver.h"
#include "gameserver.h"
#include "gameclient.h"
#include "timer.h"
#include "interrupt.h"
#include "clock.h"

extern TaskFrame* kf = 0;
extern TaskFrame* currentTaskFrame = 0;
static uint64_t* p_idle_task_total = 0;;
static uint64_t* p_program_start = 0;

int copy_msg(char *src, int srclen, char *dest, int destlen){
    int reslen = (srclen < destlen) ? srclen : destlen;
    memcpy(dest, src, reslen);
    return reslen;
}

void idle_task(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Idle Task\r\n");
    #endif 
    for(;;){
        uint32_t idle_time_percent = (*p_idle_task_total*100)/(sys_time() - *p_program_start);
        uart_printf(CONSOLE, "\0337\033[3;1HIdle percentage: %u%% \0338", idle_time_percent);
        // uart_printf(CONSOLE, "\033[2;1HIdle percentage: %u%%", idle_time_percent);
        asm volatile("wfi");
    }
}

// remove usage of this, move functionality to timer.c/h
static char* const ST_BASE = (char*)(0xFE003000); // todo use MMIO_BASE

static const uint32_t ST_UPDATE_FREQ = 1000000; // given frequency

// system timer register offsets
static const uint32_t ST_CS = 0x00;
static const uint32_t ST_CLO = 0x04;
static const uint32_t ST_CHI = 0x08;
static const uint32_t ST_CO = 0x0c;
static const uint32_t ST_C1 = 0x10;
static const uint32_t ST_C2 = 0x14;
static const uint32_t ST_C3 = 0x18;

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

void rootTask(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Root Task\r\n");
    #endif 
    // order or nameserver and idle_task matters since their tid values are assumed in implementation
    Create(2, &nameserver);
    Create(1000, &idle_task);
    Create(0, &notifier);
    Create(1, &clock);
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
    
    // uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}

int run_task(TaskFrame *tf){
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

// not necessarily needed
void kernel_frame_init(TaskFrame* kernel_frame) {
    kernel_frame->tid = 0;
    kernel_frame->parentTid = 0;
    kernel_frame->next = NULL;
    kernel_frame->priority = 0;
}

void task_init(TaskFrame* tf, uint32_t priority, uint32_t time, void (*function)(), uint32_t parent_tid, uint64_t lr, uint64_t spsr, uint16_t status) {
    tf->priority = priority;
    tf->pc = function;
    tf->parentTid = parent_tid;
    tf->x[30] = lr;
    tf->spsr = spsr;
    tf->added_time = time;
    tf->status = status;
}

void reschedule_task_with_return(Heap *heap, TaskFrame *tf, long long ret){
    tf->x[0] = ret;
    tf->added_time = sys_time();
    heap_push(heap, tf);
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
        #if DEBUG
            uart_dprintf(CONSOLE, "start of loop\r\n");
        #endif 
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

        int exception_code = run_task(currentTaskFrame);

        // idle task time calculation
        if(currentTaskFrame->tid == 2){
            idle_task_total += (sys_time()-idle_task_start);
        }

        if(exception_code==CREATE){
            int priority = (int)(currentTaskFrame->x[0]);
            void (*function)() = (void *)(currentTaskFrame->x[1]);
            if(priority<0){
                uart_printf(CONSOLE, "\x1b[31mInvalid priority %d\x1b[0m\r\n", priority);
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame* created_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
            if(created_task==NULL){
                uart_printf(CONSOLE, "\x1b[31mKernel out of task descriptors\x1b[0m\r\n");
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }
            task_init(created_task, priority, sys_time(), function, currentTaskFrame->tid, (uint64_t)&Exit, 0x60000240, READY);
            heap_push(&heap, created_task);
            reschedule_task_with_return(&heap, currentTaskFrame, created_task->tid);
        } else if(exception_code==EXIT){
            int num_tasks_waiting_on_msg = currentTaskFrame->sender_queue.count;
            if (num_tasks_waiting_on_msg > 0) {
                uart_printf(CONSOLE, "\x1b[31mTask %u exiting with %u waiting tasks \x1b[0m\r\n", currentTaskFrame->tid, num_tasks_waiting_on_msg);
            }

            reclaimTaskFrame(&nextFreeTaskFrame, currentTaskFrame);
        } else if(exception_code==MY_TID){
            reschedule_task_with_return(&heap, currentTaskFrame, currentTaskFrame->tid);
        } else if(exception_code==MY_PARENT_TID){
            reschedule_task_with_return(&heap, currentTaskFrame, currentTaskFrame->parentTid);
        } else if(exception_code==YIELD){
            reschedule_task_with_return(&heap, currentTaskFrame, 0);
        } else if(exception_code==SEND){
            int tid = (int)(currentTaskFrame->x[0]);
            const char *msg = (const char *)(currentTaskFrame->x[1]);
            int msglen = (int)(currentTaskFrame->x[2]);
            char *reply = (char *)(currentTaskFrame->x[3]);
            int rplen = (int)(currentTaskFrame->x[4]);            

            // find recipient tid, if not exist, return -1
            // if sender task not in ready, error
            // if recipient in ready, sender status ready->send-wait and add to recipient queue
            // if recipient in receive-wait, copy data, recipient wait->ready, sender ready->reply-wait
            // receive data can be in receive task frame?
            if(tid>MAX_NUM_TASKS || user_tasks[tid].status==INACTIVE){
                #if DEBUG
                    uart_dprintf(CONSOLE, "\x1b[31mOn send tid invalid %d\x1b[0m\r\n", tid);
                #endif 
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *recipient = user_tasks + tid;
            if(currentTaskFrame->sd != NULL || currentTaskFrame->status!=READY || recipient->status==SEND_WAIT || recipient->status==REPLY_WAIT){
                uart_printf(CONSOLE, "\x1b[31mOn send sender/recipient status not valid %d %d %d\x1b[0m\r\n", currentTaskFrame->sd, currentTaskFrame->status, recipient->status);
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }

            SendData *sd = getNextFreeSendData(&nextFreeSendData);
            sd->tid = tid;
            sd->msg = msg;
            sd->msglen = msglen;
            sd->reply = reply;
            sd->rplen = rplen;
            currentTaskFrame->sd = sd;
            if(recipient->status==READY){
                currentTaskFrame->status = SEND_WAIT;
                push(&(recipient->sender_queue), currentTaskFrame->tid);
            }else if(recipient->status==RECEIVE_WAIT){

                ReceiveData *rd = recipient->rd;
                if(rd==NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn send recipient rd is null\x1b[0m\r\n");
                    reschedule_task_with_return(&heap, currentTaskFrame, -2);
                    continue;
                }
                *(rd->tid) = currentTaskFrame->tid;
                copy_msg(msg, msglen, rd->msg, rd->msglen);
                currentTaskFrame->status = REPLY_WAIT;
                recipient->status = READY;
                reclaimReceiveData(&nextFreeReceiveData, recipient->rd);
                recipient->rd = NULL;
                reschedule_task_with_return(&heap, recipient, msglen);
            }
        } else if(exception_code==RECEIVE){
            int *tid = (int *)(currentTaskFrame->x[0]);
            const char *msg = (const char *)(currentTaskFrame->x[1]);
            int msglen = (int)(currentTaskFrame->x[2]);
            // if receive status not in ready, error
            // check queue to see if any senders ready
            // if not, status ready->receive-wait
            // if so, remove sender from queue, copy data, sender send-wait->reply-wait
            // if sender task is not in send-wait, error 
            if(currentTaskFrame->status!=READY){
                uart_printf(CONSOLE, "\x1b[31mOn receive task status not ready %u\x1b[0m\r\n", currentTaskFrame->status);
                for(;;){}
            }
            if(is_empty(currentTaskFrame->sender_queue)){
                if(currentTaskFrame->rd!=NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn receive task rd not null\x1b[0m\r\n");
                    for(;;){}
                }
                ReceiveData *rd = getNextFreeReceiveData(&nextFreeReceiveData);
                rd->msg = msg;
                rd->msglen = msglen;
                rd->tid = tid;
                currentTaskFrame->status = RECEIVE_WAIT;
                currentTaskFrame->rd = rd;
            }else{
                int sender_tid = pop(&(currentTaskFrame->sender_queue));
                if(sender_tid<0 || sender_tid>MAX_NUM_TASKS){
                    uart_printf(CONSOLE, "\x1b[31mOn receive sender tid invalid %u\x1b[0m\r\n", sender_tid);
                    for(;;){}
                }
                TaskFrame *sender = user_tasks + sender_tid;
                if(sender->status!=SEND_WAIT || sender->sd==NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn receive sender status not valid %u\x1b[0m\r\n", sender->status);
                    for(;;){}
                }
                SendData *sd = sender->sd;
                *tid = sender_tid;
                copy_msg(sd->msg, sd->msglen, msg, msglen);
                sender->status = REPLY_WAIT;
                reschedule_task_with_return(&heap, currentTaskFrame, sd->msglen);
            }
        } else if(exception_code==REPLY){
            // asm volatile("mov x30, x30\nmov x30, x30\nmov x30, x30\nmov x30, x30\n");
            int tid = (int)(currentTaskFrame->x[0]);
            char *reply = (char *)(currentTaskFrame->x[1]);
            int rplen = (int)(currentTaskFrame->x[2]);     
            #if DEBUG
                uart_dprintf(CONSOLE, "On reply %d %x %d\r\n", tid, reply, rplen);
            #endif 
            // if receive status not in ready or send status not in reply-wait, error
            // copy data and set both status to ready
            if(tid>MAX_NUM_TASKS || user_tasks[tid].status==INACTIVE){
                uart_printf(CONSOLE, "Replier %u sender %u, status %u\r\n", currentTaskFrame->tid, tid, user_tasks[tid].status);
                uart_printf(CONSOLE, "\x1b[31mOn reply replier %u sender tid not valid %d \x1b[0m\r\n", currentTaskFrame->tid, tid);
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *sender = user_tasks + tid;
            if(sender->status!=REPLY_WAIT){
                uart_printf(CONSOLE, "\x1b[31mOn reply replier %u to sender %u status not reply_wait %d\x1b[0m\r\n",currentTaskFrame->tid, tid, sender->status);
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }
            if(currentTaskFrame->status!=READY){
                uart_printf(CONSOLE, "\x1b[31mOn reply current status not ready %d\x1b[0m\r\n", sender->status);
                for(;;){}
            }            

            sender->status = READY;
            SendData *sd = sender->sd;
            reclaimSendData(&nextFreeSendData, sender->sd);
            sender->sd = NULL;
            int reslen = copy_msg(reply, rplen, sd->reply, sd->rplen);
            reschedule_task_with_return(&heap, currentTaskFrame, reslen);
            reschedule_task_with_return(&heap, sender, rplen);
        
        } else if (exception_code == AWAIT_EVENT) {
            int type = (int)(currentTaskFrame->x[0]);
            #if DEBUG
                uart_dprintf(CONSOLE, "AwaitEvent %d\r\n", type);
            #endif 

            if (type < CLOCK || type > TODO) {
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }

            // TODO: not sure if we should buffer the tasks and what to return when we reach that
            // currently, we can only have a single element waiting
            if (blocked_on_irq[type] != NULL) {
                uart_printf(CONSOLE, "Existing task %u already waiting for %d\r\n", blocked_on_irq[type]->tid, type);
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }

            blocked_on_irq[type] = currentTaskFrame;
        } else if (exception_code == IRQ) {
            #if DEBUG
                uart_dprintf(CONSOLE, "On irq\r\n");
            #endif 

            uint32_t irq_ack = *(uint32_t*)(GICC_IAR);
            uint32_t irq_id_bit_mask = 0x1FF; // 9 bits
            uint32_t irq = (irq_ack & irq_id_bit_mask);

            if (irq == SYSTEM_TIMER_IRQ_1) {
                if (blocked_on_irq[CLOCK] != NULL) {
                    reschedule_task_with_return(&heap, blocked_on_irq[CLOCK], 0);
                    blocked_on_irq[CLOCK] = NULL;
                }

                // update system timer C1
                *(uint32_t*)(ST_BASE+ST_C1) = *(uint32_t*)(ST_BASE+ST_C1) + INTERVAL;

                // update clock status register
                *(uint32_t*)(ST_BASE) = *(uint32_t*)(ST_BASE) | 1 << 1;

                // finish irq processing
                *(uint32_t*)(GICC_EOIR) = irq_ack;
            } else {
                // invalid event
                uart_printf(CONSOLE, "Invalid/Unsupported IRQ \r\n");
                for(;;){}
            }

            // reschedule 
            heap_push(&heap, currentTaskFrame);
        } else {
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }
    }

    return 0;
}

