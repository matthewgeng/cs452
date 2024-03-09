#include "switches.h"
#include "rpi.h"
#include "constants.h"

void sw(int cout, int mio, unsigned int switchNumber, char switchDirection){
  // TODO: should only send if it's different from current
  char cmd[4];
  if(switchDirection=='S'){
    cmd[0] = 33;
  }else if(switchDirection=='C'){
    cmd[0] = 34;
  }else{
    return;
  }
  cmd[1] = switchNumber;
  cmd[2] = 32;
  cmd[3] = 0;
  Puts_len(mio, MARKLIN, cmd, 2);

  uint32_t r,c;
  
  if(switchNumber<153){
    r = SWITCHES_ROW + 1 + (switchNumber-1)/8;
    c = ((switchNumber-1)%8)*9+6;
  }else{
    r = SWITCHES_ROW + 1 + (switchNumber-134)/8;
    c = ((switchNumber-134)%8-1)*9+6;
  }
  char str[] = "\0337\033[ ;       ";
  ui2a_no0(r, 10, str+4);
  if(c<10){
    ui2a_no0(c, 10, str+6);
    str[7] = 'H';
    str[8] = switchDirection;
    str[9] = '\033';
    str[10] = '8';
    str[11] = '\0';
  }else{
    ui2a_no0(c, 10, str+6);
    str[8] = 'H';
    str[9] = switchDirection;
    str[10] = '\033';
    str[11] = '8';
    str[12] = '\0';
  }
  Puts(cout, CONSOLE, str);
}


void switches_setup(int cout, int mio, char switch_states[]){
  // 18 switches + 4 centre ones
  int nums[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,153,154,155,156};
  char dirs[] = "CCCCCSSCCCCCCCCCCCCSSC";
  for(int i = 0; i<22; i++){
    sw(cout, mio, nums[i], dirs[i]);
    switch_states[i] = dirs[i];
  }
}


void switches_server(){
  RegisterAs("switch\0");
  int mio = WhoIs("mio\0");
  int clock = WhoIs("clock\0");
  int cout = WhoIs("cout\0");

  //TODO: can optimize by making it bitwise 2^22
  char switch_states[22];
  switches_setup(cout, mio, switch_states);

  int tid;
  SwitchChange switch_changes[22];
  int cur_switch_num;
  int cur_switch_index;
  char cur_switch_dir;
  int msg_len;
  char cmd[4];
  for(;;){
    msg_len = Receive(&tid, switch_changes, sizeof(SwitchChange)*22);
    if(msg_len==1 && ((char *)switch_changes)[0]=='s'){
        // switch state query
        Reply(tid, switch_states, sizeof(uint8_t)*22);
    }else{
        Reply(tid, NULL, 0);
        if(msg_len%sizeof(SwitchChange)!=0){
          uart_printf(CONSOLE, "\0337\033[30;1H\033[Kset switches unexpected msg len: %d\0338", msg_len);
        }
        for(int i = 0; i<msg_len/sizeof(SwitchChange); i++){
          cur_switch_num = switch_changes[i].switch_num;
          cur_switch_index = cur_switch_num-1;
          cur_switch_dir = switch_changes[i].dir;
          if(cur_switch_num>=153) cur_switch_index -= 134;
          if(switch_states[cur_switch_index] != cur_switch_dir){
            switch_states[cur_switch_index] = cur_switch_dir;
            sw(cout, mio, cur_switch_num, cur_switch_dir);
          }
        }
    }
  }
}
