#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

// conversions
int a2d(char ch);
char a2i(char ch, char **src, int base, int *nump);
unsigned int ui2a(unsigned int num, unsigned int base, char *bf);
unsigned int i2a(int num, char *bf);
unsigned int ui2a_no0( unsigned int num, unsigned int base, char *bf );

// memory
void *memset(void *s, int c, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);

// string
void str_cpy(char* dest, char* src);
void str_cpy_w0(char* dest, char* src);
int str_len(const char *str);
int str_equal(const char *c1, const char *c2);

unsigned int getArgumentTwoDigitNumber(char *src);
unsigned int getArgumentThreeDigitNumber(char *src);

#endif
