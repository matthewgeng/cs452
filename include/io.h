#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>


typedef enum IOType {
    GETC = 1,
    PUTC,
    PUTS
} IOType;

typedef struct IOMessage {
    IOType type;
    uint32_t len;
    char str[256];
} IOMessage;

int Getc(int tid, int channel);
int Putc(int tid, int channel, unsigned char ch);
int Puts(int tid, int channel, unsigned char* ch);
int Puts_len(int tid, int channel, unsigned char* ch, int len);
void printf(int tid, int channel, char *fmt, ... );

// notifier needed to still allow function calls to server with buffering
void console_out_notifier();
void console_in_notifier();
void console_out();
void console_in();

// notifier needed to still allow function calls to server with buffering
void marklin_out_tx_notifier();
void marklin_out_cts_notifier();
void marklin_in_notifier();
// only 1 marklin io task because it's half-duplex
void marklin_io();

#endif
