#include "tasks.h"
#include "rpi.h"
#include "nameserver.h"
#include "clock.h"
#include "syscall.h"
#include "taskframe.h"
#include "timer.h"
#include "io.h"
#include "console.h"
#include "gameclient.h"
#include "gameserver.h"
#include "util.h"


void k2test() {
    uart_printf(CONSOLE, "Beginning testing\r\n");

    uart_printf(CONSOLE, "Test 1\r\n");
    Create(1, &test1);
    uart_printf(CONSOLE, "Test 1 finished\r\n");

    uart_printf(CONSOLE, "Press any key to continue\r\n");
    char c = uart_getc(CONSOLE);

    uart_printf(CONSOLE, "Test 2\r\n");
    Create(1, &test2);
    uart_printf(CONSOLE, "Test 2 finished\r\n");

    uart_printf(CONSOLE, "Press any key to continue\r\n");
    c = uart_getc(CONSOLE);
    
    uart_printf(CONSOLE, "Test 3\r\n");
    Create(1, &test3);
    uart_printf(CONSOLE, "Test 3 finished\r\n");

    uart_printf(CONSOLE, "Finished testing\r\n");
}

void k2(){
    uart_dprintf(CONSOLE, "Root Task\r\n");
    Create(1, &nameserver);
    Create(2, &gameserver);
    Create(100, &k2test);
    
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}

void time_user() {
    int tid = MyTid();
    int parent = MyParentTid();
    int clock = WhoIs("clock"); // TODO: make every server have a function to get name

    char data[2];
    Send(parent, NULL, 0, data, sizeof(data));
    char delay = data[0];
    char num_delay = data[1];

    for (int i = 0; i < num_delay; i++) {
        Delay(clock, delay);
        uart_printf(CONSOLE, "tid: %u, delay: %u, completed: %u\r\n", tid, delay, i+1);
    }
}

void k3(){
    int user1 = Create(3, &time_user);
    int user2 = Create(4, &time_user);
    int user3 = Create(5, &time_user);
    int user4 = Create(6, &time_user);

    Receive(NULL, NULL, 0);
    char user1_data[2] = {10, 20};
    Reply(user1, user1_data, sizeof(user1_data));

    Receive(NULL, NULL, 0);
    char user2_data[2] = {23, 9};
    Reply(user2, user2_data, sizeof(user2_data));

    Receive(NULL, NULL, 0);
    char user3_data[2] = {33, 6};
    Reply(user3, user3_data, sizeof(user3_data));

    Receive(NULL, NULL, 0);
    char user4_data[2] = {71, 3};
    Reply(user4, user4_data, sizeof(user4_data));
}


void idle_task(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Idle Task\r\n");
    #endif 
    for(;;){
        uint32_t idle_time_percent = (*p_idle_task_total*100)/(sys_time() - *p_program_start);
        uart_printf(CONSOLE, "\0337\033[3;1HIdle percentage: %u%% \0338", idle_time_percent);
        // uart_printf(CONSOLE, "Idle percentage: %u%%\r\n", idle_time_percent);
        asm volatile("wfi");
    }
}
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
  str_cpy(last_fun, "\033[0;1H\033[K");
  last_fun[2] = LAST_FUNCTON_ROW;
  str_cpy(last_fun+9, str);
  Puts(console_tid, CONSOLE, last_fun);

  char func_res[30];
  str_cpy(func_res, "\033[%u;1H\033[K");
  func_res[2] = FUNCTION_RESULT_ROW;

  if(str[0]=='t' && str[1]=='r' && str[2]==' '){
    unsigned int trainNumber, trainSpeed;
    uint16_t trainSpeedStartIndex;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){

      str_cpy(func_res+9, "Invalid train number");
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
      str_cpy(func_res+9, "Invalid train speed");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid train speed");
      return;
    }
    tr(marklin_tid, trainNumber, trainSpeed, last_speed);
    str_cpy(func_res+9, "Train speed changed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Train speed changed");

  }else if(str[0]=='r' && str[1]=='v' && str[2]==' '){
    unsigned int trainNumber;

    trainNumber = getArgumentTwoDigitNumber(str+3);
    if(trainNumber==1000){
      str_cpy(func_res+9, "Invalid train number");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid train number");
      return;
    }
    rv(marklin_tid, trainNumber, last_speed);
    str_cpy(func_res+9, "Train reversed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Train reversed");

  }else if(str[0]=='s' && str[1]=='w' && str[2]==' '){
    unsigned int switchNumber;
    uint16_t switchDirectionIndex;

    switchNumber = getArgumentThreeDigitNumber(str+3);
    if(switchNumber==1000){
      str_cpy(func_res+9, "Invalid switch number");
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
      str_cpy(func_res+9, "Invalid switch direction");
      Puts(console_tid, CONSOLE, func_res);
      // displayFuncMessage("Invalid switch direction");
      return;
    }
    sw(console_tid, marklin_tid, switchNumber, switchDirection);
    str_cpy(func_res+9, "Switch direction changed");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Switch direction changed");

  }
  else{
    str_cpy(func_res+9, "Unknown function");
    Puts(console_tid, CONSOLE, func_res);
    // displayFuncMessage("Unknown function");
    return;
  }
}

