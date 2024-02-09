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

extern TaskFrame *kf = 0;
extern TaskFrame *currentTaskFrame = 0;

int copy_msg(char *src, int srclen, char *dest, int destlen){
    int reslen = (srclen < destlen) ? srclen : destlen;
    memcpy(dest, src, reslen);
    return reslen;
}

void idle_task(){
    uart_printf(CONSOLE, "Idle Task\r\n");
    uint32_t clock_tid = WhoIs("clock");
    for(;;){
        uart_printf(CONSOLE, "Idle task parking at system time %u\r\n", Time(clock_tid));
        asm volatile("wfi");
        // Yield();
    }
}

void user_task(){
    uart_dprintf(CONSOLE, "User Task\r\n");
    int tid = MyTid();
    int parent_tid = MyParentTid();
    uart_printf(CONSOLE, "\x1b[32mMyTid: %d, MyParentTid: %d\x1b[0m\r\n", tid, parent_tid);
    Yield();
    uart_printf(CONSOLE, "\x1b[32mMyTid: %d, MyParentTid: %d\x1b[0m\r\n", tid, parent_tid);
}

void rootTask_old(){
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

void test() {
    uart_printf(CONSOLE, "Beginning testing\r\n");

    uart_printf(CONSOLE, "Test 1\r\n");
    Create(1, &test1);
    uart_printf(CONSOLE, "Test 1 finished\r\n");

    uart_printf(CONSOLE, "Press any key to continue\r\n");
    char c = uart_getc(CONSOLE);

    uart_printf(CONSOLE, "Test 2\r\n");
    Create(1, &test2);
    uart_printf(CONSOLE, "Test 2 finished\r\n");

    uart_printf(CONSOLE, "Press any key to continue\r\n");
    c = uart_getc(CONSOLE);
    
    uart_printf(CONSOLE, "Test 3\r\n");
    Create(1, &test3);
    uart_printf(CONSOLE, "Test 3 finished\r\n");

    uart_printf(CONSOLE, "Finished testing\r\n");
}

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

    char data[2];
    Send(parent, NULL, 0, data, sizeof(data));

    char delay = data[0];
    char num_delay = data[1];
    for (int i = 0; i < num_delay; i++) {
        uart_printf(CONSOLE, "tid: %u, delay: %u, completed: %u\r\n", tid, delay, i+1);
    }
}

void rootTask(){
    uart_dprintf(CONSOLE, "Root Task\r\n");
    // order or nameserver and idle_task matters since their tid values are assumed in implementation
    Create(2, &nameserver);
    Create(1000, &idle_task);
    Create(0, &notifier);
    Create(1, &clock);
    // int user1 = Create(3, &time_user);

    // int test;
    // Receive(&test, NULL, 0);
    // char user1_data = {10, 20};
    // Reply(user1, user1_data, sizeof(user1_data));
    // Create(2, &gameserver);
    // Create(50, &test);
    // for (;;) {
    // }
    
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}

