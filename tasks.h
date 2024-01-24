#ifndef _tasks_h_
#define _tasks_h_ 1

#include <stdint.h>
#include <stddef.h>

#define NUM_FREE_TASK_FRAMES 20
#define USER_STACK_START 580000
#define USER_STACK_SIZE 1000


/*
task: 
- tid
- parent tid
- x0-x31
- program counter
- stack base pointer
- maybe stack size?
- fr (frame register)?
- lr (link register)?
- function
*/

typedef struct TaskFrame {
  uint64_t x[31];
  uint64_t sp;
  uint64_t pc;
  uint64_t spsr;
  int tid;
  int parentTid;
  uint32_t added_time;
  uint32_t priority;
  struct TaskFrame *next;
} TaskFrame;

void tasks_init(TaskFrame* task_frames, size_t stack_base, size_t stack_size, size_t num_task_frames);
int task_cmp(const TaskFrame *tf1, const TaskFrame* tf2);
TaskFrame *getNextFreeTaskFrame();
void reclaimTaskFrame(TaskFrame *tf);

#endif /* tasks.h */
