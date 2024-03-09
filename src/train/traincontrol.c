#include "rpi.h"
#include "util.h"
#include "io.h"
#include "traincontrol.h"
#include "pathfinding.h"
#include "switches.h"

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]){
  char cmd[3];
  cmd[0] = trainSpeed;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  last_speed[trainNumber] = trainSpeed;
}

void reverse(){

    RegisterAs("reverse\0");
    int marklin_tid = WhoIs("mio\0");
    int clock = WhoIs("clock\0");
    int cout = WhoIs("cout\0");

    int tid;
    char msg[2];
    int train_number, last_speed;
    int msg_len;
    char cmd[4];
    for(;;){
        msg_len = Receive(&tid, msg, 2);
        Reply(tid, NULL, 0);
        if(msg_len!=2){
            #if DEBUG
                uart_dprintf(CONSOLE, "rv received incompatible msg %d\r\n", msg_len);
            #endif
        }
        train_number = (int)msg[0];
        last_speed = (int)msg[1];

        cmd[0] = 0;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);
        if(last_speed<=10){
          Delay(clock, 500);
        }else{
          Delay(clock, 600);
        }
        cmd[0] = 15;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);
        cmd[0] = last_speed;
        cmd[1] = train_number;
        Puts_len(marklin_tid, MARKLIN, cmd, 2);
    }

}

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

void execute_tr(char *str, char *func_res, int console_tid, int marklin_tid, uint32_t last_speed[]){

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
    tr(marklin_tid, trainNumber, trainSpeed, last_speed);
    str_cpy_w0(func_res+10, "Train speed changed");
    Puts(console_tid, CONSOLE, func_res);

}

void execute_rv(char *str, char *func_res, int console_tid, int marklin_tid, int reverse_tid, uint32_t last_speed[]){
  
    unsigned int trainNumber;
    char arg[2];

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid train number");
      return;
    }
    arg[0] = (char)trainNumber;
    arg[1] = (char)(last_speed[trainNumber]);
    int reply_len = Send(reverse_tid, arg, 2, NULL, 0);
    if(reply_len!=0){
      #if DEBUG
          uart_dprintf(CONSOLE, "rv replied incompatible msg %d\r\n", reply_len);
      #endif
    }
    str_cpy_w0(func_res+10, "Train reversed");
    Puts(console_tid, CONSOLE, func_res);

}

void execute_sw(char *str, char *func_res, int console_tid, int switch_tid, uint32_t last_speed[]){

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
    SwitchChange sc;
    sc.switch_num = switchNumber;
    sc.dir = switchDirection;
    int reply_len = Send(switch_tid, &sc, sizeof(SwitchChange), NULL, 0);
    if(reply_len!=0){
      #if DEBUG
          uart_dprintf(CONSOLE, "pathfind replied incompatible msg %d\r\n", reply_len);
      #endif
    }
    str_cpy_w0(func_res+10, "Switch direction changed");
    Puts(console_tid, CONSOLE, func_res);

}

void execute_pf(char *str, char *func_res, int console_tid, int pathfind_tid, uint32_t last_speed[]){

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

    PathMessage pm;
    pm.src = src;
    pm.dest = dest;

    int reply_len = Send(pathfind_tid, &pm, sizeof(pm), NULL, 0);
    if(reply_len!=0){
      #if DEBUG
          uart_dprintf(CONSOLE, "pathfind replied incompatible msg %d\r\n", reply_len);
      #endif
    }
    str_cpy_w0(func_res+10, "Path Find Ran");
    Puts(console_tid, CONSOLE, func_res);
}

void executeFunction(int console_tid, int marklin_tid, int reverse_tid, int switch_tid, int pathfind_tid, char *str, uint32_t last_speed[]){  
  char last_fun[30];
  str_cpy(last_fun, "\033[11;1H\033[K");
  str_cpy_w0(last_fun+10, str);
  Puts(console_tid, CONSOLE, last_fun);

  char func_res[30];
  str_cpy(func_res, "\033[12;1H\033[K");

  if(str[0]=='t' && str[1]=='r' && str[2]==' '){
    execute_tr(str, func_res, console_tid, marklin_tid, last_speed);
  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
    execute_rv(str, func_res, console_tid, marklin_tid, reverse_tid, last_speed);
  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
    execute_sw(str, func_res, console_tid, switch_tid, last_speed);
  }else if(str[0]=='p' && str[1]=='f' && str[2]==' '){
    execute_pf(str, func_res, console_tid, pathfind_tid, last_speed);
  }else{
    str_cpy_w0(func_res+10, "Unknown function");
    Puts(console_tid, CONSOLE, func_res);
    return;
  }
}


// void build_sensor_str(int i, char *sensors_str, int *sensors_str_index){
//     char sensorBand = i/16 + 'A';
//     int sensorNum = i%16 + 1;
//     sensors_str[*sensors_str_index] = sensorBand;
//     ui2a( sensorNum, 10, sensors_str+*sensors_str_index+1 );
//     uart_printf(CONSOLE, "\033[40;4Hsensor %d %d\r\n", i, sensorNum);
//     if(sensorNum<10){
//         sensors_str[*sensors_str_index+2] = ' ';
//         *sensors_str_index = *sensors_str_index+3;
//     }else{
//         sensors_str[*sensors_str_index+3] = ' ';
//         *sensors_str_index = *sensors_str_index+4;
//     }
// }

