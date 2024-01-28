#include "rpi.h"
#include "util.h"
#include "exception.h"
#include "tasks.h"
#include "heap.h"

#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5
#define SEND 6
#define RECEIVE 7
#define REPLY 8

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
    uart_dprintf(CONSOLE, "task yielding \r\n");

    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(YIELD) 
    );
}

void Exit(){
    uart_dprintf(CONSOLE, "task exiting \r\n");
    // TODO: maybe remove tid from nameserver
    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(EXIT) 
    );

}

int Send(int tid, const char *msg, int msglen, char *reply, int rplen){

    // TODO: note that if a function gets a -2 return from send, should just throw error and halt
    uart_dprintf(CONSOLE, "Send \r\n");
    
    int intended_reply_len;

    asm volatile(
        "mov x0, %[tid]\n"
        "mov x1, %[msg]\n"
        "mov x2, %[msglen]\n"
        "mov x3, %[reply]\n"
        "mov x4, %[rplen]\n"
        "svc %[SYS_CODE]"
        :
        : [tid] "r" (tid),
        [msg] "r" (msg),
        [msglen] "r" (msglen),
        [reply] "r" (reply),
        [rplen] "r" (rplen),
        [SYS_CODE] "i"(RECEIVE) 
    );

    asm volatile("mov %0, x0" : "=r"(intended_reply_len));
    return intended_reply_len;
}

int Receive(int *tid, char *msg, int msglen){

    uart_dprintf(CONSOLE, "Receive \r\n");
    
    int intended_sent_len;

    asm volatile(
        "mov x0, %[tid]\n"
        "mov x1, %[msg]\n"
        "mov x2, %[msglen]\n"
        "svc %[SYS_CODE]"
        :
        : [tid] "r" (tid),
        [msg] "r" (msg),
        [msglen] "r" (msglen),
        [SYS_CODE] "i"(RECEIVE) 
    );

    asm volatile("mov %0, x0" : "=r"(intended_sent_len));
    return intended_sent_len;
}

int Reply(int tid, const char *reply, int rplen){

    uart_dprintf(CONSOLE, "Reply \r\n");
    
    int actual_reply_len;

    asm volatile(
        "mov x0, %[tid]\n"
        "mov x1, %[reply]\n"
        "mov x2, %[rplen]\n"
        "svc %[SYS_CODE]"
        :
        : [tid] "r" (tid),
        [reply] "r" (reply),
        [rplen] "r" (rplen),
        [SYS_CODE] "i"(REPLY) 
    );

    asm volatile("mov %0, x0" : "=r"(actual_reply_len));
    return actual_reply_len;

}

int RegisterAs(const char *name){

}

int WhoIs(const char *name){

}


int str_cmp(char *c1, char *c2){
    while(1){
        if(*c1=='\0' && *c2=='\0'){
            return 1;
        }else if(*c1=='\0'){
            return 0;
        }else if(*c2=='\0'){
            return 0;
        }else if(*c1!=*c2){
            return 0;
        }else{
            c1 += 1;
            c2 += 1;
        }
    }
}

// define messages to name server as r+name or w+name
void name_server(){
    char *tid_to_name[NUM_FREE_TASK_FRAMES];
    for(int i = 0; i<NUM_FREE_TASK_FRAMES; i++){
        char name[TASK_NAME_MAX_CHAR+1];
        tid_to_name[i] = &name;
    }
    char msg[TASK_NAME_MAX_CHAR+2];
    int tid;
    char reply[TASK_NAME_MAX_CHAR+1];
    for(;;){
        int msglen = Receive(&tid, msg, 2);
        msg[msglen] = '\0';
        if(msglen<2){
            uart_printf(CONSOLE, "\x1b[31mInvalid msg received by name_server\x1b[0m\r\n");
            for(;;){}
        }
        if(msg[0]=='r'){
            char *arg_name = msg+1;
            int index = 0;
            while (*(arg_name+index)!='\0'){
                tid_to_name[tid][index] = *(arg_name+index);
                index += 1;
            }
            const char *reply = "\0";
            Reply(tid, reply, 0);
        }else if(msg[0]=='w'){
            char *arg_name = msg+1;
            int reply_tid = -1;
            for(int i = 0; i<NUM_FREE_TASK_FRAMES; i++){
                char *c1 = tid_to_name[i];
                char *c2 = arg_name;
                if(str_cmp(c1, c2)){
                    reply_tid = i;
                    break;
                }
            }
            if(reply_tid==-1){
                const char *reply = "\0";
                Reply(tid, reply, 0);
            }else{
                char reply[3];
                int rplen = i2a(reply_tid, reply);
                reply[rplen] = '\0';
                Reply(tid, (const char *)reply, rplen);
            }
        }else{
            uart_printf(CONSOLE, "\x1b[31mInvalid msg received by name_server\x1b[0m\r\n");
            for(;;){}
        }
        
    }
}

