#ifndef _rpi_h_
#define _rpi_h_

#include <stdint.h>
#include <stddef.h>
#define DEBUG 0

// Serial line 1 on the RPi hat is used for the console; 2 for Marklin
#define CONSOLE 1
#define MARKLIN 2

#define COUNTER_PER_TENTH_SECOND 100000

void gpio_init();
void uart_config_and_enable(size_t line);
void uart_enable_tx(size_t line);
void uart_enable_rx(size_t line);
void uart_enable_cts(size_t line);

void uart_disable_tx(size_t line);
void uart_disable_rx(size_t line);
void uart_disable_cts(size_t line);

void uart_clear_tx(size_t line);
void uart_clear_rx(size_t line);
void uart_clear_cts(size_t line);

uint32_t uart_ifls(size_t line);
uint32_t uart_get_interrupt_lines();
uint32_t uart_irq_id(size_t line);
uint32_t uart_mis(size_t line);
uint32_t uart_mis_tx(size_t line);
uint32_t uart_mis_rx(size_t line);
uint32_t uart_mis_cts(size_t line);
uint32_t uart_cts(size_t line);

int uart_can_read(size_t line);
int uart_can_write(size_t line);

// non-blocking
unsigned char uart_readc(size_t line);
int uart_writec(size_t line, unsigned char c);

unsigned char uart_getc(size_t line);
void uart_putc(size_t line, unsigned char c);
void uart_putl(size_t line, const char *buf, size_t blen);
void uart_puts(size_t line, const char *buf);
void uart_printf(size_t line, char *fmt, ...);
void uart_dprintf(size_t line, char *fmt, ...);

#endif /* rpi.h */
