#include "exception.h"

void init_exception_vector() {
    asm volatile(
        "adr x0, exception_vector\n"
        "msr vbar_el1, x0\n"
    );
}
