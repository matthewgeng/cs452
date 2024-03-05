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
    Create(1, &nameserver);
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
            executeFunction(cout, marklin_tid, reverse_tid, clock, input, last_speed);

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

void measure_velocity_loop(){
    /*
    distance:
    R1(1) + R1(1) + 77.5 + RT + 94.2 + 77.5*4 + 94.2 + LT + 94.2 + R1(1) + R1(2) + R1(1) +
    R1(2) + 77.5*2 + R1(2) + R1(1) + R1(2) + R1(1) + 94.2 + RT + 94.2 + 77.5*4 + 94.2 +
    LT + 77.5 + R1(1) + R1(1) + R1(2) + R1(1) + R1(1) + 77.5*2 + R1(1) + R1(1) + R1(2)

    R1(1)*12 + R1(2)*6 + RT*2 + LT*2 + 77.5*14 + 94.2*6

    R1(1): 2pi*360 / 24 = 94.25
    R1(2): 2pi*360 / 12 = 188.50
    RT: 188.3
    LT: 188.3
    
    94.25*12 + 188.5*6 + 188.3*4 + 77.5*14 + 94.2*6 = 4665.4mm
    */

    int clock_tid = WhoIs("clock\0");
    int cout = WhoIs("cout\0");
    int mio = WhoIs("mio\0");
    int time;
    char sensor_byte;

    //TODO: Change
    int train_to_measure = 2;
    int sensor_to_measure = 17;

    int speeds_to_measure[] = {12,14,4,6,8,10,12,14};
    int num_loops = 2;
    int loop_count;
    int start_time;
    int prev_sensor_trig = -100;

    for(int i = 0; i<6; i++){
        tr_v_measure(mio, train_to_measure, speeds_to_measure[i]);
        uart_printf(CONSOLE, "\033[%u;1H\033[Kspeed: %d", i+30, speeds_to_measure[i]);
        loop_count = -1;
        time = Time(clock_tid);
        Delay(clock_tid, 100);
        while(loop_count!=num_loops){
            Putc(mio, MARKLIN, 0x85);
            // TODO: change number
            for(int k = 0; k<10; k++){
                sensor_byte = Getc(mio, MARKLIN);
                for (int u = 0; u < 8; u++) {
                    if (sensor_byte & (1 << u)) {
                        int sensorNum = k*8+7-u;
                        if(sensorNum == sensor_to_measure && time-prev_sensor_trig>100){
                            loop_count += 1;
                            if(loop_count==0){
                                start_time = Time(clock_tid);
                            }else if(loop_count==num_loops){
                                int time_diff = (Time(clock_tid)-start_time)*10; // unit ms
                                uart_printf(CONSOLE, "\033[%u;1H\033[Kspeed: %d, time: %d", i+30, speeds_to_measure[i], time_diff);
                            }
                            uart_printf(CONSOLE, "\033[19;1H\033[Ktime: %d, actual time: %d",time, Time(clock_tid));
                            uart_printf(CONSOLE, "\033[20;1H\033[Knum loop: %d",loop_count);
                            prev_sensor_trig = time;
                        }
                    }
                }
            }
            time += 10;
            DelayUntil(clock_tid, time);
        }
    }

    /*
    Results:
    train 47
    4: 52000
    6: 32000
    8: 23800
    10: 19400
    12: 16100
    14: 15100

    train 55
    4: 164600
    6: 103400
    8: 53800
    10: 34100
    12: 24260
    14: 18800

    train 2
    4: 54600
    6: 34200
    8: 24400
    10: 19000
    12: 15800
    14: 14700

    */
}