void game_client(){
    const char *name = "game_client1";
    RegisterAs(name);
    int server_tid = WhoIs("game_server");
    uart_printf(CONSOLE, "Server tid: %d\r\n", server_tid);
}

void game_server(){
    const char *name = "game_server";
    RegisterAs(name);
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

void reschedule_task_with_return(Heap heap, TaskFrame *tf, long long ret){
    tf->x[0] = ret;
    tf->added_time = get_time();
    heap_push(&heap, tf);
}

int copy_msg(char *src, int srclen, char *dest, int destlen){
    int reslen = (srclen < destlen) ? srclen : destlen;
    for(int i = 0; i<reslen; i++){
        dest[i] = src[i];
    }
    return reslen;
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

    // FIRST TASKS INITIALIZATION
    TaskFrame* name_server_task = getNextFreeTaskFrame();
    task_init(name_server_task, 2, get_time(), &name_server, next_user_tid, kf->tid, (uint64_t)&Exit, 0x600002C0);
    next_user_tid+=1;
    heap_push(&heap, name_server_task);

    TaskFrame* game_server_task = getNextFreeTaskFrame();
    task_init(game_server_task, 2, get_time(), &game_server, next_user_tid, kf->tid, (uint64_t)&Exit, 0x600002C0);
    next_user_tid+=1;
    heap_push(&heap, game_server_task);

    TaskFrame* game_client_task1 = getNextFreeTaskFrame();
    task_init(game_client_task1, 2, get_time(), &game_client, next_user_tid, kf->tid, (uint64_t)&Exit, 0x600002C0);
    next_user_tid+=1;
    heap_push(&heap, game_client_task1);

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

            asm volatile(
                "mov %[priority], x0\n"
                "mov %[function], x1"
                : 
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
        } else if(exception_code==SEND){
            int tid;
            const char *msg;
            int msglen;
            char *reply;
            int rplen;
            asm volatile(
                "mov %[tid], x0\n"
                "mov %[msg], x1\n"
                "mov %[msglen], x2\n"
                "mov %[reply], x3\n"
                "mov %[rplen], x4\n"
                : 
                [tid]"=r"(tid),
                [msg]"=r"(msg),
                [msglen]"=r"(msglen),
                [reply]"=r"(reply),
                [rplen]"=r"(rplen)
            );
            // find recipient tid, if not exist, return -1
            // if sender task not in ready, error
            // if recipient in ready, sender status ready->send-wait and add to recipient queue
            // if recipient in receive-wait, copy data, recipient wait->ready, sender ready->reply-wait
            // receive data can be in receive task frame?
            if(tid>NUM_FREE_TASK_FRAMES || user_tasks[tid-1].status==INACTIVE){
                reschedule_task_with_return(heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *recipient = user_tasks + tid-1;
            if(currentTaskFrame->sd != NULL || currentTaskFrame->status!=READY || recipient->status==SEND_WAIT || recipient->status==REPLY_WAIT){
                reschedule_task_with_return(heap, currentTaskFrame, -2);
                continue;
            }
            if(recipient->status==READY){
                SendData sd;
                sd.tid = tid;
                sd.msg = msg;
                sd.msglen = msglen;
                sd.reply = reply;
                sd.rplen = rplen;
                currentTaskFrame->sd = &sd;
                currentTaskFrame->status = SEND_WAIT;
                recipient->sender_queue[recipient->sender_queue_len] = currentTaskFrame->tid;
                recipient->sender_queue_len += 1;
            }else if(recipient->status==RECEIVE_WAIT){
                ReceiveData *rd = recipient->rd;
                if(rd==NULL){
                    reschedule_task_with_return(heap, currentTaskFrame, -2);
                    continue;
                }
                copy_msg(msg, msglen, rd->msg, rd->msglen);
                currentTaskFrame->status = REPLY_WAIT;
                recipient->status = READY;
                recipient->rd = NULL;
                reschedule_task_with_return(heap, recipient, msglen);
            }
            
        } else if(exception_code==RECEIVE){
            int *tid;
            const char *msg;
            int msglen;
            asm volatile(
                "mov %[tid], x0\n"
                "mov %[msg], x1\n"
                "mov %[msglen], x2\n"
                : 
                [tid]"=r"(tid),
                [msg]"=r"(msg),
                [msglen]"=r"(msglen)
            );
            // if receive status not in ready, error
            // check queue to see if any senders ready
            // if not, status ready->receive-wait
            // if so, remove sender from queue, copy data, sender send-wait->reply-wait
            // if sender task is not in send-wait, error 
            // sender data can be in queue?
            if(currentTaskFrame->status!=READY){
                uart_printf(CONSOLE, "\x1b[31mOn receive task status not ready %u\x1b[0m\r\n", exception_code);
                for(;;){}
            }
            if(currentTaskFrame->sender_queue_len==0){
                if(currentTaskFrame->rd!=NULL){
                    uart_printf(CONSOLE, "\x1b[31mOn receive task rd not null %u\x1b[0m\r\n", exception_code);
                    for(;;){}
                }
                ReceiveData rd;
                rd.msg = msg;
                rd.msglen = msglen;
                rd.tid = tid;
                currentTaskFrame->rd = &rd;
                currentTaskFrame->status = RECEIVE_WAIT;
            }else{
                int sender_tid = currentTaskFrame->sender_queue[0];
                if(sender_tid>NUM_FREE_TASK_FRAMES){
                    uart_printf(CONSOLE, "\x1b[31mOn receive sender tid invalid %u\x1b[0m\r\n", exception_code);
                    for(;;){}
                }
                TaskFrame *sender = user_tasks + sender_tid - 1;
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
                reschedule_task_with_return(heap, currentTaskFrame, sd->msglen);
            }
        } else if(exception_code==REPLY){
            int tid;
            const char *msg;
            int msglen;
            asm volatile(
                "mov %[tid], x0\n"
                "mov %[msg], x1\n"
                "mov %[msglen], x2\n"
                : 
                [tid]"=r"(tid),
                [msg]"=r"(msg),
                [msglen]"=r"(msglen)
            );
            // if receive status not in ready or send status not in reply-wait, error
            // copy data and set both status to ready
            if(tid>NUM_FREE_TASK_FRAMES || user_tasks[tid-1].status==INACTIVE){
                reschedule_task_with_return(heap, currentTaskFrame, -1);
                continue;
            }
            TaskFrame *sender = user_tasks + tid-1;
            if(sender->status!=REPLY_WAIT){
                reschedule_task_with_return(heap, currentTaskFrame, -2);
                continue;
            }
            SendData *sd = sender->sd;
            sender->sd = NULL;
            int reslen = copy_msg(msg, msglen, sd->reply, sd->rplen);
            reschedule_task_with_return(heap, currentTaskFrame, reslen);
            reschedule_task_with_return(heap, sender, msglen);
        } else{
            uart_printf(CONSOLE, "\x1b[31mUnrecognized exception code %u\x1b[0m\r\n", exception_code);
            for(;;){}
        }
    }

    return 0;
}

