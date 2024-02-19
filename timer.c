#include "timer.h"
#include "rpi.h"

/*********** SYSTEM TIMERS ************************ ************/
static char* const ST_BASE = (char*)(0xFE003000); // todo use MMIO_BASE

static const uint32_t ST_UPDATE_FREQ = 1000000; // given frequency

// system timer register offsets
static const uint32_t ST_CS = 0x00;
static const uint32_t ST_CLO = 0x04;
static const uint32_t ST_CHI = 0x08;
static const uint32_t ST_CO = 0x0c;
static const uint32_t ST_C1 = 0x10;
static const uint32_t ST_C2 = 0x14;
static const uint32_t ST_C3 = 0x18;

uint32_t st_lo() {
    uint32_t lo;
    lo = *(uint32_t*)(ST_BASE + ST_CLO);
    return lo;
}

uint32_t sys_time() {
    return st_lo();
}

uint32_t sys_time_ms(uint32_t sys_time) {
    return sys_time / ((ST_UPDATE_FREQ/1000));
}

uint32_t sys_time_tenth_seconds(uint32_t sys_time) {
    return sys_time / ((ST_UPDATE_FREQ/10));
}

void parse_st(uint32_t sys_time, uint32_t* minutes, uint32_t* seconds, uint32_t* tenth_seconds) {
    uint32_t total_tenth_seconds = sys_time_tenth_seconds(sys_time);
    
    *minutes = (uint32_t)(total_tenth_seconds / 600);
    *seconds = ((uint32_t)(total_tenth_seconds % 600)) / 10;
    *tenth_seconds = (uint32_t)(total_tenth_seconds % 10);
}

void timer_init() {
    uint32_t cur = sys_time();
    // uart_printf(CONSOLE, "cur time %d\r\n", cur);
    cur += INTERVAL;
    *(uint32_t*)(ST_BASE + ST_C1) = cur;
    // uart_printf(CONSOLE, "c0 time %d\r\n", *(uint32_t*)(ST_BASE + ST_CO));
    // uart_printf(CONSOLE, "c1 time %d\r\n", *(uint32_t*)(ST_BASE + ST_C1));
    // uart_printf(CONSOLE, "c2 time %d\r\n", *(uint32_t*)(ST_BASE + ST_C2));

    // uart_printf(CONSOLE, "cs %d\r\n", *(uint32_t*)(ST_BASE));
}

void update_c1(){
    *(uint32_t*)(ST_BASE+ST_C1) = *(uint32_t*)(ST_BASE+ST_C1) + INTERVAL;
}

void update_status_reg(){
    *(uint32_t*)(ST_BASE) = *(uint32_t*)(ST_BASE) | 1 << 1;
}