#include "exception.h"
#include "rpi.h"

void init_exception_vector() {
    asm volatile(
        "adr x0, exception_vector\n"
        "msr vbar_el1, x0\n"
    );
}

void invalid_exception(uint32_t exception) {
    uart_printf(CONSOLE, "Invalid exception %u", exception);
    for (;;) {}
}
