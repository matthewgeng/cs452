
#ifndef _cb_h_
#define _cb_h_ 1

#include <stddef.h>
#include <stdint.h>


uint32_t incrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t decrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t charToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char c);
uint32_t strToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char *str);
uint32_t printfToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char *fmt, ... );

#endif /* cb.h */