#include "rpi.h"
#include "util.h"
#include "cb.h"



static unsigned int lastTime;
static unsigned int time;


static char* const TIMER_BASE = (char*) 0xFe003000;
static const uint32_t TIMER_CS  = 0x00;
static const uint32_t TIMER_CLO  = 0x04;
static const uint32_t TIMER_CHI  = 0x08;
static const uint32_t TIMER_C0  = 0x0c;
static const uint32_t TIMER_C1  = 0x10;
static const uint32_t TIMER_C2  = 0x14;
static const uint32_t TIMER_C3  = 0x18;

static char* const CONSOLE_UART_BASE = (char*) 0xFe201000;
static const uint32_t CONSOLE_UART_DR  = 0x00;
static const uint32_t CONSOLE_UART_FR  = 0x18;

static char outputBuf[OUTPUT_BUFFER_SIZE] = "";
static uint32_t outputBufUartNext;
static uint32_t outputBufEnd;
static char inputBuf[INPUT_BUFFER_SIZE] = "";
static uint32_t inputBufEnd;
static uint32_t inputCol;

static char trainBuf[TRAIN_BUFFER_SIZE] = "";
static uint32_t trainBufUartNext;
static uint32_t trainBufEnd;

static uint32_t matchValue;

static uint32_t lastSpeed;
static uint32_t nextMarklinCmdTime;

static int sensorBuf[SENSOR_BUFFER_SIZE];
static uint32_t sensorBufEnd;
static int sensorByteBuf[10];
static uint32_t sensorByteBufEnd;

static uint16_t sensorPollingStarted;
static uint32_t lastSensorTriggered;


void clearInputBuffer(){
  inputBufEnd = 0;
  inputBuf[0] = '\0';
}


void debug(uint32_t line, char *msg){
  uart_printf(CONSOLE, "\033[%u;1H\033[KInput: ", line);
  uart_puts(CONSOLE, msg);
  // outputBufEnd = printfToBuffer(outputBuf, outputBufEnd, "\033[%u;1H\033[KInput: ", line);
  // outputBufEnd = strToBuffer(outputBuf, outputBufEnd, msg);
}
void addSensor(int sensor){
  sensorBuf[sensorBufEnd] = sensor;
  sensorBufEnd = incrementBufEnd(sensorBufEnd, SENSOR_BUFFER_SIZE);
}

void displaySensors(){
  uint32_t startSensorIndex = incrementBufEnd(sensorBufEnd, SENSOR_BUFFER_SIZE);
  if(sensorBuf[incrementBufEnd(sensorBufEnd, SENSOR_BUFFER_SIZE)]==-1){
    startSensorIndex = SENSOR_BUFFER_SIZE-1;
  }else{
    startSensorIndex = sensorBufEnd;
  }
  uint32_t nextSensorIndex = decrementBufEnd(sensorBufEnd, SENSOR_BUFFER_SIZE);
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;22H\033[K", SENSORS_ROW);
  while(nextSensorIndex!=startSensorIndex){
    char sensorBand = sensorBuf[nextSensorIndex]/16 + 'A';
    int sensorNum = sensorBuf[nextSensorIndex]%16 + 1;
    outputBufEnd = charToBuffer(CONSOLE, outputBuf, outputBufEnd, sensorBand);
    outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "%d  ", sensorNum);
    nextSensorIndex = decrementBufEnd(nextSensorIndex, SENSOR_BUFFER_SIZE);
  }
  //move cursor back to input
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;%uH", INPUT_ROW, inputCol);
}

