#ifndef _rpi_h_
#define _rpi_h_ 1

#include <stdint.h>
#include <stddef.h>

// Serial line 1 on the RPi hat is used for the console; 2 for Marklin
#define CONSOLE 1
#define MARKLIN 2

#define COUNTER_PER_HUNDREDTH_SECOND 10000
#define OUTPUT_BUFFER_SIZE 5000
#define INPUT_BUFFER_SIZE 20
#define TRAIN_BUFFER_SIZE 200
#define SENSOR_BUFFER_SIZE 12

#define SWITCHES_ROW 3
#define SENSORS_ROW 8
#define INPUT_ROW 10
#define LAST_FUNCTON_ROW 11
#define FUNCTION_RESULT_ROW 12

void gpio_init();
void uart_config_and_enable(size_t line);
unsigned char uart_getc(size_t line);
void uart_putc(size_t line, unsigned char c);
void uart_putl(size_t line, const char *buf, size_t blen);
void uart_puts(size_t line, const char *buf);
void uart_printf(size_t line, char *fmt, ...);

uint32_t charToRegBuffer(char *buf, uint32_t bufEnd, char c);

unsigned int polling_uart_putc(size_t line, unsigned char c);
unsigned char polling_uart_getc(size_t line);

uint16_t readRegisterAsUInt16(char *base, uint32_t offset);
uint32_t readRegisterAsUInt32(char *base, uint32_t offset);
void timerSetup();
// void timerUpdate();

#endif /* rpi.h */
