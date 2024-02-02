#include "rpi.h"
#include "util.h"
#include "exception.h"
#include "task.h"
#include "heap.h"
#include "syscall.h"
#include "nameserver.h"

extern TaskFrame *kf = 0;
extern TaskFrame *currentTaskFrame = 0;

int copy_msg(char *src, int srclen, char *dest, int destlen){
    int reslen = (srclen < destlen) ? srclen : destlen;
    memcpy(dest, src, reslen);
    return reslen;
}

void game_client1(){
    const char *name = "game_client1";
    RegisterAs(name);
    int server_tid = WhoIs("game_server");
    uart_printf(CONSOLE, "Server tid: %d\r\n", server_tid);

    // s = signup, p = play, q = quit
    char action = 's';
    int send;
    int recieve;

    char reply;
    send = Send(server_tid, &action, 1, &reply, 1);

}

void game_server(){
    const char *name = "game_server";
    int register_status = RegisterAs(name);
    if(register_status<0){
        uart_printf(CONSOLE, "\x1b[31mgame_server registeras failed\x1b[0m\r\n");
        for(;;){}
    }

    int server_tid = WhoIs(name);
    if(server_tid<0){
        uart_printf(CONSOLE, "\x1b[31mgame_server whois failed\x1b[0m\r\n");
        for(;;){}
    }
    uart_printf(CONSOLE, "Server tid: %d\r\n", server_tid);
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
    
    // uint32_t test1, test2;
    // asm volatile("mov %0, lr" : "=r"(test1));
    // asm volatile("mov %0, sp" : "=r"(test2));
    // uart_dprintf(CONSOLE, "lr, sp: %x, %x\r\n", test1, test2);


    context_switch_to_task();

    // uart_dprintf(CONSOLE, "reg 0 1 2 3 4 5: %d, %d, %d, %d, %d, %d\r\n", currentTaskFrame->x[0], currentTaskFrame->x[1], currentTaskFrame->x[2], currentTaskFrame->x[3], currentTaskFrame->x[4], currentTaskFrame->x[5]);
    // exit from exception
    uart_dprintf(CONSOLE, "back in kernel from exception tid: %u\r\n", tf->tid);

    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint64_t exception_code = esr & 0xFULL;

    // asm volatile("mov %0, lr" : "=r"(test1));
    // asm volatile("mov %0, sp" : "=r"(test2));
    // uart_dprintf(CONSOLE, "lr, sp: %x, %x\r\n", test1, test2);

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
    tf->added_time = get_time();
    heap_push(heap, tf);
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
    TaskFrame* name_server_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
    task_init(name_server_task, 2, get_time(), &nameserver, kf->tid, (uint64_t)&Exit, 0x600002C0);
    heap_push(&heap, name_server_task);

    TaskFrame* game_server_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
    task_init(game_server_task, 2, get_time(), &game_server, kf->tid, (uint64_t)&Exit, 0x600002C0);
    heap_push(&heap, game_server_task);

    // TaskFrame* root_task = getNextFreeTaskFrame(&nextFreeTaskFrame);
    // task_init(root_task, 2, get_time(), &rootTask, kf->tid, (uint64_t)&Exit, 0x600002C0);
    // heap_push(&heap, root_task);

    for(;;){
        currentTaskFrame = heap_pop(&heap);

        if (currentTaskFrame == NULL) {
            uart_dprintf(CONSOLE, "Finished tasks\r\n");
            for (;;){}
        }

        int exception_code = run_task(currentTaskFrame);

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
            task_init(created_task, priority, get_time(), function, currentTaskFrame->tid, (uint64_t)&Exit, 0x600002C0);
            heap_push(&heap, created_task);
            reschedule_task_with_return(&heap, currentTaskFrame, created_task->tid);
        } else if(exception_code==EXIT){
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
                recipient->sender_queue[recipient->sender_queue_len] = currentTaskFrame->tid;
                recipient->sender_queue_len += 1;
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
                uart_printf(CONSOLE, "\x1b[31mOn receive task status not ready %u\x1b[0m\r\n", exception_code);
                for(;;){}
            }
            if(currentTaskFrame->sender_queue_len==0){
                if(currentTaskFrame->rd!=NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn receive task rd not null %u\x1b[0m\r\n", exception_code);
                    for(;;){}
                }
                ReceiveData *rd = getNextFreeReceiveData(&nextFreeReceiveData);
                rd->msg = msg;
                rd->msglen = msglen;
                rd->tid = tid;
                currentTaskFrame->status = RECEIVE_WAIT;
                currentTaskFrame->rd = rd;
            }else{
                int sender_tid = currentTaskFrame->sender_queue[0];
                if(sender_tid>MAX_NUM_TASKS){
                    uart_printf(CONSOLE, "\x1b[31mOn receive sender tid invalid %u\x1b[0m\r\n", exception_code);
                    for(;;){}
                }
                TaskFrame *sender = user_tasks + sender_tid;
                if(sender->status!=SEND_WAIT || sender->sd==NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn receive sender status not valid %u\x1b[0m\r\n", exception_code);
                    for(;;){}
                }

                // TODO: make this a circular array
                for(int i = 1; i<currentTaskFrame->sender_queue_len; i++){
                    currentTaskFrame->sender_queue[i-1] = currentTaskFrame->sender_queue[i];
                }
                currentTaskFrame->sender_queue_len -= 1;

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
                uart_dprintf(CONSOLE, "\x1b[31mSender status not reply_wait %d\x1b[0m\r\n", sender->status);
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
        } else{
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }
    }

    return 0;
}

