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
  cmd[2] = 255;
  Puts(marklin_tid, MARKLIN, cmd);

  last_speed[trainNumber] = trainSpeed;
}
void rv(int marklin_tid, unsigned int trainNumber, uint32_t last_speed[]){

  char cmd[9];
  cmd[0] = 0;
  cmd[1] = trainNumber;
  cmd[2] = 254;
  cmd[3] = 15;
  cmd[4] = trainNumber;
  cmd[5] = 255;
  cmd[6] = last_speed[trainNumber];
  cmd[7] = trainNumber;
  cmd[8] = 255;
  Puts(marklin_tid, MARKLIN, cmd);

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
  if(switchDirection=='S'){
    Putc(marklin_tid, MARKLIN, 33);
    // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 33);
  }else if(switchDirection=='C'){
    Putc(marklin_tid, MARKLIN, 34);
    // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 34);
  }else{
    return;
  }
  char cmd[4];
  cmd[0] = switchNumber;
  cmd[1] = 255;
  cmd[2] = 32;
  cmd[3] = 255;
  Puts(marklin_tid, MARKLIN, cmd);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, switchNumber);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 32);
//   trainBufEnd = charToBuffer(trainBuf, trainBufEnd, TRAIN_BUFFER_SIZE, 255);

  uint32_t r,c;
  
  if(switchNumber<153){
    r = SWITCHES_ROW + 1 + switchNumber/8;
    c = (switchNumber%8-1)*9+6;
  }else{
    r = SWITCHES_ROW + 1 + (switchNumber-134)/8;
    c = ((switchNumber-134)%8-1)*9+6;
  }
  char str[] = "\033[0;0H ";
  str[2] = r;
  str[4] = c;
  str[6] = switchDirection;
  Puts(console_tid, CONSOLE, str);
//   outputBufEnd = printfToBuffer(outputBuf, outputBufEnd, OUTPUT_BUFFER_SIZE, "\033[%u;%uH", r, c);
//   outputBufEnd = charToBuffer(outputBuf, outputBufEnd, OUTPUT_BUFFER_SIZE, switchDirection);
}

void executeFunction(int console_tid, int marklin_tid, char *str, uint32_t last_speed[]){
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
    str_cpy(func_res+10, "Train speed changed");
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
    rv(marklin_tid, trainNumber, last_speed);
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
        *sensors_str_index = *sensors_str_index+3;
    }else{
        sensors_str[*sensors_str_index+3] = ' ';
        *sensors_str_index = *sensors_str_index+4;
    }
}


void switchesSetup(int console_tid, int marklin_tid){
  // 18 switches + 4 centre ones
  sw(1, 'C', console_tid, marklin_tid);
  sw(2, 'C', console_tid, marklin_tid);
  sw(3, 'C', console_tid, marklin_tid);
  sw(4, 'C', console_tid, marklin_tid);
  sw(5, 'C', console_tid, marklin_tid);
  sw(6, 'S', console_tid, marklin_tid);
  sw(7, 'S', console_tid, marklin_tid);
  sw(8, 'C', console_tid, marklin_tid);
  sw(9, 'C', console_tid, marklin_tid);
  sw(10, 'C', console_tid, marklin_tid);
  sw(11, 'C', console_tid, marklin_tid);
  sw(12, 'C', console_tid, marklin_tid);
  sw(13, 'C', console_tid, marklin_tid);
  sw(14, 'C', console_tid, marklin_tid);
  sw(15, 'C', console_tid, marklin_tid);
  sw(16, 'C', console_tid, marklin_tid);
  sw(153, 'C', console_tid, marklin_tid);
  sw(154, 'S', console_tid, marklin_tid);
  sw(155, 'S', console_tid, marklin_tid);
  sw(156, 'C', console_tid, marklin_tid);

}