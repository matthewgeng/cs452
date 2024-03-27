#ifndef TC2DEMO_H
#define TC2DEMO_H

#include <stdint.h>

typedef enum {
    DEMO_START,
    DEMO_NAV_END,
    DEMO_NAV_RETRY
} demo_arg_type;

typedef struct DemoMessage{
    demo_arg_type type;
    uint32_t arg1;
    uint32_t arg2;
} DemoMessage;


void tc2demo();

#endif
