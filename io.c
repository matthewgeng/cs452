#include "io.h"
#include "syscall.h"
#include "rpi.h"
#include "nameserver.h"
#include "util.h"
#include "char_cb.h"
#include "int_cb.h"
#include "clock.h"
#include <stdarg.h>

// notifier needed to still allow function calls to server with buffering
void console_out_notifier() {
    RegisterAs("cout_notifier");

    int cout = WhoIs("cout");
    int clock = WhoIs("clock");
    MessageStr m = {"", 0};
    for(;;){
        int res = AwaitEvent(CONSOLE_TX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console out notifier\r\n");
        }
        res = Send(cout, (char*)&m, sizeof(MessageStr), NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console out notifier\r\n");
        }
    }
}

// notifier needed to still allow function calls to server with buffering
void console_in_notifier() {
    RegisterAs("cin_notifier");

    int cin = WhoIs("cin");
    int console_tid = WhoIs("cout");
    int clock = WhoIs("clock");

    for(;;){
        int res = AwaitEvent(CONSOLE_RX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }
        // printf(console_tid, 0, "\0337\033[28;4H cin notifier before send %u\0338", Time(clock));

        res = Send(cin, NULL, 0, NULL, 0);            
        // printf(console_tid, 0, "\0337\033[29;4H cin notifier after send %u\0338", Time(clock));

        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }
    }
}

void console_out() {

    RegisterAs("cout");

    int cout_notifier = WhoIs("cout_notifier");
    int notifier_parked = 0;

    int tid;
    // circular buffer 
    // TODO: make constant a variable
    char raw_buffer[512];
    CharCB buffer;
    initialize_charcb(&buffer, raw_buffer, sizeof(raw_buffer), 0);
    // message struct
    MessageStr m;
    // uart_printf(CONSOLE, "cout initialized\r\n");

    for(;;) {
        Receive(&tid, (char*)&m, sizeof(MessageStr));

        // print
        if (tid == cout_notifier) {
            // uart_printf(CONSOLE, "notifier recieved\r\n");
            // try to flush buffer 
            // uart_printf(CONSOLE, "message length from noti %u\r\n", m.len);

            while (uart_can_write(CONSOLE) && !is_empty_charcb(&buffer)) {
                uart_writec(CONSOLE, pop_charcb(&buffer));
            }


            // uart_printf(CONSOLE, "\0337\033[30;1Hbuffer size %u\0338", buffer.count);
            // if we have data still, we should wait for the notifier to let us flush
            if (!is_empty_charcb(&buffer)) {
                // reply to notifier
                Reply(tid, NULL, 0);
                notifier_parked = 0;
            // if we don't have data, we shouldn't respond to the notifier
            } else {
                notifier_parked = 1;
            }

        // store data in buffer
        } else {
            // uart_printf(CONSOLE, "message length from user %u\r\n", m.len);

            // store data in buffer
            for (uint32_t i = 0; i < m.len; i++) {
                push_charcb(&buffer, m.str[i]);
            }
            
            // reply to sender
            Reply(tid, NULL, 0);

            // try to flush buffer 
            while (uart_can_write(CONSOLE) && !is_empty_charcb(&buffer)) {
                uart_writec(CONSOLE, pop_charcb(&buffer));
            }

            // if we have data still, we should wait for the notifier to let us flush
            if (!is_empty_charcb(&buffer) && notifier_parked) {
                notifier_parked = 0;
                // reply to notifier
                Reply(cout_notifier, NULL, 0);
            }
        }
    }
}