void measure_velocity_straight(){
    /*
    distance:
    */

    int clock_tid = WhoIs("clock\0");
    int cout = WhoIs("cout\0");
    int mio = WhoIs("mio\0");
    int time = Time(clock_tid);;
    char sensor_byte;

    //TODO: Change
    int train_to_measure = 47;
    int start_sensor = 60;
    int end_sensor = 17;

    int speeds_to_measure[] = {12,14,4,6,8,10};
    int num_loops = 2;
    int loop_count;
    int start_time;
    int total_time;

    int prev_trig_start = -100;
    int prev_trig_end = -100;

    for(int i = 0; i<6; i++){
        tr_v_measure(mio, train_to_measure, speeds_to_measure[i]);
        uart_printf(CONSOLE, "\033[%u;1H\033[Kspeed: %d", i+30, speeds_to_measure[i]);
        loop_count = -1;
        start_time = -1;
        total_time = 0;
        while(loop_count!=num_loops){
            Putc(mio, MARKLIN, 0x85);
            // TODO: change number
            for(int k = 0; k<10; k++){
                sensor_byte = Getc(mio, MARKLIN);
                for (int u = 0; u < 8; u++) {
                    if (sensor_byte & (1 << u)) {
                        int sensorNum = k*8+7-u;
                        if(sensorNum == start_sensor && time-prev_trig_start>100){
                            start_time = Time(clock_tid);
                            uart_printf(CONSOLE, "\033[18;1H\033[Kstart time: %d",start_time);
                            prev_trig_start = start_time;
                        }
                        if(sensorNum == end_sensor && start_time != -1 && time-prev_trig_end>100){
                            loop_count += 1;
                            int end_time = Time(clock_tid);
                            if(loop_count>0){
                                total_time += (end_time - start_time);
                                if(loop_count==num_loops){
                                    uart_printf(CONSOLE, "\033[%u;1H\033[Kspeed: %d, time: %d", i+30, speeds_to_measure[i], total_time);
                                }
                            }
                            uart_printf(CONSOLE, "\033[19;1H\033[Kend time: %d",end_time);
                            uart_printf(CONSOLE, "\033[20;1H\033[Ktotal time: %d",total_time);
                            uart_printf(CONSOLE, "\033[21;1H\033[Knum loop: %d",loop_count);
                            prev_trig_end = end_time;
                        }
                    }
                }
            }
            time += 10;
            DelayUntil(clock_tid, time+2);
        }
    }

    /*
    Results:
    train 2
    4: 474
    6: 294
    8: 204
    10: 154
    12: 134
    14: 114
    */
}

void measure_stop_dist(){
    /*
    distance:
    */

    int clock_tid = WhoIs("clock\0");
    int cout = WhoIs("cout\0");
    int mio = WhoIs("mio\0");
    int time = Time(clock_tid);;
    char sensor_byte;

    int train_to_measure = 47;
    int stop_sensor = 0;

    for(;;){
        Putc(mio, MARKLIN, 0x85);
        // TODO: change number
        for(int k = 0; k<10; k++){
            sensor_byte = Getc(mio, MARKLIN);
            for (int u = 0; u < 8; u++) {
                if (sensor_byte & (1 << u)) {
                    int sensorNum = k*8+7-u;
                    if(sensorNum == stop_sensor){
                        tr_v_measure(mio, train_to_measure, 0);
                    }
                }
            }
        }
        time += 10;
        DelayUntil(clock_tid, time+2);
    }

    /*
    Results:
    train 47
    4: 
    6: 
    8: 
    10: 
    12:
    14:
    */
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
    char *s2 = "001:     002:     003:     004:     005:     006:     007:     008:  \r\n";
    char *s3 = "009:     010:     011:     012:     013:     014:     015:     016:  \r\n";
    char *s4 = "017:     018:     153:     154:     155:     156:  ";
    // char *s2 = "001: C   002: C   003: C   004: C   005: C   006: S   007: S   008: C\r\n";
    // char *s3 = "009: C   010: C   011: C   012: C   013: C   014: C   015: C   016: C\r\n";
    // char *s4 = "017: C   018: C   153: C   154: S   155: S   156: C";
    printf(cout, CONSOLE, "\033[3;1H");
    Puts(cout, CONSOLE, s1);
    Puts(cout, CONSOLE, s2);
    Puts(cout, CONSOLE, s3);
    Puts(cout, CONSOLE, s4);

    printf(cout, CONSOLE, "\033[%u;1H\033[KMost recent sensors: ", SENSORS_ROW);

    Create(3, &console_time);
    // Create(3, &sensor_update);
    // Create(4, &reverse);
    // Create(5, &user_input);
    Create(2, &measure_velocity_straight);
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

    Create(3, &marklin_out_tx_notifier);
    Create(3, &marklin_out_cts_notifier);
    Create(3, &marklin_in_notifier);
    Create(3, &marklin_io);
    
    // Create(3, &k4);
    Create(3, &setup);
    
    // uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
}