void timeUpdate() {

    uint32_t currentCounterValue = readRegisterAsUInt32(TIMER_BASE, TIMER_CLO);
    if (currentCounterValue>=matchValue){
      // clearOutputBuffer();

      if(time>2000000 && time%500000==0){
        sensorByteBufEnd = 0;
        trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 0x85);
        trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);
        sensorPollingStarted = 1;
      }

      lastTime = time;
      time += COUNTER_PER_TENTH_SECOND;
      // uart_printf(CONSOLE, "Time: %u\r\n", time);
      unsigned int minutes = time/COUNTER_PER_TENTH_SECOND/600;
      unsigned int seconds = (time/COUNTER_PER_TENTH_SECOND/10)%60;
      unsigned int tenthOfSecond = (time/COUNTER_PER_TENTH_SECOND)%10;

      //put time on screen
      outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[1;1H\033[K%u:%u.%u\r\n", minutes, seconds, tenthOfSecond);
      
      //move cursor back to input
      outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;%uH", INPUT_ROW, inputCol);

      uint32_t nextMatchValue = currentCounterValue + COUNTER_PER_TENTH_SECOND;
      matchValue = nextMatchValue;


    }
}

void printToConsole(){
  if(outputBufUartNext!=outputBufEnd){
    if(polling_uart_putc(CONSOLE, outputBuf[outputBufUartNext])){
      outputBufUartNext+=1;
      if(outputBufUartNext == OUTPUT_BUFFER_SIZE){
        outputBufUartNext = 0;
      }
    }
  }
  if(trainBufUartNext!=trainBufEnd){
    if ((int)trainBuf[trainBufUartNext]==255){
      nextMarklinCmdTime = readRegisterAsUInt32(TIMER_BASE, TIMER_CLO) + 20000;
      trainBufUartNext += 1;
      if(trainBufUartNext == TRAIN_BUFFER_SIZE){
        trainBufUartNext = 0;
      }
    }else if((int)trainBuf[trainBufUartNext]==254){
      nextMarklinCmdTime = readRegisterAsUInt32(TIMER_BASE, TIMER_CLO) + 2500000;
      trainBufUartNext += 1;
      if(trainBufUartNext == TRAIN_BUFFER_SIZE){
        trainBufUartNext = 0;
      }
    }
    else if(readRegisterAsUInt32(TIMER_BASE, TIMER_CLO)>=nextMarklinCmdTime){
      if(polling_uart_putc(MARKLIN, trainBuf[trainBufUartNext])){
        trainBufUartNext+=1;
        if(trainBufUartNext == TRAIN_BUFFER_SIZE){
          trainBufUartNext = 0;
        }
      }
    }
  }
}

unsigned int isInt(char c){
  if(c>='0' && c<='9'){
    return 1;
  }
  return 0;
}

unsigned int getArgumentTwoDigitNumber(char *src){
  if(isInt(src[0])&&(src[1]==' '||src[1]=='\0')){
    return a2d(src[0]);
  }else if(isInt(src[0])&&isInt(src[1])&&(src[2]==' '||src[2]=='\0')){
    return a2d(src[0])*10+a2d(src[1]);
  }
  //invalid value cuz only result should be 2 digits
  return 1000;
}
unsigned int getArgumentThreeDigitNumber(char *src){
  if(isInt(src[0])&&(src[1]==' '||src[1]=='\0')){
    return a2d(src[0]);
  }else if(isInt(src[0])&&isInt(src[1])&&(src[2]==' '||src[2]=='\0')){
    return a2d(src[0])*10+a2d(src[1]);
  }else if(isInt(src[0])&&isInt(src[1])&&isInt(src[2])&&(src[3]==' '||src[3]=='\0')){
    return a2d(src[0])*100+a2d(src[1])*10+a2d(src[2]);
  }
  //invalid value cuz only result should be 2 digits
  return 1000;
}