int run_task(TaskFrame *tf){
    uart_dprintf(CONSOLE, "running task tid: %u\r\n", tf->tid);
    
    int exception_type = context_switch_to_task();
    // exit from exception

    // return interrupt if irq
    if (exception_type == 1) {
        uart_dprintf(CONSOLE, "back in kernel from interrupt cur tid: %u\r\n", tf->tid);
        return IRQ;
    }

    uart_dprintf(CONSOLE, "back in kernel from exception cur tid: %u\r\n", tf->tid);
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

void task_init(TaskFrame* tf, uint32_t priority, uint32_t time, void (*function)(), uint32_t parent_tid, uint64_t lr, uint64_t spsr) {
    tf->priority = priority;
    tf->pc = function;
    tf->parentTid = parent_tid;
    tf->x[30] = lr;
    tf->spsr = spsr;
    tf->added_time = time;
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

    uart_printf(CONSOLE, "Program starting\r\n\r\n");

    // IDLE TASK MEASUREMENT SET UP
    uint64_t idle_task_total, idle_task_start, program_start;
    program_start = sys_time();

    // set some quality of life defaults, not important to functionality
    TaskFrame kernel_frame;
    kernel_frame_init(&kernel_frame);
    kf = &kernel_frame;

    // IRQ event type to blocked task map
    TaskFrame* blocked_on_irq[NUM_IRQ_EVENTS] = {NULL};

    // USER TASK INITIALIZATION
    TaskFrame *nextFreeTaskFrame;
    TaskFrame user_tasks[MAX_NUM_TASKS];
    nextFreeTaskFrame = tasks_init(user_tasks, USER_STACK_START, 2048, MAX_NUM_TASKS);

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
    task_init(root_task, 100, sys_time(), &rootTask, 0, (uint64_t)&Exit, 0x60000240); // 1001 bits for DAIF
    heap_push(&heap, root_task);

    // timer
    timer_init();

    for(;;){
        uart_dprintf(CONSOLE, "start of loop\r\n");
        currentTaskFrame = heap_pop(&heap);

        if (currentTaskFrame == NULL) {
            uart_dprintf(CONSOLE, "Finished tasks\r\n");
            for (;;){}
        }

        // if(currentTaskFrame->tid == 2){
        //     idle_task_start = sys_time();
        // }

        int exception_code = run_task(currentTaskFrame);

        // if(currentTaskFrame->tid == 2){
        //     idle_task_total += (sys_time()-idle_task_start);
        // }

        if(exception_code==CREATE){
            int priority = (int)(currentTaskFrame->x[0]);
            void (*function)() = (void *)(currentTaskFrame->x[1]);
            if(priority<0){
                uart_dprintf(CONSOLE, "\x1b[31mInvalid priority %d\x1b[0m\r\n", priority);
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame* created_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
            if(created_task==NULL){
                uart_dprintf(CONSOLE, "\x1b[31mKernel out of task descriptors\x1b[0m\r\n");
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }
            task_init(created_task, priority, sys_time(), function, currentTaskFrame->tid, (uint64_t)&Exit, 0x60000240);
            heap_push(&heap, created_task);
            reschedule_task_with_return(&heap, currentTaskFrame, created_task->tid);
        } else if(exception_code==EXIT){
            int num_tasks_waiting_on_msg = currentTaskFrame->sender_queue.count;
            if (num_tasks_waiting_on_msg > 0) {
                uart_printf(CONSOLE, "\x1b[31mTask %u exiting with %u waiting tasks \x1b[0m\r\n", currentTaskFrame->tid, num_tasks_waiting_on_msg);
            }

            reclaimTaskFrame(nextFreeTaskFrame, currentTaskFrame);
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
                uart_dprintf(CONSOLE, "\x1b[31mOn send tid invalid %d\x1b[0m\r\n", tid);
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *recipient = user_tasks + tid;
            if(currentTaskFrame->sd != NULL || currentTaskFrame->status!=READY || recipient->status==SEND_WAIT || recipient->status==REPLY_WAIT){
                uart_dprintf(CONSOLE, "\x1b[31mOn send sender/recipient status not valid %d %d %d\x1b[0m\r\n", currentTaskFrame->sd, currentTaskFrame->status, recipient->status);
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
                    uart_dprintf(CONSOLE, "\x1b[31mOn send recipient rd is null\x1b[0m\r\n");
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
            uart_dprintf(CONSOLE, "On reply %d %x %d\r\n", tid, reply, rplen);
            // if receive status not in ready or send status not in reply-wait, error
            // copy data and set both status to ready
            if(tid>MAX_NUM_TASKS || user_tasks[tid].status==INACTIVE){
                uart_dprintf(CONSOLE, "\x1b[31mtid not valid %d \x1b[0m\r\n", tid);
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *sender = user_tasks + tid;
            if(sender->status!=REPLY_WAIT){
                uart_dprintf(CONSOLE, "\x1b[31mOn reply sender status not reply_wait %d\x1b[0m\r\n", sender->status);
                reschedule_task_with_return(&heap, currentTaskFrame, -2);
                continue;
            }
            if(currentTaskFrame->status!=READY){
                uart_dprintf(CONSOLE, "\x1b[31mOn reply current status not ready %d\x1b[0m\r\n", sender->status);
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
            uart_dprintf(CONSOLE, "AwaitEvent %d\r\n", type);

            if (type < CLOCK || type > TODO) {
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
            }

            // TODO: not sure if we should buffer the tasks and what to return when we reach that
            // currently, we can only have a single element waiting
            if (blocked_on_irq[type] != NULL) {
                reschedule_task_with_return(&heap, currentTaskFrame, -1);
            }

            blocked_on_irq[type] = currentTaskFrame;
        } else if (exception_code == IRQ) {
            uart_dprintf(CONSOLE, "On irq\r\n");

            uint32_t irq_ack = *(uint32_t*)(GICC_IAR);
            uint32_t irq_id_bit_mask = 0x1FF; // 9 bits
            uint32_t irq = (irq_ack & irq_id_bit_mask);

            if (irq == SYSTEM_TIMER_IRQ_1) {
                if (blocked_on_irq[CLOCK] != NULL) {
                    reschedule_task_with_return(&heap, blocked_on_irq[CLOCK], 0);
                    blocked_on_irq[CLOCK] = NULL;
                }

                // uart_printf(CONSOLE, "\r\ntime = %u\r\n", sys_time());
                // uart_printf(CONSOLE, "c1 before = %u\r\n", *(uint32_t*)(ST_BASE+ST_C1));

                // update system timer C1
                *(uint32_t*)(ST_BASE+ST_C1) = *(uint32_t*)(ST_BASE+ST_C1) + INTERVAL;

                // uart_printf(CONSOLE, "c1 after = %u\r\n", *(uint32_t*)(ST_BASE+ST_C1));

                // update clock status register
                *(uint32_t*)(ST_BASE) = *(uint32_t*)(ST_BASE) | 1 << 1;

                // finish irq processing
                *(uint32_t*)(GICC_EOIR) = irq_ack;
            } else {
                uart_printf(CONSOLE, "Unrecognized interrupt %d\r\n", irq);
                for(;;){}
            }

            heap_push(&heap, currentTaskFrame);
        } else {
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }

        // uart_printf(CONSOLE, "Idle time ratio: %u\r\n", idle_task_total/(program_start-sys_time()));
    }

    return 0;
}

