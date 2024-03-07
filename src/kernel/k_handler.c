#include "k_handler.h"
#include "syscall.h"
#include "rpi.h"
#include "interrupt.h"
#include "timer.h"
#include "util.h"

void task_init(TaskFrame* tf, uint32_t priority, uint32_t time, void (*function)(), uint32_t parent_tid, uint64_t lr, uint64_t spsr, uint16_t status) {
    tf->priority = priority;
    tf->pc = (uint64_t)function;
    tf->parentTid = parent_tid;
    tf->x[30] = lr;
    tf->spsr = spsr;
    tf->added_time = time;
    tf->status = status;
}

int copy_msg(const char *src, int srclen, char *dest, int destlen){
    int reslen = (srclen < destlen) ? srclen : destlen;
    memcpy(dest, src, reslen);
    return reslen;
}

void reschedule_task_with_return(Heap *heap, TaskFrame *tf, long long ret){
    tf->x[0] = ret;
    tf->added_time = sys_time();
    heap_push(heap, tf);
}

void handle_create(Heap *heap, TaskFrame **nextFreeTaskFrame){
    int priority = (int)(currentTaskFrame->x[0]);
    void (*function)() = (void *)(currentTaskFrame->x[1]);
    int sp_size = (void *)(currentTaskFrame->x[2]);
    if(priority<0){
        uart_printf(CONSOLE, "\x1b[31mInvalid priority %d\x1b[0m\r\n", priority);
        reschedule_task_with_return(heap, currentTaskFrame, -1);
        return;
    }
    TaskFrame* created_task = getNextFreeTaskFrame(nextFreeTaskFrame, sp_size);
    if(created_task==NULL){
        uart_printf(CONSOLE, "\x1b[31mKernel out of task descriptors\x1b[0m\r\n");
        reschedule_task_with_return(heap, currentTaskFrame, -2);
        return;
    }
    task_init(created_task, priority, sys_time(), function, currentTaskFrame->tid, (uint64_t)&Exit, 0x60000240, READY);
    heap_push(heap, created_task);
    reschedule_task_with_return(heap, currentTaskFrame, created_task->tid);
    uart_printf(CONSOLE, "handled create: priority: %d, sp_size: %d, tid: %d\r\n", priority, sp_size, created_task->tid);
}

void handle_exit(TaskFrame **nextFreeTaskFrame){
    int num_tasks_waiting_on_msg = currentTaskFrame->sender_queue.count;
    if (num_tasks_waiting_on_msg > 0) {
        uart_printf(CONSOLE, "\x1b[31mTask %u exiting with %u waiting tasks \x1b[0m\r\n", currentTaskFrame->tid, num_tasks_waiting_on_msg);
    }

    reclaimTaskFrame(nextFreeTaskFrame, currentTaskFrame);
}

void handle_my_tid(Heap *heap){
    reschedule_task_with_return(heap, currentTaskFrame, currentTaskFrame->tid);

}

void handle_parent_tid(Heap *heap){
    reschedule_task_with_return(heap, currentTaskFrame, currentTaskFrame->parentTid);

}

void handle_yield(Heap *heap){
    reschedule_task_with_return(heap, currentTaskFrame, 0);

}

