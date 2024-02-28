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
    // TODO: maybe we can send none?
    IOMessage m = {0, "", 0};
    for(;;){
        int res = AwaitEvent(CONSOLE_TX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console out notifier\r\n");
        }
        res = Send(cout, (char*)&m, sizeof(IOMessage), NULL, 0);
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

    // TODO: maybe we can send none?
    IOMessage m = {0, "", 0};
    for(;;){
        int res = AwaitEvent(CONSOLE_RX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "AWAITEVENT ERROR console in notifier\r\n");
        }

        res = Send(cin, &m, sizeof(IOMessage), NULL, 0);            
        // printf(console_tid, 0, "\0337\033[29;4H cin notifier after send %u\0338", Time(clock));

        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "SEND ERROR console in notifier\r\n");
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
    IOMessage m;
    // uart_printf(CONSOLE, "cout initialized\r\n");

    for(;;) {
        Receive(&tid, (char*)&m, sizeof(IOMessage));

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
    
    // waiting tasks
    int task_buffer[32];
    IntCB task_cb;
    initialize_intcb(&task_cb, task_buffer, sizeof(task_buffer)/sizeof(task_buffer[0]), 0);

    // data
    char data_buffer[32];
    CharCB data_cb;
    initialize_charcb(&data_cb, data_buffer, sizeof(data_buffer), 0);
    
    IOMessage m;

    for(;;) {
        Receive(&tid, (char*)&m, sizeof(IOMessage));

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
void marklin_out_tx_notifier() {
    RegisterAs("mout_tx_notifier");

    int mio = WhoIs("mio");

    IOMessage m = {0, "", 0};
    for(;;){
        // uart_printf(CONSOLE, "\r\n before await tx cur cts %u\r\n", uart_cts(MARKLIN));
        int res = AwaitEvent(MARKLIN_TX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR marklin out notifier\r\n");
        }

        res = Send(mio, (char*)&m, sizeof(IOMessage), NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR marklin out notifier\r\n");
        }
    }

}

void marklin_out_cts_notifier() {
    RegisterAs("mout_cts_notifier");

    int mio = WhoIs("mio");

    IOMessage m = {0, "", 0};
    for(;;){
        // uart_printf(CONSOLE, "\r\n before await cts cur cts %u\r\n", uart_cts(MARKLIN));
        int res = AwaitEvent(MARKLIN_CTS);
        if (res < 0) {
            // TODO: make more robust
            // uart_printf(CONSOLE, "ERROR marklin out notifier\r\n");
        }

        // uart_printf(CONSOLE, "\n after await cur cts %u\r\n", uart_cts(MARKLIN));
        res = Send(mio, (char*)&m, sizeof(IOMessage), NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR marklin out notifier\r\n");
        }
        // uart_printf(CONSOLE, "\r\n after send and io server reply cur cts %u\r\n", uart_cts(MARKLIN));
    }

}

// notifier needed to still allow function calls to server with buffering
void marklin_in_notifier() {
    RegisterAs("min_notifier");

    int mio = WhoIs("mio");

    IOMessage m = {0, "", 0};
    for(;;){
        int res = AwaitEvent(MARKLIN_RX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR marklin in notifier\r\n");
        }

        res = Send(mio, (char*)&m, sizeof(IOMessage), NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR marklin in notifier\r\n");
        }
    }
}

void marklin_io() {
    RegisterAs("mio");

    int mout_tx_notifier = WhoIs("mout_tx_notifier");
    int mout_cts_notifier = WhoIs("mout_cts_notifier");
    int min_notifier = WhoIs("min_notifier");
    int clock_tid = WhoIs("clock");
    int time = Time(clock_tid);

    int tid;
    // OUTPUT
    char raw_out_buffer[128];
    CharCB out_buffer;
    initialize_charcb(&out_buffer, raw_out_buffer, 128, 0);
    int prev_cts = 1; // 1 --> high able to send, 0 --> down should not send
    int cts_transition = 2;
    // TODO: parking for output notifiers


    // INPUT
    int raw_in_tasks_buffer[32];
    IntCB task_buffer;
    initialize_intcb(&task_buffer, raw_in_tasks_buffer, 32, 0);
    char raw_in_buffer[32];
    CharCB in_buffer;
    initialize_charcb(&in_buffer, raw_in_buffer, 32, 0);
    int in_notifier_parked = 0;

    int tx_ready = 1;
    int min_counter = 0;

    IOMessage m;
    for(;;) {
        Receive(&tid, (char*)&m, sizeof(IOMessage));
        // uart_printf(CONSOLE, "message receivied prev_cts %u, cur_cts %u, transition %u\r\n", prev_cts, uart_cts(MARKLIN), cts_transition);

        // print
        if (tid == mout_tx_notifier) {
            // uart_printf(CONSOLE, "tx notifier recieved, %d\r\n", Time(clock_tid));
            // try to flush buffer 
            // uart_printf(CONSOLE, "message length from noti %u\r\n", m.len);
            
            tx_ready = 1;
            if (uart_can_write(MARKLIN) && cts_transition==2 && !is_empty_charcb(&out_buffer)) {
                char c = peek_charcb(&out_buffer);
                uart_writec(MARKLIN, pop_charcb(&out_buffer));
                // uart_printf(CONSOLE, "\n after writing %u prev cts = %u, cur cts %u\r\n", c, prev_cts, uart_cts(MARKLIN));
                cts_transition = 0;
                tx_ready = 0;
            } else {
                // uart_printf(CONSOLE, "\n couldn't write prev cts %u, cur cts %u\r\n", prev_cts, uart_cts(MARKLIN));
            }

            // uart_printf(CONSOLE, "\n after writing prev cts = %u, cur cts %u\r\n", prev_cts, uart_cts(MARKLIN));

            // reply to notifier
            Reply(tid, NULL, 0);
            // trywrite()
        } else if (tid == mout_cts_notifier) {
            // cts turning off
            int cur_cts = uart_cts(MARKLIN);

            if (cur_cts == 0 && cts_transition==0) {
                prev_cts = cur_cts;
                cts_transition = 1;
                // uart_printf(CONSOLE, "\033[30;1Hcts interrupt 0, %d\r\n", Time(clock_tid));
            
                // uart_printf(CONSOLE, "\n prev cts 1 cur cts %u should be 0\r\n", uart_cts(MARKLIN));
            // cts turning on
            } else if (cur_cts == 1 && cts_transition==1){
                prev_cts = cur_cts;
                cts_transition = 2;
                // uart_printf(CONSOLE, "\033[31;1Hcts interrupt 1, %d\r\n", Time(clock_tid));
                // uart_printf(CONSOLE, "\n prev cts 0 cur cts %u should be 1, transition should be true\r\n", uart_cts(MARKLIN));
            }

            if (uart_can_write(MARKLIN) && cts_transition==2 && tx_ready && !is_empty_charcb(&out_buffer)) {
                // char c = peek_charcb(&out_buffer);
                uart_writec(MARKLIN, pop_charcb(&out_buffer));
                // uart_printf(CONSOLE, "WRITING cts and replying to %u\r\n", tid);
                cts_transition = 0;
                tx_ready = 0;
            } else {
                // uart_printf(CONSOLE, "can't write cts and replying to %u\r\n", tid);
            }
            //trywrite()

            // reply to notifier
            Reply(tid, NULL, 0);
        // store data in buffer
        } else if (tid == min_notifier) {
            char c;
            while (uart_can_read(MARKLIN)) {
                // uart_printf(CONSOLE, "\033[35;1Hmin notifier recieved, count:%d, %d\r\n", min_counter, Time(clock_tid));
                min_counter += 1;
                c = uart_readc(MARKLIN);
                push_charcb(&in_buffer, c);
            }

            while (!is_empty_charcb(&in_buffer) && !is_empty_intcb(&task_buffer)) {
                int task = pop_intcb(&task_buffer);
                char data = pop_charcb(&in_buffer);
                Reply(task, &data, 1);
            }

            // if (is_empty_charcb(&in_buffer) && !is_empty_intcb(&task_buffer)) {
                Reply(tid, NULL, 0);
            //     in_notifier_parked = 0;
            // } else {
            //     in_notifier_parked = 1;        
            // }

        // store data in buffer
        } else {
            // uart_printf(CONSOLE, "user message from tid %u\r\n", tid);

            if (m.type == GETC) {
                push_intcb(&task_buffer, tid);

                while (!is_empty_charcb(&in_buffer) && !is_empty_intcb(&task_buffer)) {
                    int task = pop_intcb(&task_buffer);
                    char data = pop_charcb(&in_buffer);

                    // printf(cout, 0, "\0337\033[40;4H getc before replying data %c %u\0338", data, Time(clock));
                    Reply(task, &data, 1);
                }

                // if (is_empty_charcb(&in_buffer) && !is_empty_intcb(&task_buffer) && in_notifier_parked) {
                //     Reply(min_notifier, NULL, 0);
                //     in_notifier_parked = 0;
                // } else {
                //     in_notifier_parked = 1;
                // }

            } else if (m.type == PUTC || m.type == PUTS) {
                // store data in buffer
                for (uint32_t i = 0; i < m.len; i++) {
                    push_charcb(&out_buffer, m.str[i]);
                }

                // try write
                // if (uart_can_write(MARKLIN) && cts_transition==2 && tx_ready && !is_empty_charcb(&out_buffer)) {
                if (uart_can_write(MARKLIN) && cts_transition==2 && !is_empty_charcb(&out_buffer)) {
                    // char c = peek_charcb(&out_buffer);
                    // uart_printf(CONSOLE, "\033[29;1Hprinted, %d \r\n", Time(clock_tid));
                    uart_writec(MARKLIN, pop_charcb(&out_buffer));
                    // uart_printf(CONSOLE, "WRITING immediately and replying to %u\r\n", tid);
                    cts_transition = 0;
                    // tx_ready = 0;
                } else {
                    // uart_printf(CONSOLE, "can't write immediately cur cts = %u, cts_transition = %u, and replying to %u\r\n", uart_cts(MARKLIN), cts_transition,tid);
                }
                Reply(tid, NULL, 0);
            } else {
                uart_printf(CONSOLE, "io message type not supported\r\n");
                for(;;){}
            }
        }
    }

}


// TODO: channel is not being used. Should it?

int Getc(int tid, int channel) {
    char r;
    IOMessage m = {GETC, NULL, 0};

    int res = Send(tid, &m, sizeof(IOMessage), &r, 1);
    if (res < 0) {
        return -1;
    }
    return r;
}

int Putc(int tid, int channel, unsigned char ch) {

    char r;
    IOMessage m = {
        PUTC,
        &ch,
        1
    };

    int res = Send(tid, (char*)&m, sizeof(IOMessage), &r, 1);
    if (res < 0) {
        return -1;
    }
    // TODO: do something with reply

    return 0;
}


int Puts(int tid, int channel, unsigned char* ch) {

    char r;
    IOMessage m = {
        PUTS,
        ch,
        str_len(ch)
    };

    int res = Send(tid, (char*)&m, sizeof(IOMessage), &r, 1);
    if (res < 0) {
        return -1;
    }
    // TODO: do something with reply

    return 0;
}

int Puts_len(int tid, int channel, unsigned char* ch, int len) {

    char r;
    IOMessage m = {
        PUTS,
        ch,
        len
    };

    int res = Send(tid, (char*)&m, sizeof(IOMessage), &r, 1);
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
