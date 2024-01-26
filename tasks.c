#include "tasks.h"


TaskFrame *nextFreeTaskFrame;

void tasks_init(TaskFrame* task_frames, size_t stack_base, size_t stack_size, size_t num_task_frames) {
    nextFreeTaskFrame = task_frames;

    for(size_t i = 0; i<num_task_frames; i++){
        // intrusive linking. Why do we have this?
        if(i<num_task_frames-1){
            task_frames[i].next = task_frames+i+1;
        }else{
            task_frames[i].next = NULL;
        }
        // stack initialization
        task_frames[i].sp = stack_base + i*stack_size;
    }
}

int task_cmp(const TaskFrame* tf1, const TaskFrame* tf2) {
    if (tf1->priority < tf2->priority) {
        return -1;
    } else if (tf1->priority > tf2->priority) {
        return 1;
    } else {
        // if tf1 is older than tf2
        if (tf2->added_time > tf1->added_time) {
            return -1;
        } else if (tf1->added_time > tf2->added_time) {
            return 1;
        }
    }
    return 0;
}

TaskFrame *getNextFreeTaskFrame(){
  if(nextFreeTaskFrame==NULL){
    // TODO: handle later
    return NULL;
  }
  TaskFrame *tf = nextFreeTaskFrame;
  nextFreeTaskFrame = nextFreeTaskFrame->next;
  return tf;
}

void reclaimTaskFrame(TaskFrame *tf){
  tf->next = nextFreeTaskFrame;
  nextFreeTaskFrame = tf;
}
