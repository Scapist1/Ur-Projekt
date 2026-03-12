#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "I2C.h"     // i2c driver
#include "ssd1306.h" // display driver
#include <avr/interrupt.h>
#include <utils.h>
#include <stdint.h> // til at lave 8-bit ints
#define BAUD 115200
#define UART_FULL_DUPLEX (F_CPU/8/BAUD) -1
#define UART_HALF_DUPLEX (F_CPU/16/BAUD) -1



void UART_0_init(unsigned int UBRR){ //duplex mode? eller hvad
    UCSR0A = (1 << U2X0);                   // Control Status Register: Sæt Double baud rate
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);   // Enable receive og transmit
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); // Bit 1,3,4,5,6,7 = 0 pr. default, altså ingen clk, ansynkron, uden parity, 1 bit stop.
    // De to USCZ00 og 01 = 1 samt UCSR0B UCSZ02 = 0, giver 8 bit character size (data)
    UBRR0H = (unsigned char)(UBRR >> 8);
    UBRR0L = (unsigned char)UBRR; // vi bruger 2 bytes til at gemme hele UART pakken

}

void putchUSART0(char tx)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
           // wait for empty transmit buffer
    };
    UDR0 = tx; // skriver byten til data registeret
}

void putChar( char* ptr){
    while (*ptr != '\0'){ //explicit på null terminator selvom while(*ptr) nok også ville virke.
        putchUSART0(*ptr);
        ptr++;
    }
}

char getchUSART0(void)
{
    while (!(UCSR0A & (1 << RXC0)))
        ; // wait until a character is received
    return UDR0;
}


void readString(char *buffer, uint8_t maxlength){
    uint8_t i = 0;

    while (i < maxlength -1){
        char c = getchUSART0();
        if (c == '\n' || c == '\r'){
            break;
        }
        buffer[i++] = c; //incrementer efter 
    }
    buffer[i] = '\0'; // null terminér
}   

void main(void)
{
    display_init();
    UART_0_init(UART_FULL_DUPLEX);
    putChar("test\n");

    char wordtoprint[10];

    while (1)
    {
        readString(wordtoprint, 10);
        putChar("You wrote: ");
        putChar(wordtoprint);
        putchUSART0('\n');

        for (size_t i = 0; i < 10 && wordtoprint[i] != '\0'; i++)
        {
            sendCharXY(wordtoprint[i], 2, 2 + i);
        }
        
    }
}
