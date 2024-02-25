#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>

typedef struct MessageStr {
    char* str;
    uint32_t len;
} MessageStr;

int Getc(int tid, int channel);
int Putc(int tid, int channel, unsigned char ch);
int Puts(int tid, int channel, unsigned char* ch);
void printf(int tid, int channel, char *fmt, ... );

// notifier needed to still allow function calls to server with buffering
void console_out_notifier();
void console_in_notifier();
void console_out();
void console_in();

// notifier needed to still allow function calls to server with buffering
void marklin_out_notifier();
void marklin_in_notifier();
// only 1 marklin io task because it's half-duplex
void marklin_io();

#endif
