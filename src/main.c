#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>       // Nødvendig for sprintf 
#include "I2C.h"
#include "ssd1306.h"

#define BAUD 115200
#define MYUBRRF F_CPU/8/BAUD -1 //full duplex

void uart1_Init(unsigned int UBRR) { // Baud Rate Register
  UCSR0A = (1 << U2X0);  // double speed
  UCSR0B |= (1 << RXEN0) | (1 << TXEN0); // enable receive and trasmit
  UCSR0C |= (1 << UVSZ00) | (1 << UCSZ01); // 8 bit symboler kommunikeres
  UBRR0H = (unsigned char)(UBRR >> 8);
  UBRR0L = (unsigned char) UBRR; // vi bruger 2 bytes til at gemme hele UART pakken
}

char getchUSART0(void)  {
  while(!(UCSR0A & (1 << RXC0))); // wait until a character is received
  return UDR0;
}

char putchUSART0 (char tx) {
  while(!(UCSR0A & (1 << UDRE0)));  // wait for wait for empty transmit buffer
  UDR0 = tx;
}

int main()  {
  char name[10];


  // Initialisering
  I2C_Init();
  InitializeDisplay();
  clear_display(); 


  
  uart1_Init(MYUBRRF);
  
  fgets(name, sizeof(name), stdin);

  while(*name)  {
    putchUSART0(*name);
    name++; // forskyder addressen den læser til
  }

  while(1) {
    putchUSART0(getchUSART0()); // transmit what is received
    _delay_ms(1000);
  }

}

