#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"
#include "timer.h"
#include "utils.h"

void ur() {
    ss++;
    if (ss >= 60) { ss = 0; mm++; }
    if (mm >= 60) { mm = 0; hh++; }
    if (hh >= 24) { hh = 0; }
}

int main(void) {
    button_init();
    display_init();
    uart0_Init(16); // 115200 baud
    DDRB |= (1 << DDB7); // Sæt Pin 13 (LED) som output, bare for at have et visuelt timing output
    
    timer1_init(); // starter hardware timeren
    sei();  // aktivere interrupts

    printString("\e[2J\e[H"); // Ryd monitor
    printString("ur projekt\r\n");
    printString("   [ skriv ny tid ind i formatet: tt:mm:ss (f.eks. 12:30:00) ]\r\n");
    printString("       tid:\r\n");

    char display_str[20];

    while (1) {

        if  (ss_flag) {
            cli(); //disabler interrupts kort så ikk vi ved uheld kører et interrupt midt i 16 bit tallet (læses ad 2x8bit)
            ms -= 1000;    // hvis vi trækker 1000 fra frem for at reset til 0, så risikere vi ikke at tabe tid

            if (ms < 1000)  {
                ss_flag = 0;
            }
            sei(); //enabler interrupts igen
            
            ur();   // ss++ og logik til ss, mm, hh tæller
            
            PORTB ^= (1 << PORTB7); // XOR skifter bit 7 (Toggles LED) 

            sprintf(display_str, "%02d:%02d:%02d", hh, mm, ss);    // Display update
            sendStrXY(display_str, 4, 4);

            printString("\e[s"); // Gem markør
            sprintf(display_str, "\e[3;14H\e[K%02d:%02d:%02d", hh, mm, ss);
            printString(display_str);
            printString("\e[5;0H"); // det der skrives starter under uret
        }

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
        }
        ny_data_klar = 0;       
    }
}
