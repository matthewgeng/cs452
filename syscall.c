#include "syscall.h"
#include "rpi.h"

int Create(int priority, void (*function)()){
    #if DEBUG
        uart_dprintf(CONSOLE, "creating task: %d %x \r\n", priority, function);
    #endif 
    
    int tid;

    asm volatile(
        "mov x0, %[priority]\n"
        "mov x1, %[function]\n"
        "svc %[SYS_CODE]"
        :
        : [priority] "r" (priority),
        [function] "r" (function),
        [SYS_CODE] "i"(CREATE) 
        : "x9", "x10"
    );

    asm volatile("mov %0, x0" : "=r"(tid));
    return tid;
}


int MyTid(){
    #if DEBUG
        uart_dprintf(CONSOLE, "fetching tid \r\n");
    #endif 
    int tid;

    asm volatile(
        "svc %[SYS_CODE]"
        :
        : [SYS_CODE] "i"(MY_TID) 
    );
    asm volatile("mov %0, x0" : "=r"(tid));
    #if DEBUG
        uart_dprintf(CONSOLE, "MyTid: %d\r\n", tid);
    #endif 

    return tid;
}

int MyParentTid(){
    #if DEBUG
        uart_dprintf(CONSOLE, "fetching parent tid \r\n");
    #endif 
    int parent_tid;

    asm volatile(
        "svc %[SYS_CODE]"
        :
        : [SYS_CODE] "i"(MY_PARENT_TID) 
    );
    asm volatile("mov %0, x0" : "=r"(parent_tid));

    return parent_tid;
}

void Yield(){
    #if DEBUG
        uart_dprintf(CONSOLE, "task yielding \r\n");
    #endif 

    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(YIELD) 
    );
}

void Exit(){
    #if DEBUG
        uart_dprintf(CONSOLE, "task exiting \r\n");
    #endif 
    // TODO: maybe remove tid from nameserver
    asm volatile(
        "svc %[SYS_CODE]\n"
        :
        : [SYS_CODE] "i"(EXIT) 
    );
}

int Send(int tid, const char *msg, int msglen, char *reply, int rplen){

    // TODO: note that if a function gets a -2 return from send, should just throw error and halt
    
    #if DEBUG
        uart_dprintf(CONSOLE, "Send %d %d %d %d %d\r\n", tid, msg, msglen, reply, rplen);
    #endif 

    int intended_reply_len;

    asm volatile(
        "mov x9, %[tid]\n"
        "mov x10, %[msg]\n"
        "mov x11, %[msglen]\n"
        "mov x12, %[reply]\n"
        "mov x13, %[rplen]\n"
        "mov x0, x9\n"
        "mov x1, x10\n"
        "mov x2, x11\n"
        "mov x3, x12\n"
        "mov x4, x13\n"
        "svc %[SYS_CODE]"
        :
        : [tid] "r" (tid),
        [msg] "r" (msg),
        [msglen] "r" (msglen),
        [reply] "r" (reply),
        [rplen] "r" (rplen),
        [SYS_CODE] "i"(SEND) 
    );


    asm volatile("mov %0, x0" : "=r"(intended_reply_len));
    return intended_reply_len;
}

int Receive(int *tid, char *msg, int msglen){

    #if DEBUG
        uart_dprintf(CONSOLE, "Receive %d %d %d\r\n", tid, msg, msglen);
    #endif 

    int intended_sent_len;

    asm volatile(
        "mov x9, %[tid]\n"
        "mov x10, %[msg]\n"
        "mov x11, %[msglen]\n"
        "mov x0, x9\n"
        "mov x1, x10\n"
        "mov x2, x11\n"
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
    #if DEBUG
        uart_dprintf(CONSOLE, "Reply %d %d %d\r\n", tid, reply, rplen);
    #endif 

    asm volatile(
        "mov x9, %[tid]\n"
        "mov x10, %[reply]\n"
        "mov x11, %[rplen]\n"
        "mov x0, x9\n"
        "mov x1, x10\n"
        "mov x2, x11\n"
        "svc %[SYS_CODE]"
        :
        : 
        [tid] "r" (tid),
        [reply] "r" (reply),
        [rplen] "r" (rplen),
        [SYS_CODE] "i"(REPLY) 
    );    

    int actual_reply_len;
    asm volatile("mov %0, x0" : "=r"(actual_reply_len));
    return actual_reply_len;
}

int AwaitEvent(int eventType) {
    #if DEBUG
        uart_dprintf(CONSOLE, "AwaitEvent %d \r\n", eventType);
    #endif 

    asm volatile(
        "mov x9, %[eventType]\n"
        "mov x0, x9\n"
        "svc %[SYS_CODE]"
        :
        : 
        [eventType] "r" (eventType),
        [SYS_CODE] "i"(AWAIT_EVENT) 
    );    

    int ret;
    asm volatile("mov %0, x0" : "=r"(ret));
    return ret;
}
