#ifndef _rpi_h_
#define _rpi_h_ 1

#include <stdint.h>
#include <stddef.h>

// Serial line 1 on the RPi hat is used for the console; 2 for Marklin
#define CONSOLE 1
#define MARKLIN 2

#define COUNTER_PER_TENTH_SECOND 100000
#define OUTPUT_BUFFER_SIZE 5000
#define INPUT_BUFFER_SIZE 20
#define TRAIN_BUFFER_SIZE 5000
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

uint32_t incrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t decrementBufEnd(uint32_t bufEnd, uint32_t bufSize);
uint32_t charToRegBuffer(char *buf, uint32_t bufEnd, char c);
uint32_t charToBuffer(size_t line, char *buf, uint32_t bufEnd, char c);
uint32_t strToBuffer(size_t line, char *buf, uint32_t bufEnd, char *str);
uint32_t printfToBuffer(size_t line, char *buf, uint32_t bufEnd, char *fmt, ... );

unsigned int polling_uart_putc(size_t line, unsigned char c);
unsigned char polling_uart_getc(size_t line);

void clearConsole();
uint16_t readRegisterAsUInt16(char *base, uint32_t offset);
uint32_t readRegisterAsUInt32(char *base, uint32_t offset);
void timerSetup();
// void timerUpdate();

#endif /* rpi.h */