void handle_send(Heap *heap, TaskFrame user_tasks[], SendData **nextFreeSendData, ReceiveData **nextFreeReceiveData){

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
        reschedule_task_with_return(heap, currentTaskFrame, -1);
        return;
    }
    TaskFrame *recipient = user_tasks + tid;
    if(currentTaskFrame->sd != NULL || currentTaskFrame->status!=READY || recipient->status==SEND_WAIT || recipient->status==REPLY_WAIT){
        uart_printf(CONSOLE, "\x1b[31mOn send sender/recipient status not valid %d %d %d\x1b[0m\r\n", currentTaskFrame->sd, currentTaskFrame->status, recipient->status);
        reschedule_task_with_return(heap, currentTaskFrame, -2);
        return;
    }

    SendData *sd = getNextFreeSendData(nextFreeSendData);
    sd->tid = tid;
    sd->msg = msg;
    sd->msglen = msglen;
    sd->reply = reply;
    sd->rplen = rplen;
    currentTaskFrame->sd = sd;
    if(recipient->status==READY){
        currentTaskFrame->status = SEND_WAIT;
        push_intcb(&(recipient->sender_queue), currentTaskFrame->tid);
    }else if(recipient->status==RECEIVE_WAIT){

        ReceiveData *rd = recipient->rd;
        if(rd==NULL){
            uart_printf(CONSOLE, "\x1b[31mOn send recipient rd is null\x1b[0m\r\n");
            reschedule_task_with_return(heap, currentTaskFrame, -2);
            return;
        }
        *(rd->tid) = currentTaskFrame->tid;
        copy_msg(msg, msglen, rd->msg, rd->msglen);
        currentTaskFrame->status = REPLY_WAIT;
        recipient->status = READY;
        reclaimReceiveData(nextFreeReceiveData, recipient->rd);
        recipient->rd = NULL;
        reschedule_task_with_return(heap, recipient, msglen);
    }
}

void handle_receive(Heap *heap, TaskFrame user_tasks[], ReceiveData **nextFreeReceiveData){

    int *tid = (int *)(currentTaskFrame->x[0]);
    char *msg = (const char *)(currentTaskFrame->x[1]);
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
    if(is_empty_intcb(&currentTaskFrame->sender_queue)){
        if(currentTaskFrame->rd!=NULL){
            uart_printf(CONSOLE, "\x1b[31mOn receive task rd not null\x1b[0m\r\n");
            for(;;){}
        }
        ReceiveData *rd = getNextFreeReceiveData(nextFreeReceiveData);
        rd->msg = msg;
        rd->msglen = msglen;
        rd->tid = tid;
        currentTaskFrame->status = RECEIVE_WAIT;
        currentTaskFrame->rd = rd;
    }else{
        int sender_tid = pop_intcb(&(currentTaskFrame->sender_queue));
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
        reschedule_task_with_return(heap, currentTaskFrame, sd->msglen);
    }
}

void handle_reply(Heap *heap, TaskFrame user_tasks[], SendData **nextFreeSendData){

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
        reschedule_task_with_return(heap, currentTaskFrame, -1);
        return;
    }
    TaskFrame *sender = user_tasks + tid;
    if(sender->status!=REPLY_WAIT){
        uart_printf(CONSOLE, "\x1b[31mOn reply replier %u to sender %u status not reply_wait %d\x1b[0m\r\n",currentTaskFrame->tid, tid, sender->status);
        reschedule_task_with_return(heap, currentTaskFrame, -2);
        return;
    }
    if(currentTaskFrame->status!=READY){
        uart_printf(CONSOLE, "\x1b[31mOn reply current status not ready %d\x1b[0m\r\n", sender->status);
        for(;;){}
    }            

    sender->status = READY;
    SendData *sd = sender->sd;
    reclaimSendData(nextFreeSendData, sender->sd);
    sender->sd = NULL;
    int reslen = copy_msg(reply, rplen, sd->reply, sd->rplen);
    reschedule_task_with_return(heap, currentTaskFrame, reslen);
    reschedule_task_with_return(heap, sender, rplen);

}

void handle_await_event(Heap *heap, TaskFrame* blocked_on_irq[]){

    int type = (int)(currentTaskFrame->x[0]);
    #if DEBUG
        uart_dprintf(CONSOLE, "AwaitEvent %d\r\n", type);
    #endif 

    if (type < CLOCK || type > TODO) {
        reschedule_task_with_return(heap, currentTaskFrame, -1);
        return;
    }

    // re-enable interrupt for specific i/o irq, since this is turned off during actual
    // irq handling 
    if (type == CONSOLE_TX) {
        uart_enable_tx(CONSOLE);
    } else if (type == CONSOLE_RX) {
        uart_enable_rx(CONSOLE);
    } else if (type == MARKLIN_TX) {
        uart_enable_tx(MARKLIN);
    } else if (type == MARKLIN_RX) {
        uart_enable_rx(MARKLIN);
    } else if (type == MARKLIN_CTS) {
        uart_enable_cts(MARKLIN);
    }

    // TODO: not sure if we should buffer the tasks and what to return when we reach that
    // currently, we can only have a single element waiting
    if (blocked_on_irq[type] != NULL) {
        uart_printf(CONSOLE, "Existing task %u already waiting for %d\r\n", blocked_on_irq[type]->tid, type);
        reschedule_task_with_return(heap, currentTaskFrame, -2);
        return;
    }

    blocked_on_irq[type] = currentTaskFrame;
}

