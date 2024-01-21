#include "tasks.h"


struct TaskFrame *nextFreeTaskFrame;
uint32_t nextTid;

//max-heap
struct TaskFrame *taskFrameHeap[NUM_FREE_TASK_FRAMES];
uint32_t tfHeapLength;

static uint32_t nextUserStackBaseAddr;

void initializeTasks(struct TaskFrame *tfs){

  nextUserStackBaseAddr = USER_STACK_START;

  nextTid = 1;
  tfHeapLength = 0;
  for(int i = 0; i<NUM_FREE_TASK_FRAMES; i++){
    if(i<NUM_FREE_TASK_FRAMES-1){
      tfs[i].next = tfs+i+1;
    }else{
      tfs[i].next = NULL;
    }
    tfs[i].tid = 0;
    tfs[i].parentTid = 0;
    tfs[i].sp = 0;
    tfs[i].priority = 0;
    tfs[i].function = NULL;
  }
  nextFreeTaskFrame = tfs;

}


struct TaskFrame *getNextFreeTaskFrame(){
  if(nextFreeTaskFrame==NULL){
    // TODO: error
    return NULL;
  }
  struct TaskFrame *tf = nextFreeTaskFrame;
  nextFreeTaskFrame = nextFreeTaskFrame->next;
  return tf;
}

void reclaimTaskFrame(struct TaskFrame *tf){
  tf->next = nextFreeTaskFrame;
  nextFreeTaskFrame = tf;
}

uint32_t getNextTid(){
  nextTid += 1;
  return nextTid-1;
}

void tfHeapSwap(uint32_t i1, uint32_t i2){
  struct TaskFrame *tmp = taskFrameHeap[i1];
  taskFrameHeap[i1] = taskFrameHeap[i2];
  taskFrameHeap[i2] = tmp;
}

void insertTaskFrame(struct TaskFrame *tf){
  // TODO: assert taskFrameHeap not overflowing
  uint32_t i = tfHeapLength;
  uint32_t parent = (i-1)/2;
  taskFrameHeap[i] = tf;
  tfHeapLength += 1;
  while(i!=0 && tf->priority > taskFrameHeap[parent]->priority){
    tfHeapSwap(i, parent);
    i = parent;
    if(i!=0){
      parent = (i-1)/2;
    }
  }
}

struct TaskFrame *popNextTaskFrame(){
  // TODO: assert tfHeapLength>0
  if(tfHeapLength==0){
    return NULL;
  }

  struct TaskFrame *ret = taskFrameHeap[0];
  tfHeapLength-=1;
  if(tfHeapLength>0){
    taskFrameHeap[0] = taskFrameHeap[tfHeapLength];
    uint32_t i = 0;
    while(i<tfHeapLength){
      uint32_t child1 = i*2+1;
      uint32_t child2 = (i+1)*2;
      if(child1<tfHeapLength && child2>=tfHeapLength && taskFrameHeap[child1]>taskFrameHeap[i]){
        tfHeapSwap(i, child1);
        i = child1;
      }else if(child2<tfHeapLength && taskFrameHeap[child1]>taskFrameHeap[i] && taskFrameHeap[child1]>taskFrameHeap[child2]){
        tfHeapSwap(i, child1);
        i = child1;
      }else if(child2<tfHeapLength && taskFrameHeap[child1]>taskFrameHeap[i] && taskFrameHeap[child1]<=taskFrameHeap[child2]){
        tfHeapSwap(i, child2);
        i = child2;
      }else{
        i = tfHeapLength;
      }
    }
  }
  return ret;
}

uint32_t getNextUserStackPointer(){
    nextUserStackBaseAddr-=USER_STACK_SIZE;
    return nextUserStackBaseAddr + USER_STACK_SIZE;
}
