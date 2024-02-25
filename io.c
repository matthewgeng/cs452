#include "io.h"
#include "syscall.h"
#include "rpi.h"
#include "nameserver.h"
#include "util.h"
#include "char_cb.h"
#include "clock.h"
#include <stdarg.h>

// notifier needed to still allow function calls to server with buffering
void console_out_notifier() {
    RegisterAs("cout_notifier");

    int cout = WhoIs("cout");

    // int clock = WhoIs("clock");
    MessageStr m = {"", 0};
    for(;;){
        // Delay(clock, 1000);
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

    for(;;){
        int res = AwaitEvent(CONSOLE_RX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }

        res = Send(cin, NULL, 0, NULL, 0);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console in notifier\r\n");
        }
    }
}

void console_out() {

    RegisterAs("cout");

    int cout_notifier = WhoIs("cout_notifier");

    int tid;
    // circular buffer 
    // TODO: make constant a variable
    char raw_buffer[512];
    CharCB buffer;
    initialize_charcb(&buffer, raw_buffer, 200);
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

            // reply to notifier
            Reply(tid, NULL, 0);
        // store data in buffer
        } else {
            // uart_printf(CONSOLE, "message length from user %u\r\n", m.len);

            // store data in buffer
            for (uint32_t i = 0; i < m.len; i++) {
                push_charcb(&buffer, m.str[i]);
            }

            // try to flush buffer 
            while (uart_can_write(CONSOLE) && !is_empty_charcb(&buffer)) {
                uart_writec(CONSOLE, pop_charcb(&buffer));
            }

            // reply to sender
            Reply(tid, NULL, 0);
        }
    }
}

void console_in() {
    RegisterAs("cin");

    int cin_notifier = WhoIs("cin_notifier");

    int tid;
    // circular buffer 
    // message struct
    
    char c;

    for(;;) {
        Receive(&tid, &c, 1);

        // print
        if (tid == cin_notifier) {
            // try to flush buffer 
            if (uart_can_read(CONSOLE)) {
                // data = uart_readc(CONSOLE);
                // reply to notifier

            }

        } else {


        }
    }
}

// notifier needed to still allow function calls to server with buffering
void marklin_out_notifier() {
    RegisterAs("mout_notifier");

}

// notifier needed to still allow function calls to server with buffering
void marklin_in_notifier() {
    RegisterAs("min_notifier");
    
}

void marklin_io() {
    RegisterAs("mio");

}


// TODO: channel is not being used. Should it?

int Getc(int tid, int channel) {
    char r;
    int res = Send(tid, NULL, 0, &r, 1);
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
	char bf[12];
	char ch;
    int len;

    char buffer[128];
    int i = 0;

    while ( ( ch = *(fmt++) ) && i < sizeof(buffer)) {
		if ( ch != '%' ){
            buffer[i] = ch;
            i++;
		} else {
			ch = *(fmt++);
			switch( ch ) {
                case 'u':
                    len = ui2a( va_arg( va, unsigned int ), 10, bf );
                    memcpy(buffer + i, bf, len);
                    i+= len;
                    break;
                case 'd':
                    len = i2a( va_arg( va, int ), bf );
                    memcpy(buffer + i, bf, len);
                    i+= len;
                    break;
                case 'x':
                    len = ui2a( va_arg( va, unsigned int ), 16, bf );
                    memcpy(buffer + i, bf, len);
                    i+= len;
                    break;
                case 's':
                    char* s = va_arg(va, char*);
                    len = str_len(s);

                    memcpy(buffer + i, s, len);
                    i+= len;
                    break;
                case '%':
                    buffer[i] = ch;
                    i++;
                    break;
                case '\0':
                    buffer[i] = ch;
                    i++;
                    uart_printf(CONSOLE, buffer);
                    return;
			}
        }
	}
    if (i >= sizeof(buffer)) {
        printf(tid, channel, "Printf was > 128 bytes\r\n");
        return;
    } else {
        buffer[i] = '\0';
        uart_printf(CONSOLE, buffer);
        // Puts(tid, channel, buffer);
    }
}

void printf(int tid, int channel, char *fmt, ... ) {
	va_list va;
	va_start(va,fmt);
	format_print( tid, channel, fmt, va );
	va_end(va);
}
