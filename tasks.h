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

struct TaskFrame {
  uint64_t x[31];
  uint64_t fp;
  uint64_t lr;
  uint64_t sp;
  uint64_t pc;
  uint64_t spsr;
  int tid;
  int parentTid;
  uint32_t priority;
  struct TaskFrame *next;
};

void initializeTasks(struct TaskFrame *tfs);
void setNextTaskToBeScheduled(struct TaskFrame *tf);
struct TaskFrame *getNextFreeTaskFrame();
void reclaimTaskFrame(struct TaskFrame *tf);
uint32_t getNextTid();

void insertTaskFrame(struct TaskFrame *tf);
struct TaskFrame *getNextTask();

uint32_t getNextUserStackPointer();

#endif /* tasks.h */