void handle_irq(Heap *heap, TaskFrame* blocked_on_irq[]){

    #if DEBUG
        uart_dprintf(CONSOLE, "On irq\r\n");
    #endif 

    uint32_t irq_ack = *(uint32_t*)(GICC_IAR);
    uint32_t irq_id_bit_mask = 0x3FF; // 10 bits
    uint32_t irq = (irq_ack & irq_id_bit_mask);

    if (irq == SYSTEM_TIMER_IRQ_1) {
        if (blocked_on_irq[CLOCK] != NULL) {
            reschedule_task_with_return(heap, blocked_on_irq[CLOCK], 0);
            blocked_on_irq[CLOCK] = NULL;
        }
        update_c1();
        update_status_reg();
    } else if (irq == UART_IRQ){
        // NOTE: don't add any print statements before handling since it'll mess up the handling state

        // console irq
        // tx irq
        if (uart_mis_tx(CONSOLE)) {
            uart_disable_tx(CONSOLE);
            // uart_printf(CONSOLE, "uart console tx irq\r\n");

            if (blocked_on_irq[CONSOLE_TX] != NULL) {
                reschedule_task_with_return(heap, blocked_on_irq[CONSOLE_TX], 0);
                blocked_on_irq[CONSOLE_TX] = NULL;
            }
            uart_clear_tx(CONSOLE);

        // rx irq
        } else if (uart_mis_rx(CONSOLE)) {
            uart_disable_rx(CONSOLE);
            // uart_printf(CONSOLE, "uart console rx irq\r\n");
            if (blocked_on_irq[CONSOLE_RX] != NULL) {
                reschedule_task_with_return(heap, blocked_on_irq[CONSOLE_RX], 0);
                blocked_on_irq[CONSOLE_RX] = NULL;
            }
            uart_clear_rx(CONSOLE);

        // marklin irq
        // tx irq
        } else if (uart_mis_tx(MARKLIN)) {
            uart_disable_tx(MARKLIN);
            if (blocked_on_irq[MARKLIN_TX] != NULL) {
                reschedule_task_with_return(heap, blocked_on_irq[MARKLIN_TX], 0);
                blocked_on_irq[MARKLIN_TX] = NULL;
            }
            uart_clear_tx(MARKLIN);

        // rx irq
        } else if (uart_mis_rx(MARKLIN)) {
            uart_disable_rx(MARKLIN);
            if (blocked_on_irq[MARKLIN_RX] != NULL) {
                reschedule_task_with_return(heap, blocked_on_irq[MARKLIN_RX], 0);
                blocked_on_irq[MARKLIN_RX] = NULL;
            }
            uart_clear_rx(MARKLIN);
        } else if (uart_mis_cts(MARKLIN)) {
            uart_disable_cts(MARKLIN);
            // uart_printf(CONSOLE, "\nHELLO interrupt cts %u\r\n", uart_cts(MARKLIN));
            if (blocked_on_irq[MARKLIN_CTS] != NULL) {
                reschedule_task_with_return(heap, blocked_on_irq[MARKLIN_CTS], 0);
                blocked_on_irq[MARKLIN_CTS] = NULL;
            }
            uart_clear_cts(MARKLIN);
        } else {
            // invalid irq event
            uart_printf(CONSOLE, "Invalid/Unsupported UART IRQ mis register not valid %u\r\n");
            // for(;;){}
        }
    } else {
        // invalid event
        uart_printf(CONSOLE, "Invalid/Unsupported IRQ %u\r\n", irq);
        // for(;;){}
    }
    
    // finish irq processing
    *(uint32_t*)(GICC_EOIR) = irq_ack;

    // reschedule 
    heap_push(heap, currentTaskFrame);
}
