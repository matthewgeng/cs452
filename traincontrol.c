#include "rpi.h"
#include "console.h"
#include "util.h"
#include "io.h"
#include "traincontrol.h"

void tr(int marklin_tid, unsigned int trainNumber, unsigned int trainSpeed, uint32_t last_speed[]){
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, trainSpeed);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, trainNumber);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);

  char cmd[3];
  cmd[0] = trainSpeed;
  cmd[1] = trainNumber;
  cmd[2] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  last_speed[trainNumber] = trainSpeed;
}
void rv(int marklin_tid, int clock, unsigned int trainNumber, uint32_t last_speed[]){

  char cmd[4];
  // TODO: can't send 0 rn
  cmd[0] = 0;
  cmd[1] = trainNumber;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);
  Delay(clock, 250);
  // cmd[2] = 254;
  cmd[0] = 15;
  cmd[1] = trainNumber;
  Delay(clock, 15);
  Puts_len(marklin_tid, MARKLIN, cmd, 2);
  cmd[0] = last_speed[trainNumber];
  cmd[1] = trainNumber;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);

  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 0);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, trainNumber);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 254);
  
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 15);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, trainNumber);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);
  
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, lastSpeed[trainNumber]);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, trainNumber);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);


}

void sw(int console_tid, int marklin_tid, unsigned int switchNumber, char switchDirection){
  char cmd[4];
  if(switchDirection=='S'){
    cmd[0] = 33;
    // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 33);
  }else if(switchDirection=='C'){
    cmd[0] = 34;
    // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 34);
  }else{
    return;
  }
  cmd[1] = switchNumber;
  cmd[2] = 32;
  cmd[3] = 0;
  Puts_len(marklin_tid, MARKLIN, cmd, 2);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, switchNumber);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 32);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);

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
//   outputBufEnd = printfToBuffer(outputBuf, outputBufEnd, OUTPUT_BUFFER_SIZE, "\033[%u;%uH", r, c);
//   outputBufEnd = charToBuffer(outputBuf, outputBufEnd, OUTPUT_BUFFER_SIZE, switchDirection);
}

void executeFunction(int console_tid, int marklin_tid, int clock, char *str, uint32_t last_speed[]){
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
      // displayFuncMessage("Invalid train number");
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
      // displayFuncMessage("Invalid train speed");
      return;
    }
    tr(marklin_tid, trainNumber, trainSpeed, last_speed);
    str_cpy_w0(func_res+10, "Train speed changed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Train speed changed");

  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
    unsigned int trainNumber;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      str_cpy_w0(func_res+10, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid train number");
      return;
    }
    rv(marklin_tid, clock, trainNumber, last_speed);
    str_cpy_w0(func_res+10, "Train reversed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Train reversed");

  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
    unsigned int switchNumber;
    uint16_t switchDirectionIndex;

    switchNumber = getArgumentThreeDigitNumber(str+3);
    if(switchNumber==1000){
      str_cpy_w0(func_res+10, "Invalid switch number");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid switch number");
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
      // displayFuncMessage("Invalid switch direction");
      return;
    }
    sw(console_tid, marklin_tid, switchNumber, switchDirection);
    str_cpy_w0(func_res+10, "Switch direction changed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Switch direction changed");

  }
  else{
    str_cpy_w0(func_res+10, "Unknown function");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Unknown function");
    return;
  }
}


void build_sensor_str(int i, char *sensors_str, int *sensors_str_index){
    char sensorBand = i/16 + 'A';
    int sensorNum = i%16 + 1;
    sensors_str[*sensors_str_index] = sensorBand;
    i2a( sensorNum, sensors_str[*sensors_str_index+1] );
    if(sensorNum<10){
        sensors_str[*sensors_str_index+2] = ' ';
        sensors_str[*sensors_str_index+3] = 0;
        *sensors_str_index = *sensors_str_index+3;
    }else{
        sensors_str[*sensors_str_index+3] = ' ';
        sensors_str[*sensors_str_index+4] = 0;
        *sensors_str_index = *sensors_str_index+4;
    }
}


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