void displayFuncMessage(char *err){
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;1H\033[K", FUNCTION_RESULT_ROW);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, err);
}


void tr(unsigned int trainNumber, unsigned int  trainSpeed){
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, trainSpeed);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, trainNumber);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);
  lastSpeed = trainSpeed;
}
void rv(unsigned int trainNumber){
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 0);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, trainNumber);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 254);
  
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 15);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, trainNumber);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);
  
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, lastSpeed);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, trainNumber);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);

}
void sw(unsigned int switchNumber, char switchDirection){
  if(switchDirection=='S'){
    trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 33);
  }else if(switchDirection=='C'){
    trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 34);
  }else{
    return;
  }
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, switchNumber);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);

  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 32);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);

  uint32_t r,c;
  
  if(switchNumber<153){
    r = SWITCHES_ROW + 1 + switchNumber/8;
    c = (switchNumber%8-1)*9+6;
  }else{
    r = SWITCHES_ROW + 1 + (switchNumber-134)/8;
    c = ((switchNumber-134)%8-1)*9+6;
  }
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;%uH", r, c);
  outputBufEnd = charToBuffer(CONSOLE, outputBuf, outputBufEnd, switchDirection);
}

void executeFunction(char *str){
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd,"\033[%u;1H\033[K", LAST_FUNCTON_ROW);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, str);
  if(str[0]=='t' && str[1]=='r' && str[2]==' '){
    unsigned int trainNumber, trainSpeed;
    uint16_t trainSpeedStartIndex;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      displayFuncMessage("Invalid train number");
      return;
    }

    if(str[4]==' '){
      trainSpeedStartIndex=5;
    }else{
      trainSpeedStartIndex=6;
    }
    trainSpeed = getArgumentTwoDigitNumber(str+trainSpeedStartIndex);
    if(trainSpeed>30){
      displayFuncMessage("Invalid train speed");
      return;
    }
    tr(trainNumber, trainSpeed);
    displayFuncMessage("Train speed changed");

  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
    unsigned int trainNumber;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      displayFuncMessage("Invalid train number");
      return;
    }
    rv(trainNumber);
    displayFuncMessage("Train reversed");

  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
    unsigned int switchNumber;
    uint16_t switchDirectionIndex;

    switchNumber = getArgumentThreeDigitNumber(str+3);
    if(switchNumber==1000){
      displayFuncMessage("Invalid switch number");
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
      displayFuncMessage("Invalid switch direction");
      return;
    }
    sw(switchNumber, switchDirection);
    displayFuncMessage("Switch direction changed");

  }
  // else if(str[0]=='t'){
  //   //TODO: remove
  //   sensorByteBufEnd = 0;
  //   trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 0x85);
  //   trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);
  // }
  else{
    displayFuncMessage("Unknown function");
    return;
  }
}

void addSensors() {
  for(int i = 0; i<10; i++){
    char c = sensorByteBuf[i];
    for (int u = 0; u < 8; u++) {
        if (c & (1 << u)) {
            uint32_t sensorNum = i*8+7-u;
            if(sensorNum!=lastSensorTriggered){
              addSensor(sensorNum);
              lastSensorTriggered = sensorNum;
            }
            
        }
    }
  }
}

void onReceiveSensorByte(char sensorByte){
  sensorByteBuf[sensorByteBufEnd] = sensorByte;
  if(sensorByteBufEnd==9){
    // uart_printf(CONSOLE, "\033[%d;1H\033[KAll sensor bytes received.", 40);
    addSensors();
    displaySensors();
    sensorByteBufEnd=0;
  }else{
    sensorByteBufEnd+=1;
    // uart_printf(CONSOLE, "\033[%d;1H\033[KSensor byte received. %u", 41, sensorByteBufEnd);
  }
}

