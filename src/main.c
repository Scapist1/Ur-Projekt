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
#include "externInt.h"

void ur() {
    ss++;
    if (ss >= 60) { ss = 0; mm++; }
    if (mm >= 60) { mm = 0; hh++; }
    if (hh >= 24) { hh = 0; }
}



uint8_t a_ss = 0, a_mm = 0, a_hh = 0;
int button_flag = 0;
int alarm_flag = 0;

int main(void) {
    button_init(); // init button
    extint4_init();
    display_init(); 
    uart0_Init(16); // 115200 baud
    DDRB |= (1 << DDB7); // Sæt Pin 13 (LED) som output, bare for at have et visuelt timing output
    DDRB |= (1 << DDB6);

    timer1_init(); // starter hardware timeren

    printString("\e[2J\e[H"); // ryd skærmen: escape karakter, clear screen ("[2J"), ryk cursor tilbage i venstre top hjørne ("[H")
    printString("ur projekt\r\n");
    printString("   [ skriv ny tid ind i formatet: tt:mm:ss (f.eks. 12:30:00) ]\r\n");
    printString("       tid:\r\n");

    char display_str[21];
    char a_display_str[21];
    char a_flag_str[10];
    char b_flag_str[10];
    char b_press_str[10];
    snprintf(a_display_str, sizeof(a_display_str), "%02d:%02d:%02d", a_hh, a_mm, a_ss); // undgå overflow

    sei(); //enabler interrupts
    
    while (1) {
        //sprintf(a_flag_str, "A-Flag: %d", alarm_flag);
        sprintf(b_flag_str, "%d", button_flag);
       // sprintf(b_press_str, "B-press: %d", button_pressed);
       // sendStrXY(a_flag_str, 0, 0);
        sendStrXY(b_flag_str, 6, 15);
       // sendStrXY(b_press_str, 2, 0);

        if  (ss_flag) {
            cli(); //disabler interrupts kort så ikk vi ved uheld kører et interrupt midt i 16 bit tallet (læses ad 2x8bit)
            ms -= 1000;    // hvis vi trækker 1000 fra frem for at reset til 0, så risikere vi ikke at tabe tid
            if (ms < 1000)  {
                ss_flag = 0;
            }
            sei(); //enabler interrupts igen
            
            ur();   // ss++ og logik til ss, mm, hh tæller
            
            PORTB ^= (1 << PORTB7); // XOR skifter bit 7 (Toggles LED) 

            snprintf(display_str, sizeof(display_str), "%02d:%02d:%02d", hh, mm, ss);    // Display update
            sendStrXY(display_str, 5, 4);

            printString("\e[s"); // Gem markør
            snprintf(display_str, sizeof(display_str), "\e[3;14H\e[K%02d:%02d:%02d", hh, mm, ss);
            printString(display_str);
            printString("\e[5;0H"); // det der skrives starter under uret

            if (hh == a_hh && mm == a_mm && ss == a_ss){
                alarm_flag = 1;
            }
        }

        if (alarm_flag){
            static uint16_t blink_counter = 0; //static så den ikk bliver nulstillet hvert loop.
            sendStrXY("ALARM!", 1, 5);
            if (ms < 500){
                sendStrXY(a_display_str, 2, 4);
                PORTB ^= (1 << PB6);
            }
            if (ms >= 500){
                sendStrXY("                     ", 2, 0); //clear display
                PORTB ^= (1 << PB6);
            }
            if (button_pressed){
                alarm_flag = 0;
                sendStrXY("             ", 1, 5);
                sendStrXY("                     ", 2, 0);
            }
        }

        if (button_pressed && !button_flag){
            button_flag = 1;
            button_pressed = 0;
            _delay_ms(200);
        }
        if (button_pressed && button_flag)
        {
            button_flag = 0;
            button_pressed = 0;
            _delay_ms(200);
        }
        // Tjek om der er modtaget data fra UART.c
        if (!button_flag && ny_data_klar)
        {
            int h, m, s;
            int fundet = sscanf((char*)rx_buffer, "%d:%d:%d", &h, &m, &s);  // returnere med 3, hvis den successfuldt har fundet 3 heltal, hvis bufferen indeholder korrekt tidsformat

            if (fundet == 3) {  
                if (h < 24 && h >= 0 && m < 60 && m >= 0 && s < 60 && s >= 0) {   // hvis tallene passer inden for rammerne af et døgn, så indstilles tiden ellers fejlmelding
                    hh = h; mm = m; ss = s;
                    printString("\r\nOK: Tid indstillet!\r\n");
                } else {
                    printString("\r\nFEJL: Ugyldige tal!\r\n");
                }
            } else {
                printString("\r\nFEJL: Format skal være tt:mm:ss\r\n");
            }
        }

        if (button_flag && ny_data_klar) //indstil alarm 
        {
            int h, m, s;
            int fundet = sscanf((char *)rx_buffer, "%d:%d:%d", &h, &m, &s); // returnere med 3, hvis den successfuldt har fundet 3 heltal, hvis bufferen indeholder korrekt tidsformat

            if (fundet == 3)
            {
                if (h < 24 && h >= 0 && m < 60 && m >= 0 && s < 60 && s >= 0)
                { // hvis tallene passer inden for rammerne af et døgn, så indstilles alarmtiden ellers fejlmelding
                    a_hh = h;
                    a_mm = m;
                    a_ss = s;
                    printString("\r\nAlarm indstillet!\r\n");
                    snprintf(a_display_str, sizeof(a_display_str), "%02d:%02d:%02d", a_hh, a_mm, a_ss); //gemmer en string med klokkeslettet
                    printString(a_display_str);
                }
                else
                {
                    printString("\r\nFEJL, alarm: Ugyldige tal!\r\n");
                }
            }
            else
            {
                printString("\r\nFEJL, alarm: Format skal være tt:mm:ss\r\n");
            }
        }
            ny_data_klar = 0;
        }
}
