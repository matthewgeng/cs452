#include "io.h"
#include "syscall.h"
#include "rpi.h"
#include "nameserver.h"
#include "util.h"
#include "char_cb.h"

// notifier needed to still allow function calls to server with buffering
void console_out_notifier() {
    RegisterAs("cout_notifier");

    int cout = WhoIs("cout");

    for(;;){
        int res = AwaitEvent(CONSOLE_TX);
        if (res < 0) {
            // TODO: make more robust
            uart_printf(CONSOLE, "ERROR console out notifier\r\n");
        }

        res = Send(cout, NULL, 0, NULL, 0);
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
    char raw_buffer[200];
    CharCB buffer;

    // message struct
    char* s;

    for(;;) {
        Receive(&tid, s, 1);

        // print
        if (tid == cout_notifier) {
            // try to flush buffer 


            // reply to notifier

        // store data in buffer
        } else {

            // if putc
                // store data in buffer

                // try to flush buffer 

                // reply to sender

            // if puts
                // store all data in buffer

                // try to flush buffer 

                // reply to sender
        }
    }
}

void console_in() {
    RegisterAs("cin");

    int cout_notifier = WhoIs("cin_notifier");

    int tid;
    // circular buffer 
    // message struct
    

    for(;;) {
        // Receive(&tid, &(message struct), 1);

        // print
        if (tid == cout_notifier) {
            // try to flush buffer 


            // reply to notifier

        // store data in buffer
        } else {

            // if putc
                // store data in buffer

                // try to flush buffer 

                // reply to sender

            // if puts
                // store all data in buffer

                // try to flush buffer 

                // reply to sender
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
