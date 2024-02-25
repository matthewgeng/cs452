#include "taskframe.h"

TaskFrame *tasks_init(TaskFrame* task_frames, size_t stack_base, size_t stack_size, size_t num_task_frames) {

    for(size_t i = 0; i<num_task_frames; i++){
        if(i<num_task_frames-1){
            task_frames[i].next = task_frames+i+1;
        }else{
            task_frames[i].next = NULL;
        }
        // stack initialization
        task_frames[i].sp = stack_base + i*stack_size;
        task_frames[i].tid = i;
        task_frames[i].status = INACTIVE;
        task_frames[i].sd = NULL;
        task_frames[i].rd = NULL;
        initialize_intcb(&(task_frames[i].sender_queue), task_frames[i].sender_queue_data, MAX_NUM_TASKS, 0);
    }
    return task_frames;
}

int task_cmp(const TaskFrame* tf1, const TaskFrame* tf2) {
    if (tf1->priority < tf2->priority) {
        return -1;
    } else if (tf1->priority > tf2->priority) {
        return 1;
    } else {
        // if tf1 is older than tf2
        if (tf1->added_time < tf2->added_time) {
            return -1;
        } else if (tf1->added_time > tf2->added_time) {
            return 1;
        }
    }
    return 0;
}

TaskFrame *getNextFreeTaskFrame(TaskFrame **nextFreeTaskFrame){
    if(*nextFreeTaskFrame==NULL){
        return NULL;
    }
    TaskFrame *tf = *nextFreeTaskFrame;
    tf->status = READY;
    *nextFreeTaskFrame = (*nextFreeTaskFrame)->next;
    return tf;
}

void reclaimTaskFrame(TaskFrame **nextFreeTaskFrame, TaskFrame *tf){
    tf->status = INACTIVE;
    tf->next = *nextFreeTaskFrame;
    *nextFreeTaskFrame = tf;
}

SendData *sds_init(SendData* sds, size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            sds[i].next = sds+i+1;
        }else{
            sds[i].next = NULL;
        }
    }
    return sds;
}
SendData *getNextFreeSendData(SendData **nextFreeSendData){
    if(*nextFreeSendData==NULL){
        return NULL;
    }
    SendData *sd = *nextFreeSendData;
    *nextFreeSendData = (*nextFreeSendData)->next;
    return sd;
}
void reclaimSendData(SendData **nextFreeSendData, SendData *sd){
    sd->next = *nextFreeSendData;
    *nextFreeSendData = sd;
}

ReceiveData *rds_init(ReceiveData* rds, size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            rds[i].next = rds+i+1;
        }else{
            rds[i].next = NULL;
        }
    }
    return rds;
}
ReceiveData *getNextFreeReceiveData(ReceiveData **nextFreeReceiveData){
    if(*nextFreeReceiveData==NULL){
        return NULL;
    }
    ReceiveData *rd = *nextFreeReceiveData;
    *nextFreeReceiveData = (*nextFreeReceiveData)->next;
    return rd;
}
void reclaimReceiveData(ReceiveData **nextFreeReceiveData, ReceiveData *rd){
    rd->next = *nextFreeReceiveData;
    *nextFreeReceiveData = rd;
}
