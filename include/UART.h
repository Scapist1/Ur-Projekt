#ifndef UART_H_
#define UART_H_

#include <avr/io.h>

void uart0_Init(unsigned int ubrr);
void putchUSART0(char tx);
char getchUSART0(void);
void printString(const char* s);

#endif