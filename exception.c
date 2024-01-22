#include "exception.h"

/* TODO: 
void get_el() {
    asm(

    );
}
*/

void init_exception_vector() {
    asm(
        "adr x0, exception_vector\n"
        "msr vbar_el1, x0\n"
    );
}
