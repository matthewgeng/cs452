#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>

#define INACTIVE 0
#define READY 1
#define SEND_WAIT 2
#define RECEIVE_WAIT 3
#define REPLY_WAIT 4

#define MAX_NUM_TASKS 20
#define USER_STACK_START 580000
#define USER_STACK_SIZE 1024
#define SENDER_QUEUE_SIZE 5


typedef struct SendData {
  int tid;
  const char *msg;
  int msglen;
  char *reply;
  int rplen;
} SendData;

typedef struct ReceiveData {
  int *tid;
  const char *msg;
  int msglen;
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
  // TODO: make this a circular array
  int sender_queue[SENDER_QUEUE_SIZE];
  int sender_queue_len;
  struct TaskFrame *next;
} TaskFrame;

void tasks_init(TaskFrame* task_frames, size_t stack_base, size_t stack_size, size_t num_task_frames);
int task_cmp(const TaskFrame *tf1, const TaskFrame* tf2);
TaskFrame *getNextFreeTaskFrame();
void reclaimTaskFrame(TaskFrame *tf);

#endif
