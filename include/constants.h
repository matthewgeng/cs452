#ifndef CONSTANTS_H
#define CONSTANTS_H

#define NUM_TASKS {20,2,1}
#define MAX_NUM_TASKS 23
#define NAME_SERVER_TID 20
#define IDLE_TID 1

#define TASK_SIZES {4096,40960,409600}
#define USER_STACK_SIZE 2048
#define SENDER_QUEUE_SIZE 5

#define INACTIVE 0
#define READY 1
#define SEND_WAIT 2
#define RECEIVE_WAIT 3
#define REPLY_WAIT 4

#endif