void console_in() {
    RegisterAs("cin");

    int cin_notifier = WhoIs("cin_notifier");
    int cout = WhoIs("cout");
    int clock = WhoIs("clock");

    int tid;
    int notifier_parked = 0;
    // circular buffer 
    int task_buffer[32];
    IntCB task_cb;
    initialize_intcb(&task_cb, task_buffer, sizeof(task_buffer)/sizeof(task_buffer[0]), 0);
    char data_buffer[32];
    CharCB data_cb;
    initialize_charcb(&data_cb, data_buffer, sizeof(data_buffer), 0);
    
    char c;

    for(;;) {
        Receive(&tid, &c, 1);

        // print
        if (tid == cin_notifier) {        
            // uart_printf(CONSOLE, "\0337\033[24;4H recieved cin event time %u\0338", Time(time));
            // printf(cout, 0, "\0337\033[24;4H recieved cin event time %u\0338", Time(clock));

            // get data
            while (uart_can_read(CONSOLE)) {
                char data = uart_readc(CONSOLE);            

                push_charcb(&data_cb, data);
            }

            // try to flush buffer 
            while (!is_empty_charcb(&data_cb) && !is_empty_intcb(&task_cb)) {
                int task = pop_intcb(&task_cb);
                char data = pop_charcb(&data_cb);
                Reply(task, &data, 1);
            }

            /*
            no data && no people --> park
            no data && people --> reply 

            data && no people --> park
            data && people --> park
            
            */
            if (is_empty_charcb(&data_cb) && !is_empty_intcb(&task_cb)) {
                Reply(tid, NULL, 0);
                notifier_parked = 0;
            } else {
                notifier_parked = 1;        
            }

        } else {
            // printf(cout, 0, "\0337\033[31;4H getc received %u\0338", Time(clock));
            push_intcb(&task_cb, tid);

            while (!is_empty_charcb(&data_cb) && !is_empty_intcb(&task_cb)) {
                int task = pop_intcb(&task_cb);
                char data = pop_charcb(&data_cb);

                // printf(cout, 0, "\0337\033[40;4H getc before replying data %c %u\0338", data, Time(clock));
                Reply(task, &data, 1);
            }


            /*
            no data && no people --> park
            no data && people --> reply 

            data && no people --> park
            data && people --> park
            */

            if (is_empty_charcb(&data_cb) && !is_empty_intcb(&task_cb) && notifier_parked) {
                Reply(cin_notifier, NULL, 0);
                notifier_parked = 0;
            } else {
                notifier_parked = 1;
            }
        }
    }
}

// notifier needed to still allow function calls to server with buffering
void marklin_out_notifier() {
    RegisterAs("mout_notifier");

    int mio = WhoIs("mio");

    for(;;){
        int res = AwaitEvent(MARKLIN_TX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }

        res = Send(mio, NULL, 0, NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }
    }

}

// notifier needed to still allow function calls to server with buffering
void marklin_in_notifier() {
    RegisterAs("min_notifier");

    int mio = WhoIs("mio");

    for(;;){
        int res = AwaitEvent(MARKLIN_RX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }

        res = Send(mio, NULL, 0, NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }
    }
}

void marklin_io() {
    RegisterAs("mio");

    int mout = WhoIs("mout_notifier");
    int min = WhoIs("min_notifier");

    int tid;
    // circular buffer 
    // TODO: make constant a variable
    char raw_buffer[512];
    CharCB buffer;
    initialize_charcb(&buffer, raw_buffer, 200, 0);
    // message struct
    MessageStr m;

    // implement delay
    int clock_tid = WhoIs("clock");
    int time = Time(clock_tid);

    for(;;) {
        Receive(&tid, (char*)&m, sizeof(MessageStr));

        // print
        if (tid == mout) {
            // uart_printf(CONSOLE, "notifier recieved\r\n");
            // try to flush buffer 
            // uart_printf(CONSOLE, "message length from noti %u\r\n", m.len);

            while (uart_can_write(MARKLIN) && !is_empty_charcb(&buffer)) {
                uart_writec(MARKLIN, pop_charcb(&buffer));
            }

            // reply to notifier
            Reply(tid, NULL, 0);
        // store data in buffer
        } else if (tid == min) {
            char c;
            while (uart_can_read(MARKLIN) && !is_empty_charcb(&buffer)) {
                c = uart_readc(MARKLIN);
                push_charcb(&buffer, c);
            }

            // reply to notifier
            Reply(tid, NULL, 0);
        // store data in buffer
        } else {
            // uart_printf(CONSOLE, "message length from user %u\r\n", m.len);

            // store data in buffer
            for (uint32_t i = 0; i < m.len; i++) {
                push_charcb(&buffer, m.str[i]);
            }

            if((int)(peek_charcb(&buffer)) == 255){
                DelayUntil(clock_tid, time+15);
            }else if((int)(peek_charcb(&buffer)) == 254){
                // TODO: maybe need to do smth to pause sensor queries here
                DelayUntil(clock_tid, time+250);
            }

            // try to flush buffer 
            while (uart_can_write(MARKLIN) && !is_empty_charcb(&buffer)) {
                uart_writec(MARKLIN, pop_charcb(&buffer));
                time = Time(clock_tid);
            }

            // reply to sender
            Reply(tid, NULL, 0);
        }
    }

}


