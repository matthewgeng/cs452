#include "rpi.h"
#include "util.h"
#include "io.h"
#include "traincontrol.h"
#include "pathfinding.h"
#include "switches.h"
#include "trainserver.h"

int get_sensor_num(char *str){
  char sensor_letter;
  int sensor_num;
  int res;
  sensor_letter = str[0];
  if(sensor_letter<'A' || sensor_letter>'E'){
    return -1;
  }
  res += (sensor_letter-'A')*16;
  sensor_num = getArgumentTwoDigitNumber(str+1);
  if(sensor_num<0 || sensor_num>16){
    return -2;
  }
  res += sensor_num-1;
  return res;
}

uint8_t is_valid_speed(int train_num, uint32_t train_speed){
    if (train_speed >= 31) {
        return 0;
    }

    if (train_speed >= 16) {
        train_speed -=16;
    }

    if (train_speed < 0) {
        return 0;
    } else if (train_speed > 0 && train_speed < train_min_speed(train_num)) {
        return 0;
    }

    return 1;
}

// TODO: validate train numbers are supported
void execute_tr(char *str, char *func_res, int console_tid, int train_server_tid){

    unsigned int trainNumber, trainSpeed;
    uint16_t trainSpeedStartIndex;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){

      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

    if(str[4]==' '){
      trainSpeedStartIndex=5;
    }else{
      trainSpeedStartIndex=6;
    }
    trainSpeed = getArgumentTwoDigitNumber(str+trainSpeedStartIndex);
    if(trainSpeed>30){
      str_cpy_w0(func_res+10, "Invalid train speed\0");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

    if(is_valid_speed(trainNumber,trainSpeed)==0){
      str_cpy_w0(func_res+10, "train speed too low\0");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_TR;
    tsm.arg1 = trainNumber;
    tsm.arg2 = trainSpeed;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      #if DEBUG
          uart_dprintf(CONSOLE, "rv replied incompatible msg %d\r\n", reply_len);
      #endif
    }
    str_cpy_w0(func_res+10, "Train speed changed");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_rv(char *str, char *func_res, int console_tid, int train_server_tid){
  
    unsigned int trainNumber;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid train number");
      return;
    }
    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_RV;
    tsm.arg1 = trainNumber;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "rv invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

}

void execute_sw(char *str, char *func_res, int console_tid, int train_server_tid){

    unsigned int switchNumber;
    uint16_t switchDirectionIndex;

    switchNumber = getArgumentThreeDigitNumber(str+3);
    if(switchNumber==1000){
      str_cpy_w0(func_res+10, "Invalid switch number");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    if(str[4]==' '){
      switchDirectionIndex=5;
    }else if(str[5]==' '){
      switchDirectionIndex=6;
    }else{
      switchDirectionIndex=7;
    }
    char switchDirection;
    if(str[switchDirectionIndex]=='S' || str[switchDirectionIndex]=='C'){
      switchDirection = str[switchDirectionIndex];
    }else{
      str_cpy_w0(func_res+10, "Invalid switch direction");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_SW;
    tsm.arg1 = switchNumber;
    tsm.arg2 = (uint32_t)switchDirection;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "sw invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Switch direction changed");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_pf(char *str, char *func_res, int console_tid, int train_server_tid){

    int src, dest;
    str += 3;
    src = get_sensor_num(str);
    if(src==-1){
      str_cpy_w0(func_res+10, "Invalid sensor char 1");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }else if(src==-2){
      str_cpy_w0(func_res+10, "Invalid sensor num 1");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    if(str[2]==' '){
      str += 3;
    }else if(str[3]==' '){
      str += 4;
    }else{
      str_cpy_w0(func_res+10, "Invalid pathfinding command");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

    dest = get_sensor_num(str);
    if(dest==-1){
      str_cpy_w0(func_res+10, "Invalid sensor char 2");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }else if(dest==-2){
      str_cpy_w0(func_res+10, "Invalid sensor num 2");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_PF;
    tsm.arg1 = src;
    tsm.arg2 = dest;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "pf invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Path Find Ran");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_nav(char *str, char *func_res, int console_tid, int train_server_tid){

    // int clock = WhoIs("clock");
    uint8_t train_number = getArgumentTwoDigitNumber(str+4);
    if(train_number==1000){
      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    int dest;
    if(str[5]==' '){
      str += 6;
    }else if(str[6]==' '){
      str += 7;
    }
    dest = get_sensor_num(str);
    if(dest==-1){
      str_cpy_w0(func_res+10, "Invalid sensor char");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }else if(dest==-2){
      str_cpy_w0(func_res+10, "Invalid sensor num");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

    uint32_t offset = 0;

    if(str[2]!='\0' && str[3]!='\0'){
      
      if(str[2]==' '){
        str += 3;
      }else if(str[3] == ' '){
        str += 4;
      }else{
        str_cpy_w0(func_res+10, "Invalid offset");
        Puts(console_tid, CONSOLE, func_res);
        return;
      }
      offset = getArgumentThreeDigitNumber(str);
      if(offset==1000){
        str_cpy_w0(func_res+10, "Invalid offset");
        Puts(console_tid, CONSOLE, func_res);
        return;
      }
    }

    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_NAV;
    tsm.arg1 = train_number;
    tsm.arg2 = dest;
    tsm.arg3 = offset;
    
    // uart_printf(CONSOLE, "\0337\033[53;1H\033[Ktrain control before send %d\0338", Time(clock));
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "pf invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Train navigation Ran");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_go(char *str, char *func_res, int console_tid, int train_server_tid){

    uint8_t train_number = getArgumentTwoDigitNumber(str+3);
    if(train_number==1000){
      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    if(str[4]==' '){
      str += 5;
    }else if(str[5]==' '){
      str += 6;
    }

    uint8_t train_speed = getArgumentTwoDigitNumber(str);
    if(train_speed<1 || train_speed>30 || train_speed==15){
      str_cpy_w0(func_res+10, "Invalid train speed");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    if(is_valid_speed(train_number,train_speed)==0){
      str_cpy_w0(func_res+10, "Unsupported train speed\0");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    if(str[1]==' '){
      str += 2;
    }else if(str[2]==' '){
      str += 3;
    }

    int dest = get_sensor_num(str);
    if(dest==-1){
      str_cpy_w0(func_res+10, "Invalid sensor char");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }else if(dest==-2){
      str_cpy_w0(func_res+10, "Invalid sensor num");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_GO;
    tsm.arg1 = train_number;
    tsm.arg2 = dest;
    tsm.arg3 = train_speed;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "go invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Train Go Ran");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_track(char *str, char *func_res, int console_tid, int train_server_tid){

    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_TRACK_CHANGE;

    if(str[6]=='a'){
      tsm.arg1 = 'a';
    }else if(str[6]=='b'){
      tsm.arg1 = 'b';
    }else{
      str_cpy_w0(func_res+10, "track change invalid track");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }

    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "track change invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Track Change Ran");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_switch_reset(char *str, char *func_res, int console_tid, int train_server_tid){

    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_SWITCH_RESET;
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "switch reset invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Switches reset");
    Puts(console_tid, CONSOLE, func_res);
}

void execute_set_sensor(char *str, char *func_res, int console_tid, int train_server_tid){

    int clock = WhoIs("clock");
    
    uint8_t sensor = getArgumentTwoDigitNumber(str+3);
    if(sensor==1000){
      str_cpy_w0(func_res+10, "Invalid sensor");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    // if(sensor<0 || sensor >79){
    //   str_cpy_w0(func_res+10, "Invalid sensor number");
    //   Puts(console_tid, CONSOLE, func_res);
    //   return;
    // }
    uint8_t second_sensor = 255;

    // first num was a single digit
    if(str[4] == ' ' && str[5]!='\0'){
      second_sensor = getArgumentTwoDigitNumber(str + 5);
    // first num was a double digit
    } else if(str[5] == ' ' && str[6]!='\0'){
      second_sensor = getArgumentTwoDigitNumber(str + 6);
    }

    TrainServerMsgSimple tsm;
    tsm.type = TRAIN_SERVER_NEW_SENSOR;
    tsm.arg1 = sensor;
    tsm.arg2 = second_sensor;
    tsm.arg3 = Time(clock);
    int reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsgSimple), NULL, 0);
    if(reply_len!=0){
      str_cpy_w0(func_res+10, "switch reset invalid rpllen");
      Puts(console_tid, CONSOLE, func_res);
      return;
    }
    str_cpy_w0(func_res+10, "Senser set");
    Puts(console_tid, CONSOLE, func_res);
}

void executeFunction(int console_tid, int train_server_tid, char *str){  
  char last_fun[30];
  str_cpy(last_fun, "\033[14;1H\033[K");
  str_cpy_w0(last_fun+10, str);
  Puts(console_tid, CONSOLE, last_fun);

  char func_res[50];
  str_cpy(func_res, "\033[15;1H\033[K");

  if(str[0]=='t' && str[1]=='r' && str[2]==' '){
    execute_tr(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
    execute_rv(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
    execute_sw(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='p' && str[1]=='f' && str[2]==' '){
    execute_pf(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='n' && str[1]=='a' && str[2]=='v' && str[3]==' '){
    execute_nav(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='g' && str[1]=='o' && str[2]==' '){
    execute_go(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='t' && str[1]=='r' && str[2]=='a' && str[3]=='c' && str[4]=='k' && str[5]==' '){
    execute_track(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='r' && str[1]=='e' && str[2]=='s' && str[3]=='e' && str[4]=='t'){
    execute_switch_reset(str, func_res, console_tid, train_server_tid);
  }else if(str[0]=='s' && str[1]=='s' && str[2]==' '){
    execute_set_sensor(str, func_res, console_tid, train_server_tid);
  }else{
    str_cpy_w0(func_res+10, "Unknown function");
    Puts(console_tid, CONSOLE, func_res);
    return;
  }
}

