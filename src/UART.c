#include "UART.h"

void uart0_Init(unsigned int UBRR) { // Baud Rate Register
  UCSR0A = (1 << U2X0);  // double speed
  UCSR0B |= (1 << RXEN0) | (1 << TXEN0); // enable receive and trasmit
  UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); // 8 bit symboler kommunikeres
  UBRR0H = (unsigned char)(UBRR >> 8);
  UBRR0L = (unsigned char) UBRR; // vi bruger 2 bytes til at gemme hele UART pakken
}

void putchUSART0(char tx) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait for empty buffer
    UDR0 = tx;
}

char getchUSART0(void) {
    while (!(UCSR0A & (1 << RXC0))); // Wait for data
    return UDR0;
}

// Function to send a whole string back to Serial Monitor
void printString(const char* s) {
    while (*s) {
        putchUSART0(*s++);
    }
}