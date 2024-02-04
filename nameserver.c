#include "nameserver.h"
#include "task.h"
#include "rpi.h"
#include "util.h"
#include "syscall.h"

int RegisterAs(const char *name){
    // assuming name_server tid is always 1
    uart_dprintf(CONSOLE, "RegisterAs \r\n");
    int name_len = str_len(name);
    char msg[name_len+1];
    msg[0]='r';
    memcpy(msg+1, name, name_len);
    
    int intended_reply_len = Send(1, msg, name_len+1, NULL, 0);
    if(intended_reply_len < 0){
        return -1;
    }
    if(intended_reply_len != 0){
        uart_printf(CONSOLE, "\x1b[31mRegisterAs unexpected behaviour %d\x1b[0m\r\n", intended_reply_len);
        for(;;){}
    }
    return 0;
}

int WhoIs(const char *name){
    uart_dprintf(CONSOLE, "WhoIs \r\n");
    int name_len = str_len(name);
    char msg[name_len+1];
    msg[0]='w';
    memcpy(msg+1, name, name_len);

    char tid[0];
    tid[0] = 160;
    asm volatile("mov x30, x30");
    int intended_reply_len = Send(1, msg, name_len+1, tid, 1);
    if(intended_reply_len < 0 ){
        return -1;
    }
    if(intended_reply_len == 0){
        uart_printf(CONSOLE, "\x1b[31mCannot find corresponding tid %d %d\x1b[0m\r\n", intended_reply_len, tid);
        for(;;){}
    }
    if(intended_reply_len != 1){
        uart_printf(CONSOLE, "\x1b[31mWhoIs unexpected behaviour %d %d\x1b[0m\r\n", intended_reply_len, tid);
        for(;;){}
    }
    if(tid[0]==160){
        uart_printf(CONSOLE, "\x1b[31mtid not set\x1b[0m\r\n");
        for(;;){}
    }
    return (int)tid[0];
}

// define messages to name server as r+name or w+name
void nameserver(){
    uart_dprintf(CONSOLE, "Running nameserver tid %u\r\n", MyTid());
    char tid_to_name[MAX_NUM_TASKS][MAX_TASK_NAME_CHAR+1];
    // char *tid_to_name[MAX_NUM_TASKS];
    for(int i = 0; i<MAX_NUM_TASKS; i++){
        tid_to_name[i][0] = '\0';
    }

    char msg[MAX_TASK_NAME_CHAR+2];
    int tid;
    char reply[1];
    for(;;){
        int msglen = Receive(&tid, msg, MAX_TASK_NAME_CHAR+1);
        if(msglen>MAX_TASK_NAME_CHAR+1){
            uart_printf(CONSOLE, "Supplied name exceeded maximum task name length\r\n");
            msglen = MAX_TASK_NAME_CHAR+1;
        }
        msg[msglen] = '\0';
        if(msglen<2){
            uart_printf(CONSOLE, "\x1b[31mInvalid msg received by name_server %d\x1b[0m\r\n", msglen);
            for(;;){}
        }
        if(msg[0]=='r'){
            char *arg_name = msg+1;
            int index = 0;
            // check if another task has the same name
            for(int i = 0; i<MAX_NUM_TASKS; i++){
                if(str_equal(arg_name, tid_to_name[i])){
                    uart_printf(CONSOLE, "Name overwritten for task %d\r\n", i);
                    tid_to_name[i][0] = '\0';
                }
            }
            while (arg_name[index]!='\0'){
                char c = arg_name[index];
                tid_to_name[tid][index] = c;
                index += 1;
            }
            tid_to_name[tid][index]='\0';
            Reply(tid, NULL, 0);
        }else if(msg[0]=='w'){
            char *arg_name = msg+1;
            int reply_tid = -1;
            for(int i = 0; i<MAX_NUM_TASKS; i++){
                if(str_equal(tid_to_name[i], arg_name)){
                    reply_tid = i;
                    break;
                }
            }
            if(reply_tid==-1){
                Reply(tid, NULL, 0);
            }else{
                reply[0] = reply_tid;
                Reply(tid, (const char *)reply, 1);
            }
        }else{
            uart_printf(CONSOLE, "\x1b[31mInvalid msg received by name_server\x1b[0m\r\n");
            for(;;){}
        }
        
    }
}
