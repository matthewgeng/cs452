#include "taskframe.h"

void tasks_init(TaskFrame* task_frames, TaskFrame **nextFreeTaskFrame) {

    size_t cur_stack = USER_STACK_START;
    uint8_t *task_nums = NUM_TASKS;
    uint32_t *task_sizes =  TASK_SIZES;
    uint8_t tf_index = 0;
    for(int k = 0; k<3; k++){
        for(size_t i = 0; i<task_nums[k]; i++){
            if(i<task_nums[k]-1){
                task_frames[i].next = task_frames+i+1;
            }else{
                task_frames[i].next = NULL;
            }
            // stack initialization
            task_frames[i].tid = i;
            task_frames[i].status = INACTIVE;
            task_frames[i].sd = NULL;
            task_frames[i].rd = NULL;
            task_frames[i].sp_size = k;
            task_frames[i].sp = cur_stack;
            cur_stack += task_sizes[k];
            initialize_intcb(&(task_frames[i].sender_queue), task_frames[i].sender_queue_data, MAX_NUM_TASKS, 0);
        }
        nextFreeTaskFrame[k] = task_frames + tf_index;
        tf_index += task_nums[k];
    }
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

TaskFrame *getNextFreeTaskFrame(TaskFrame **nextFreeTaskFrame, uint8_t sp_size){
    if(nextFreeTaskFrame[sp_size]==NULL){
        return NULL;
    }
    TaskFrame *tf = nextFreeTaskFrame[sp_size];
    tf->status = READY;
    nextFreeTaskFrame[sp_size] = nextFreeTaskFrame[sp_size]->next;
    return tf;
}

void reclaimTaskFrame(TaskFrame **nextFreeTaskFrame, TaskFrame *tf){
    tf->status = INACTIVE;
    tf->next = nextFreeTaskFrame[tf->sp_size];
    nextFreeTaskFrame[tf->sp_size] = tf;
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
