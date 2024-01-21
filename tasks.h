#ifndef _tasks_h_
#define _tasks_h_ 1

#include <stdint.h>
#include <stddef.h>

#define NUM_FREE_TASK_FRAMES 20

struct TaskFrame {
  int tid;
  int parentTid;
  uint64_t sp;
  uint32_t priority;
  void (*function)();
  struct TaskFrame *next;
};

void initializeTasks(struct TaskFrame *tfs);
struct TaskFrame *getNextFreeTaskFrame();
void reclaimTaskFrame(struct TaskFrame *tf);
uint32_t getNextTid();

void insertTaskFrame(struct TaskFrame *tf);
struct TaskFrame *popNextTaskFrame();

#endif /* tasks.h */
