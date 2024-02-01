#ifndef SYSCALL_H
#define SYSCALL_H

#define CREATE 1
#define MY_TID 2
#define MY_PARENT_TID 3
#define YIELD 4
#define EXIT 5
#define SEND 6
#define RECEIVE 7
#define REPLY 8

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

#endif
