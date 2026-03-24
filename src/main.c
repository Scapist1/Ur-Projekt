#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"

volatile int hh = 0, mm = 0, ss = 0;

void ur() {
    ss++;
    if (ss >= 60) { ss = 0; mm++; }
    if (mm >= 60) { mm = 0; hh++; }
    if (hh >= 24) { hh = 0; }
}

int main(void) {
    I2C_Init();
    InitializeDisplay();
    clear_display();
    uart0_Init(16); // 115200 baud
    
    sei(); 

    printString("\r\n--- UR SYSTEM KLAR ---\r\n");
    printString("Format: tt:mm:ss (f.eks. 12:30:00)\r\n");

    char display_str[20];
    char debug_msg[50];

    while (1) {
        
        // Tjek om der er modtaget data fra UART.c
        if (ny_data_klar) {
        int h, m, s;
        int fundet = sscanf((char*)rx_buffer, "%d:%d:%d", &h, &m, &s);  // returnere med 3, hvis den successfuldt har fundet 3 heltal, hvis bufferen indeholder korrekt tidsformat

        if (fundet == 3) {  
            if (h < 24 && m < 60 && s < 60) {   // hvis tallene passer inden for rammerne af et døgn, så indstilles tiden ellers fejlmelding
                hh = h; mm = m; ss = s;
                printString("\r\nOK: Tid indstillet!\r\n");
            } else {
                printString("\r\nFEJL: Ugyldige tal!\r\n");
            }
        } else {
            printString("\r\nFEJL: Format skal være tt:mm:ss\r\n");
        }
        _delay_ms(50); // Lille pause for at undgå bounce, især delay mellem /r og /n når man trykker enter i monitor er vigtigt
        ny_data_klar = 0; 
        }

        sprintf(display_str, "Tid: %02d:%02d:%02d", hh, mm, ss);    // Display update
        sendStrXY(display_str, 4, 1);
        _delay_ms(1000);    // 1000 ms delay, må skulle optimeres, da denne løsning slet ikke giver et ur der er præcist
        ur();            // tæller 1 sek. frem på uret
    }
}