void console_time(){
    int clock_tid = WhoIs("clock");
    int console_tid = WhoIs("cout");
    char time_str[20] = "\033[1;1H\033[K  :  .  \r\n";
    char bf[2];
    int time = Time(clock_tid);
    for(;;){
        unsigned int minutes = time/COUNTER_PER_TENTH_SECOND/600;
        unsigned int seconds = (time/COUNTER_PER_TENTH_SECOND/10)%60;
        unsigned int tenthOfSecond = (time/COUNTER_PER_TENTH_SECOND)%10;
        ui2a( minutes, 10, bf );
        time_str[10] = bf[0];
        time_str[11] = bf[1];
        ui2a( seconds, 10, bf );
        time_str[13] = bf[0];
        time_str[14] = bf[1];
        ui2a( tenthOfSecond, 10, bf );
        time_str[16] = bf[0];
        time_str[17] = bf[1];
        Puts(console_tid, CONSOLE, time_str);
        DelayUntil(clock_tid, time+COUNTER_PER_TENTH_SECOND);        
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

void sensor_update(){
  int clock_tid = WhoIs("clock");
  int console_tid = WhoIs("cout");
  int marklin_tid = WhoIs("mio");
  int time = Time(clock_tid);;
  int last_sensor_triggered = 1000;
  char sensor_byte;

  int sensors[12];
  IntCB sensor_cb;
  initialize_intcb(&sensor_cb, sensors, 12, 1);

  int sensors_changed = 0;
  char sensors_str[48];
  int sensors_str_index;
  for(;;){
    Putc(marklin_tid, MARKLIN, 0x85);
    Putc(marklin_tid, MARKLIN, 255);
    for(int i = 0; i<10; i++){
      sensor_byte = Getc(marklin_tid, MARKLIN);
      for (int u = 0; u < 8; u++) {
        if (sensor_byte & (1 << u)) {
          int sensorNum = i*8+7-u;
          if(sensorNum!=last_sensor_triggered){
            push_intcb(&sensor_cb, sensorNum);
            last_sensor_triggered = sensorNum;
            sensors_changed = 1;
          }
        }
      }
    }
    if(sensors_changed){
        sensors_str_index = 0;
        iter_elements_backwards(&sensor_cb, build_sensor_str, sensors_str, &sensors_str_index);
    }
    time += 500000;
    DelayUntil(clock_tid, time);
  }
}

void user_input(){
    int console_tid = WhoIs("cout");
    int marklin_tid = WhoIs("mio");
    char input[20];
    int input_index = 0;
    int input_col = 3;

    char new_line_str[] = "\033[0;1H\033[K> ";
    new_line_str[2] = INPUT_ROW;

    char next_char_str[] = "\033[0;0H ";
    next_char_str[2] = INPUT_ROW;

    uint32_t last_speed[100];
    for(int i = 0; i<100; i++){
      last_speed[i] = 0;
    }

    for(;;){
        int console_tid = WhoIs("cout");
        char c = Getc(console_tid, CONSOLE);
        if (c == '\r') {
            input[input_index] = '\0';
            if(input[0] == 'q' && input[1]=='\0') {
                Quit();
            }
            executeFunction(console_tid, marklin_tid, input, last_speed);

            input_index = 0;
            // clear input line and add >
            Puts(console_tid, CONSOLE, new_line_str);
            input_col = 3;
        }else{
            //add char to input buffer
            input[input_index] = c;
            input_index += 1;

            //put char on screen
            next_char_str[4] = input_col;
            next_char_str[6] = c;
            Puts(console_tid, CONSOLE, next_char_str);
            input_col += 1;
        } 
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

void setup(){
    int console_tid = WhoIs("cout");
    int marklin_tid = WhoIs("mio");

    Puts(console_tid, CONSOLE, "\033[2J");
    Printf(console_tid, CONSOLE, "\033[%u;1H> ", INPUT_ROW);
    Printf(console_tid, CONSOLE, "\033[%u;3H", INPUT_ROW);

    Putc(marklin_tid, MARKLIN, 96);
    Putc(marklin_tid, MARKLIN, 255);
    Putc(marklin_tid, MARKLIN, 0xC0);
    Putc(marklin_tid, MARKLIN, 255);

    switchesSetup(console_tid, marklin_tid);
    char *s1 = "Switches\r\n";
    char *s2 = "001: C   002: C   003: C   004: C   005: C   006: S   007: S   008: C\r\n";
    char *s3 = "009: C   010: C   011: C   012: C   013: C   014: C   015: C   016: C\r\n";
    char *s4 = "017: C   018: C   153: C   154: S   155: S   156: C";
    Printf(marklin_tid, MARKLIN, "\033[%u;1H", SWITCHES_ROW);
    Puts(marklin_tid, MARKLIN, s1);
    Puts(marklin_tid, MARKLIN, s2);
    Puts(marklin_tid, MARKLIN, s3);
    Puts(marklin_tid, MARKLIN, s4);

    Printf(console_tid, CONSOLE, "\033[%u;1H\033[K", SENSORS_ROW);
    Puts(console_tid, CONSOLE, "Most recent sensors: ");

    Create(3, &console_time);
    Create(3, &sensor_update);
    Create(3, &user_input);
}


void rootTask(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Root Task\r\n");
    #endif 
    // order or nameserver and idle_task matters since their tid values are assumed in implementation
    Create(2, &nameserver);
    Create(1000, &idle_task);
    Create(0, &clock_notifier);
    Create(1, &clock);

    Create(3, &console_out_notifier);
    Create(3, &console_in_notifier);
    Create(3, &console_out);
    Create(3, &console_in);

    Create(3, &marklin_out_notifier);
    Create(3, &marklin_in_notifier);
    Create(3, &marklin_io);
    
    // Create(3, &k4);
    Create(3, &setup);
    
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}
