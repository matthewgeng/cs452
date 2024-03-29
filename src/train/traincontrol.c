#include "rpi.h"
#include "console.h"
#include "util.h"
#include "io.h"
#include "traincontrol.h"

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

void sw(int console_tid, int marklin_tid, unsigned int switchNumber, char switchDirection){
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
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

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
  Puts(console_tid, CONSOLE, str);
}

void executeFunction(int console_tid, int marklin_tid, int reverse_tid, int clock, char *str, uint32_t last_speed[]){
  char last_fun[30];
  str_cpy(last_fun, "\033[11;1H\033[K");
  str_cpy_w0(last_fun+10, str);
  Puts(console_tid, CONSOLE, last_fun);

  char func_res[30];
  str_cpy(func_res, "\033[12;1H\033[K");

  if(str[0]=='t' && str[1]=='r' && str[2]==' '){
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

  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
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

  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
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
    sw(console_tid, marklin_tid, switchNumber, switchDirection);
    str_cpy_w0(func_res+10, "Switch direction changed");
    Puts(console_tid, CONSOLE, func_res);

  }
  else{
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


void switchesSetup(int console_tid, int marklin_tid){
  // 18 switches + 4 centre ones
  sw(console_tid, marklin_tid, 1, 'C');
  sw(console_tid, marklin_tid, 2, 'C');
  sw(console_tid, marklin_tid, 3, 'C');
  sw(console_tid, marklin_tid, 4, 'C');
  sw(console_tid, marklin_tid, 5, 'C');
  sw(console_tid, marklin_tid, 6, 'S');
  sw(console_tid, marklin_tid, 7, 'S');
  sw(console_tid, marklin_tid, 8, 'C');
  sw(console_tid, marklin_tid, 9, 'C');
  sw(console_tid, marklin_tid, 10, 'C');
  sw(console_tid, marklin_tid, 11, 'C');
  sw(console_tid, marklin_tid, 12, 'C');
  sw(console_tid, marklin_tid, 13, 'C');
  sw(console_tid, marklin_tid, 14, 'C');
  sw(console_tid, marklin_tid, 15, 'C');
  sw(console_tid, marklin_tid, 16, 'C');
  sw(console_tid, marklin_tid, 153, 'C');
  sw(console_tid, marklin_tid, 154, 'S');
  sw(console_tid, marklin_tid, 155, 'S');
  sw(console_tid, marklin_tid, 156, 'C');

}
