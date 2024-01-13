
#ifndef _cb_h_
#define _cb_h_ 1

#include <stddef.h>
#include <stdint.h>


uint32_t incrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t decrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t charToBuffer(size_t line, char *buf, uint32_t bufEnd, char c);
uint32_t strToBuffer(size_t line, char *buf, uint32_t bufEnd, char *str);
uint32_t printfToBuffer(size_t line, char *buf, uint32_t bufEnd, char *fmt, ... );

#endif /* cb.h */