#include "rpi.h"

void assert(const char* str) {
    uart_printf(CONSOLE, str);
    for (;;){}
}
