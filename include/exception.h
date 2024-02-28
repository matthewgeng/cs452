#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdint.h>

extern void exception_vector(void);
void init_exception_vector();
void invalid_exception(uint32_t exception);
#endif
