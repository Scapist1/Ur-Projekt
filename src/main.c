#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "I2C.h"
#include "ssd1306.h"

#define BAUD 115200
#define MYUBRRF (F_CPU / 8 / BAUD - 1)

void uart0_Init(unsigned int ubrr) {
    UCSR0A = (1 << U2X0); // Double speed mode
    UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable RX and TX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data format
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
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

int main(void) {
    char buffer[17];    // 16 chars + null terminator
    uint8_t pos = 0;

    I2C_Init();
    InitializeDisplay();
    clear_display();

    uart0_Init(MYUBRRF);
    
    printString("Systemet er klar: Skriv og print string\r\n");

    while (1) {
        char c = getchUSART0(); // get char via UART

        // terminere streng og læser til display når c er et Carriage Return \r eller Line Feed \n tegn
        if (c == '\r' || c == '\n') { 
            if (pos > 0) { 
                buffer[pos] = '\0'; // Null-terminate the string
                
                clear_display();
                sendStrXY(buffer, 0, 0);
                
                printString("\r\n[OLED Updated]\r\n");
                
                pos = 0;  // reset position til næste linje der skal skrives til Display
            }
        } 
        // Fjerner backspace symbol fra strengen, hvis c er et backspace \b (ASCII 8 eller 127 både gammelt og nyt backspace tegn)
        else if ((c == 0x08 || c == 0x7F) && pos > 0) {
            pos--;  // tæller 1 tilbage på positionen
            printString("\b \b"); // ryk skrivemarkør tilbage, print space, ryk skrivemarkør tilbage igen
        }

        // gemmer c i buffer array, hvis det opfylder specifikke kriterier
        else if (c >= 32 && c <= 126) { // godtager kun ASCII tegn som kan vises på display
            buffer[pos++] = c;  // gemmer 8-bit char c i buffer array og tilføjer 1 til int pos tæller
            putchUSART0(c); // ekkoer tegn tilbage til serial monitoren, så vi kan se hvad vi skriver i den
        }
    }
}