// TODO: channel is not being used. Should it?

int Getc(int tid, int channel) {
    char r;

    int console_tid = WhoIs("cout");
    int res = Send(tid, NULL, 0, &r, 1);
    // for(;;){}
    if (res < 0) {
        return -1;
    }
    return r;
}

int Putc(int tid, int channel, unsigned char ch) {

    char r;
    MessageStr m = {
        &ch,
        1
    };

    int res = Send(tid, (char*)&m, sizeof(MessageStr), &r, 1);
    if (res < 0) {
        return -1;
    }
    // TODO: do something with reply

    return 0;
}


int Puts(int tid, int channel, unsigned char* ch) {

    char r;
    MessageStr m = {
        ch,
        str_len(ch)
    };

    int res = Send(tid, (char*)&m, sizeof(MessageStr), &r, 1);
    if (res < 0) {
        return -1;
    }
    // TODO: do something with reply

    return 0;
}

static void format_print (int tid, int channel, char *fmt, va_list va ) {
	char bf[12] = {0};
	char ch;
    char buffer[128] = {0};
    int i = 0;

    while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			Putc( tid, channel, ch );
		else {
			ch = *(fmt++);
			switch( ch ) {
			case 'u':
				ui2a( va_arg( va, unsigned int ), 10, bf );
				Puts( tid, channel, bf );
				break;
			case 'd':
				i2a( va_arg( va, int ), bf );
				Puts( tid, channel, bf );
				break;
			case 'x':
				ui2a( va_arg( va, unsigned int ), 16, bf );
				Puts( tid, channel, bf );
				break;
			case 's':
				Puts( tid, channel, va_arg( va, char* ) );
				break;
			case '%':
				Putc( tid, channel, ch );
				break;
      case '\0':
        return;
			}
		}
	}
}

// static void format_print (int tid, int channel, char *fmt, va_list va ) {
// 	char bf[12] = {0};
// 	char ch;
//     int len;
//     const int buffer_size = 128;
//     char buffer[128] = {0};
//     int i = 0;

//     while ( ( ch = *(fmt++) )  && i < buffer_size) {
// 		if ( ch != '%' ){ 
//             buffer[i] = ch;
//             i++;
// 		} else {
// 			ch = *(fmt++);
// 			switch( ch ) {
//                 case 'u':
//                     len = ui2a( va_arg( va, unsigned int ), 10, bf );
//                     // Puts( tid, channel, bf );
//                     memcpy(buffer + i, bf, len);
//                     i += len;
//                     break;
//                 case 'd':
//                     len = i2a( va_arg( va, int ), bf );
//                     // Puts( tid, channel, bf );
//                     memcpy(buffer + i, bf, len);
//                     i += len;
//                     break;
//                 case 'x':
//                     len = ui2a( va_arg( va, unsigned int ), 16, bf );
//                     // Puts( tid, channel, bf );
//                     memcpy(buffer + i, bf, len);
//                     i += len;
//                     break;
//                 case 's':
//                     char* s = va_arg( va, char* );
//                     len = str_len(s);
//                     // Puts( tid, channel, va_arg( va, char* ) );
//                     memcpy(buffer + i, s, len);
//                     i += len;
//                     break;
//                 case '%':
//                     // Putc( tid, channel, ch );
//                     buffer[i] = ch;
//                     i++;
//                     break;
//                 case '\0':
//                     Puts(tid, channel, buffer);
//                     return;
//             }
//         }
// 	}

//     if (i < buffer_size) {
//         Puts(tid, channel, buffer);
//     } else {
//         uart_printf(CONSOLE, "print >= 128 bytes");
//         for(;;){}
//     }
// }

void printf(int tid, int channel, char *fmt, ... ) {
	va_list va;
	va_start(va,fmt);
	format_print( tid, channel, fmt, va );
	va_end(va);
}
