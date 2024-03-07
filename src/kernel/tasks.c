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
#include "pathfinding.h"
#include "traincontrol.h"


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
    Create_sp_size(1, &nameserver, 1);
    Create(2, &gameserver);
    Create(100, &k2test);
    
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}

void time_user() {
    int tid = MyTid();
    int parent = MyParentTid();
    int clock = WhoIs("clock\0"); // TODO: make every server have a function to get name

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

void k4(){
    uart_printf(CONSOLE, "in k4\r\n");
    int cout = WhoIs("cout\0");
    int cin = WhoIs("cin\0");
    int clock = WhoIs("clock\0"); // TODO: make every server have a function to get name

    // clear screen
    Puts(cout, 0, "\033[2J");
    // reset cursor to top left
    Puts(cout, 0, "\033[H");
    // program start
    Puts(cout, 0, "Program starting: \r\n\r\n");
    Puts(cout, 0, "Idle percentage:   \r\n");

    Puts(cout, 0, "running k4\r\n");

    for(;;) {
        char c = Getc(cin, 0);
        Puts(cout, 0, "got data: ");
        Putc(cout, 0, c);
        Puts(cout, 0, "\r\n");
    }
}

void idle_task(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Idle Task\r\n");
    #endif 
    int console_tid = WhoIs("cout\0");
    // int clock = WhoIs("clock");
    // int time = Time(clock);
    uint32_t count = 0;
    for(;;){
        //TODO: maybe make this better somehow
        // int cur_time = Time(clock);
        if(count%30==0){
            uint32_t idle_time_percent = (*p_idle_task_total*100)/(sys_time() - *p_program_start);
            char str[] = "\0337\033[14;1HIdle:   % \0338";
            ui2a_no0(idle_time_percent, 10, str+15);
            // ui2a_no0(cur_time, 10, str+19);
            Puts(console_tid, CONSOLE, str);
            // time = cur_time;
        }
        count += 1;
        asm volatile("wfi");
    }
}

void i2str(int i, char *str){
    char bf[3];
    ui2a( i, 10, bf );
    if(i<10){
        str[1] = bf[0];
    }else{
        str[0] = bf[0];
        str[1] = bf[1];
    }
}

void console_time(){
    int clock_tid = WhoIs("clock\0");
    int console_tid = WhoIs("cout\0");
    char time_str[] = "\0337\033[1;1H\033[K00:00. \0338";
    char bf[3];
    int time = Time(clock_tid);
        // char tmp_bf[20];
        // ui2a(time, 10, tmp_bf);
        // Puts(console_tid, CONSOLE, tmp_bf);
    for(;;){
        // uart_printf(CONSOLE, "\0337\033[15;1H\033[Ktime%u\0338", time);
        unsigned int minutes = time/100/60;
        unsigned int seconds = (time/100)%60;
        unsigned int tenthOfSecond = (time/10)%10;
        i2str(minutes, time_str+11);
        i2str(seconds, time_str+14);
        ui2a( tenthOfSecond, 10, bf );
        time_str[17] = bf[0];
        Puts(console_tid, CONSOLE, time_str);
        time += 10;
        DelayUntil(clock_tid, time);        
    }
}

void sensor_update(){
  int clock_tid = WhoIs("clock\0");
  int cout = WhoIs("cout\0");
  int mio = WhoIs("mio\0");
  int time = Time(clock_tid);;
  int last_sensor_triggered = 1000;
  char sensor_byte;

  int sensors[12];
  IntCB sensor_cb;
  initialize_intcb(&sensor_cb, sensors, 12, 1);

  int sensors_changed = 0;
  char sensors_str[] = "\033[8;1H\033[KMost recent sensors:                                                           ";
  int sensors_str_index;
  for(;;){
    Putc(mio, MARKLIN, 0x85);
    // uart_printf(CONSOLE, "\033[36;1Hput sensor character, %d \r\n", Time(clock_tid));
    // TODO: change number
    for(int i = 0; i<10; i++){
      sensor_byte = Getc(mio, MARKLIN);
    //   uart_printf(CONSOLE, "\033[37;1Hgot sensor character, %d, %d, time:%d \r\n", i, (int)sensor_byte, Time(clock_tid));
      for (int u = 0; u < 8; u++) {
        if (sensor_byte & (1 << u)) {
          int sensorNum = i*8+7-u;
        //   uart_printf(CONSOLE, "\033[38;1Hsensor triggered, %d, %d \r\n", sensorNum, Time(clock_tid));
          if(sensorNum!=last_sensor_triggered){
            push_intcb(&sensor_cb, sensorNum);
            last_sensor_triggered = sensorNum;
            sensors_changed = 1;
          }
        }
      }
    }
    if(sensors_changed){
        // Puts(cout, CONSOLE, sensors_str);
        // uart_printf(CONSOLE, "\033[39;1Hsensor changed, %d \r\n", Time(clock_tid));
        sensors_str_index = 31;
        int cb_count = 0;
        int cb_index = decrement_intcb(sensor_cb.capacity, sensor_cb.end);
        int i;
        char sensor_band;
        int sensor_num;
        while(cb_count < sensor_cb.count){
            i = sensor_cb.queue[cb_index];
            sensor_band = i/16 + 'A';
            sensors_str[sensors_str_index] = sensor_band;
            sensor_num = i%16 + 1;
            ui2a( sensor_num, 10, sensors_str+sensors_str_index+1 );
            if(sensor_num<10){
                sensors_str[sensors_str_index+2] = ' ';
                sensors_str_index = sensors_str_index+3;
            }else{
                sensors_str[sensors_str_index+3] = ' ';
                sensors_str_index = sensors_str_index+4;
            }
            cb_index = decrement_intcb(sensor_cb.capacity, cb_index);
            cb_count += 1;
        }
        sensors_str[sensors_str_index] = '\0';
        Puts(cout, CONSOLE, sensors_str);
        sensors_changed = 0;
    }
    time += 10;
    DelayUntil(clock_tid, time);
  }
}

void user_input(){
    int cout = WhoIs("cout\0");
    int cin = WhoIs("cin\0");
    int marklin_tid = WhoIs("mio\0");
    int clock = WhoIs("clock\0");
    int reverse_tid = WhoIs("reverse\0");
    int pathfind_tid = WhoIs("pathfind\0");
    int max_input_len = 20;
    char input[max_input_len+2];
    int input_index = 0;
    int input_col = 3;

    char new_line_str[] = "\033[10;1H\033[K> ";
    Puts(cout, CONSOLE, new_line_str);
    // new_line_str[2] = INPUT_ROW;

    char next_char_str[] = "\033[10;0H  ";
    // next_char_str[2] = INPUT_ROW;

    uint32_t last_speed[100];
    for(int i = 0; i<100; i++){
      last_speed[i] = 0;
    }
    for(;;){

        char c = Getc(cin, CONSOLE);
        // uart_printf()
        if (c == '\r') {
            input[input_index] = '\0';
            if(input[0] == 'q' && input[1]=='\0') {
                Quit();
            }
            executeFunction(cout, marklin_tid, reverse_tid, pathfind_tid, clock, input, last_speed);

            input_index = 0;
            // clear input line and add >
            Puts(cout, CONSOLE, new_line_str);
            input_col = 3;
        }else if (c == '\b'){
            if(input_col>3){
                input_index -= 1;
                input[input_index] = ' ';
                input[input_index+1] = '\0';

                input_col -= 1;
                ui2a_no0(input_col, 10, next_char_str+5);
                if(input_col<10){
                    next_char_str[6] = 'H';
                    next_char_str[7] = ' ';
                    next_char_str[8] = '\0';
                }else{
                    next_char_str[7] = 'H';
                    next_char_str[8] = ' ';
                }
                Puts(cout, CONSOLE, next_char_str);
                if(input_col<10){
                    next_char_str[6] = 'H';
                    next_char_str[7] = '\0';
                }else{
                    next_char_str[7] = 'H';
                    next_char_str[8] = '\0';
                }
                Puts(cout, CONSOLE, next_char_str);
            }
        }else{
            //add char to input buffer
            if(input_index>=max_input_len){
                input_index = 0;
                input_col = 3;
                Puts(cout, CONSOLE, new_line_str);
                char err[] = "\033[12;1H\033[Kinput exceeded maximum number of characters";
                Puts(cout, CONSOLE, err);
            }else{
                input[input_index] = c;
                input[input_index+1] = '\0';
                input_index += 1;

                //put char on screen
                ui2a_no0(input_col, 10, next_char_str+5);
                if(input_col<10){
                    next_char_str[6] = 'H';
                    next_char_str[7] = c;
                    next_char_str[8] = '\0';
                }else{
                    next_char_str[7] = 'H';
                    next_char_str[8] = c;
                }
                // next_char_str[5] = input_col;
                Puts(cout, CONSOLE, next_char_str);
                input_col += 1;
            }
        } 
    }
}


void setup(){

    int cout = WhoIs("cout\0");
    int marklin_tid = WhoIs("mio\0");
    
    // clear screen
    Puts(cout, 0, "\033[2J");
    // reset cursor to top left
    Puts(cout, 0, "\033[H");

    Puts(cout, CONSOLE, "\033[2J");
    // printf(cout, CONSOLE, "\033[%u;1H> ", INPUT_ROW);
    // printf(cout, CONSOLE, "\033[%u;3H", INPUT_ROW);

    Putc(marklin_tid, MARKLIN, 96);
    Putc(marklin_tid, MARKLIN, 0xC0);

    switchesSetup(cout, marklin_tid);
    char *s1 = "Switches\r\n";
    char *s2 = "001: C   002: C   003: C   004: C   005: C   006: S   007: S   008: C\r\n";
    char *s3 = "009: C   010: C   011: C   012: C   013: C   014: C   015: C   016: C\r\n";
    char *s4 = "017: C   018: C   153: C   154: S   155: S   156: C";
    printf(cout, CONSOLE, "\033[3;1H");
    Puts(cout, CONSOLE, s1);
    Puts(cout, CONSOLE, s2);
    Puts(cout, CONSOLE, s3);
    Puts(cout, CONSOLE, s4);

    printf(cout, CONSOLE, "\033[%u;1H\033[KMost recent sensors: ", SENSORS_ROW);

    Create(3, &console_time);
    Create_sp_size(1, &path_finding, 2);
    // Create(3, &sensor_update);
    // Create(4, &reverse);
    // Create(5, &user_input);
}


void rootTask(){
    #if DEBUG
        uart_dprintf(CONSOLE, "Root Task\r\n");
    #endif 
    // uart_printf(CONSOLE, "in root task\r\n");
    // order of nameserver and idle_task matters since their tid values are assumed in implementation
    Create_sp_size(2, &nameserver, 1);
    // Create(2, &nameserver);
    Create(1000, &idle_task);
    Create(0, &clock_notifier);
    Create(1, &clock);

    uart_printf(CONSOLE, "1\r\n");
    Create(3, &console_out_notifier);
    Create(3, &console_in_notifier);
    Create(3, &console_out);
    Create(3, &console_in);

    uart_printf(CONSOLE, "2\r\n");
    Create(3, &marklin_out_tx_notifier);
    Create(3, &marklin_out_cts_notifier);
    Create(3, &marklin_in_notifier);
    Create(3, &marklin_io);
    
    uart_printf(CONSOLE, "3\r\n");
    Create(3, &k3);
    // Create(3, &setup);
    
    // uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}
