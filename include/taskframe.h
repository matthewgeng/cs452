#ifndef TASKFRAME_H
#define TASKFRAME_H

#include <stdint.h>
#include <stddef.h>
#include "int_cb.h"

typedef struct SendData {
  int tid;
  char *msg;
  int msglen;
  char *reply;
  int rplen;
  struct SendData *next;
} SendData;

typedef struct ReceiveData {
  int *tid;
  char *msg;
  int msglen;
  struct ReceiveData *next;
} ReceiveData;

typedef struct TaskFrame {
  long long x[31];
  uint64_t sp;
  uint64_t pc;
  uint64_t spsr;
  int tid;
  int parentTid;
  uint32_t added_time;
  uint32_t priority;
  uint16_t status; // 0=inactive, 1=ready, 2=send-wait, 3=receive-wait, 4=reply-wait
  struct SendData *sd;
  struct ReceiveData *rd;
  IntCB sender_queue;
  int sender_queue_data[MAX_NUM_TASKS];
  uint8_t sp_size;
  struct TaskFrame *next;
} TaskFrame;

void tasks_init(TaskFrame* task_frames, TaskFrame *nextFreeTaskFrame[]);
int task_cmp(const TaskFrame *tf1, const TaskFrame* tf2);
TaskFrame *getNextFreeTaskFrame(TaskFrame *nextFreeTaskFrame[], int sp_size);
void reclaimTaskFrame(TaskFrame **nextFreeTaskFrame, TaskFrame *tf);

SendData *sds_init(SendData* sds, size_t size);
SendData *getNextFreeSendData(SendData **nextFreeSendData);
void reclaimSendData(SendData **nextFreeSendData, SendData *sd);

ReceiveData *rds_init(ReceiveData* rds, size_t size);
ReceiveData *getNextFreeReceiveData(ReceiveData **nextFreeReceiveData);
void reclaimReceiveData(ReceiveData **nextFreeReceiveData, ReceiveData *rd);

// defined in linker script, used & (address operator) to get linker address
extern char user_stack_start;

#endif
