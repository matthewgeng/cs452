#ifndef K_HANDLER_H
#define K_HANDLER_H

#include "taskframe.h"
#include "heap.h"

extern TaskFrame* kf;
extern TaskFrame* currentTaskFrame;

void task_init(TaskFrame* tf, uint32_t priority, uint32_t time, void (*function)(), uint32_t parent_tid, uint64_t lr, uint64_t spsr, uint16_t status);

void handle_create(Heap *heap, TaskFrame *nextFreeTaskFrame);
void handle_exit(TaskFrame *nextFreeTaskFrame);
void handle_my_tid(Heap *heap);
void handle_parent_tid(Heap *heap);
void handle_yield(Heap *heap);
void handle_send(Heap *heap, TaskFrame user_tasks[], SendData *nextFreeSendData, ReceiveData *nextFreeReceiveData);
void handle_receive(Heap *heap, TaskFrame user_tasks[], ReceiveData *nextFreeReceiveData);
void handle_reply(Heap *heap, TaskFrame user_tasks[], SendData *nextFreeSendData);
void handle_await_event(Heap *heap, TaskFrame* blocked_on_irq[]);
void handle_irq(Heap *heap, TaskFrame* blocked_on_irq[]);

#endif