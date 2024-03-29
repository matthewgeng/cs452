#ifndef SYSCALL_H
#define SYSCALL_H

// K1
#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5

// K2
#define SEND 6
#define RECEIVE 7
#define REPLY 8

// K3
#define AWAIT_EVENT 9
#define NUM_IRQ_EVENTS 6 // note this is 1 less than in the enum due to the TODO
#define IRQ 10

// K4
#define QUIT_SYSCALL 11

#define SYSCALL 0

typedef enum {
    CLOCK,
    CONSOLE_TX,
    CONSOLE_RX,
    MARKLIN_TX,
    MARKLIN_RX,
    MARKLIN_CTS,
    TODO
} IRQ_TYPE;

// K1
int Create(int priority, void (*function)());
int MyTid();
int MyParentTid();
void Yield();
void Exit();

// K2
int Send(int tid, const char *msg, int msglen, char *reply, int rplen);
int Receive(int *tid, char *msg, int msglen);
int Reply(int tid, const char *reply, int rplen);

// K3
// blocks until the event identified by eventid occurs then returns with event-specific data, if any. -1 = invalid event
int AwaitEvent(int eventType);

// K4
void Quit();

#endif
