#include "nameserver.h"
#include "taskframe.h"
#include "rpi.h"
#include "util.h"
#include "syscall.h"

int RegisterAs(const char *name){
    // assuming name_server tid is always 1
    #if DEBUG
        uart_dprintf(CONSOLE, "RegisterAs \r\n");
    #endif 
    NameServerMsg nsm;
    nsm.type = 'R';
    int name_len = str_len(name);
    if(name_len>MAX_TASK_NAME_CHAR){
        uart_printf(CONSOLE, "Registering name exceeded maximum task name length %s\r\n", name);
        memcpy(nsm.name, name, MAX_TASK_NAME_CHAR);
        nsm.name[MAX_TASK_NAME_CHAR] = '\0';
    }else{
        memcpy(nsm.name, name, name_len);
        nsm.name[name_len] = '\0';
    }
    
    int intended_reply_len = Send(NAME_SERVER_TID, &nsm, sizeof(NameServerMsg), NULL, 0);
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
    #if DEBUG
        uart_dprintf(CONSOLE, "WhoIs \r\n");
    #endif 
    NameServerMsg nsm;
    nsm.type = 'W';
    int name_len = str_len(name);
    if(name_len>MAX_TASK_NAME_CHAR){
        uart_printf(CONSOLE, "WhoIs querying name exceeded maximum task name length %s\r\n", name);
        memcpy(nsm.name, name, MAX_TASK_NAME_CHAR);
        nsm.name[MAX_TASK_NAME_CHAR] = '\0';
    }else{
        memcpy(nsm.name, name, name_len);
        nsm.name[name_len] = '\0';
    }

    char tid = 160;
    // asm volatile("mov x30, x30");
    int intended_reply_len = Send(NAME_SERVER_TID, &nsm, sizeof(NameServerMsg), &tid, 1);
    if(intended_reply_len < 0 ){
        return -1;
    }
    if(intended_reply_len == 0){
        uart_printf(CONSOLE, "\x1b[31mCannot find corresponding tid %s %d\x1b[0m\r\n", name, tid);
        for(;;){}
    }
    if(intended_reply_len != 1){
        uart_printf(CONSOLE, "\x1b[31mWhoIs unexpected behaviour %d %d\x1b[0m\r\n", intended_reply_len, tid);
        for(;;){}
    }
    if(tid==160){
        intended_reply_len = Send(NAME_SERVER_TID, &nsm, sizeof(NameServerMsg), &tid, 1);
        if(intended_reply_len < 0 ){
            return -1;
        }
        if(intended_reply_len == 0){
            uart_printf(CONSOLE, "\x1b[31mCannot find corresponding tid %s %d\x1b[0m\r\n", name, tid);
            for(;;){}
        }
        if(intended_reply_len != 1){
            uart_printf(CONSOLE, "\x1b[31mWhoIs unexpected behaviour %d %d\x1b[0m\r\n", intended_reply_len, tid);
            for(;;){}
        }
        if(tid==160){
            uart_printf(CONSOLE, "\x1b[31mtid not set %s\x1b[0m\r\n", name);
            for(;;){}
        }
    }
    return (int)tid;
}

void set_name(NameMap *map, int tid, char *name){
    int index = 0;
    while (name[index]!='\0'){
        char c = name[index];
        map->mapping[tid][index] = c;
        index += 1;
    }
    map->mapping[tid][index]='\0';
}

char* get_name(NameMap *map, int tid){
    return map->mapping[tid];
}

// define messages to name server as r+name or w+name
void nameserver(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Running nameserver \r\n");
    #endif 
    NameMap tid_to_name;
    NameMap blocked_whois;

    for(int i = 0; i<MAX_NUM_TASKS; i++){
        set_name(&tid_to_name, i, "\0");
        set_name(&blocked_whois, i, "\0");
    }

    NameServerMsg nsm;
    // char msg[MAX_TASK_NAME_CHAR+2];
    int tid;
    char reply[MAX_NUM_TASKS][1];
    for(;;){
        int msglen = Receive(&tid, &nsm, sizeof(NameServerMsg));
        if(nsm.type=='R'){
            char *arg_name = nsm.name;
            // check if another task has the same name
            for(int i = 0; i<MAX_NUM_TASKS; i++){
                if(str_equal(arg_name, get_name(&tid_to_name, i))){
                    uart_printf(CONSOLE, "Name overwritten for task %d\r\n", i);
                    set_name(&tid_to_name, i, "\0");
                }
            }
            set_name(&tid_to_name, tid, arg_name);
            Reply(tid, NULL, 0);
            for(int i = 0; i<MAX_NUM_TASKS; i++){
                if(str_equal(arg_name, get_name(&blocked_whois, i))){
                    reply[i][0] = tid;
                    Reply(i, (const char *)(reply[i]), 1);
                }
            }
        }else if(nsm.type=='W'){
            char *arg_name = nsm.name;
            int reply_tid = -1;
            for(int i = 0; i<MAX_NUM_TASKS; i++){
                if(str_equal(get_name(&tid_to_name, i), arg_name)){
                    reply_tid = i;
                    break;
                }
            }
            if(reply_tid==-1){
                memcpy(get_name(&blocked_whois, tid), arg_name, msglen);
            }else{
                reply[tid][0] = reply_tid;
                Reply(tid, (const char *)reply[tid], 1);
            }
        }else{
            uart_printf(CONSOLE, "\x1b[31mInvalid msg received by name_server\x1b[0m\r\n");
            for(;;){}
        }
        
    }
}
