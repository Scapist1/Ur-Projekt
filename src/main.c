#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"

#define F_CPU 16000000UL
#define BAUD 115200
#define MYUBRRF (F_CPU / 8 / BAUD - 1)


// Funktion til at sende streng til seriel monitor (printer guide besked i starten)
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
    
    printString("Guide: Tryk på arduino knap for at indstille tiden i seriel monitoren\r\n");

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
        else if (pos < 16 && c >= 32 && c <= 126) { // godtager kun ASCII tegn som kan vises på display og begrænser pos til højest 15 tegn så displayet ikke flyder over med tegn
            buffer[pos++] = c;  // gemmer 8-bit char c i buffer array og tilføjer 1 til int pos tæller
            putchUSART0(c); // ekkoer tegn tilbage til serial monitoren, så vi kan se hvad vi skriver i den
        }
    }
}