static void mainloop() {
  for (;;) {

    timeUpdate();
    printToConsole();

    // if(lastTime!=time){
    // if(readRegisterAsUInt32(TIMER_BASE, TIMER_CLO)%5000000==0){

    //   // uart_putc(MARKLIN, 0x85);
    //   // for(int i = 0; i<10000; i++){}
    //   lastTime=time;
    // }

    char sensorByte = polling_uart_getc(MARKLIN);
    if(sensorPollingStarted && sensorByte!=255){
      onReceiveSensorByte(sensorByte);
    }

    char c = polling_uart_getc(CONSOLE);
    if(c!=255){
      if (c == '\r') {
        if(inputBuf[0] == 'q' && inputBuf[1]=='\0') {
          return;
        }
        executeFunction(inputBuf);
        clearInputBuffer();
        // clear input line and add >
        outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;1H\033[K", INPUT_ROW);
        outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, "> ");
        inputCol = 3;
      }else{
        //add char to input buffer
        inputBufEnd = charToRegBuffer(inputBuf, inputBufEnd, c);

        //put char on screen
        outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;%uH", INPUT_ROW, inputCol);
        outputBufEnd = charToBuffer(CONSOLE, outputBuf, outputBufEnd, c);
        inputCol += 1;
      } 
    }
  }
}

void switchesSetup(){
  // 18 switches + 4 centre ones
  sw(1, 'C');
  sw(2, 'C');
  sw(3, 'C');
  sw(4, 'C');
  sw(5, 'C');
  sw(6, 'S');
  sw(7, 'S');
  sw(8, 'C');
  sw(9, 'C');
  sw(10, 'C');
  sw(11, 'C');
  sw(12, 'C');
  sw(13, 'C');
  sw(14, 'C');
  sw(15, 'C');
  sw(16, 'C');
  sw(153, 'C');
  sw(154, 'S');
  sw(155, 'S');
  sw(156, 'C');

}

int kmain() {
  // set up GPIO pins for both console and marklin uarts
  gpio_init();

  // not strictly necessary, since line 1 is configured during boot
  // but we'll configure the line anyway, so we know what state it is in
  uart_config_and_enable(CONSOLE);
  uart_config_and_enable(MARKLIN);
  uart_printf(CONSOLE, "Start of code\r\n");


  outputBufUartNext = 0;
  outputBufEnd = 0;
  trainBufUartNext = 0;
  trainBufEnd = 0;
  inputBufEnd = 0;



  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[2J");
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;1H> ", INPUT_ROW);
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;3H", INPUT_ROW);
  inputCol = 3;
  nextMarklinCmdTime = 0;

  //marklin go
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 96);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);
  //marklin sensor reset mode on
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 0xC0);
  trainBufEnd = charToBuffer(MARKLIN, trainBuf, trainBufEnd, 255);

  switchesSetup();
  char *s1 = "Switches\r\n";
  char *s2 = "001: C   002: C   003: C   004: C   005: C   006: S   007: S   008: C\r\n";
  char *s3 = "009: C   010: C   011: C   012: C   013: C   014: C   015: C   016: C\r\n";
  char *s4 = "017: C   018: C   153: C   154: S   155: S   156: C";
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;1H", SWITCHES_ROW);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, s1);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, s2);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, s3);
  outputBufEnd = strToBuffer(CONSOLE, outputBuf, outputBufEnd, s4);

  for(int i = 0; i<SENSOR_BUFFER_SIZE; i++){
    sensorBuf[i]=-1;
  }
  sensorBufEnd = 0;
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "\033[%u;1H\033[K", SENSORS_ROW);
  outputBufEnd = printfToBuffer(CONSOLE, outputBuf, outputBufEnd, "Most recent sensors: ", SENSORS_ROW);
  sensorByteBufEnd = 0;

  lastSensorTriggered = 1000;
  sensorPollingStarted = 0;


  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, 0x85);
  // trainBufEnd = charToBuffer(trainBuf, trainBufEnd, 255);

  // for(int i = 0; i<10; i++){
  //   char sensorByte = uart_getc(MARKLIN);
  //   onReceiveSensorByte(sensorByte);
  // }

  matchValue = readRegisterAsUInt32(TIMER_BASE, TIMER_CLO) + COUNTER_PER_TENTH_SECOND;
  time = 0;
  lastTime = 0;
  timerSetup();
  mainloop();

  // exit to boot loader
  return 0;
}
