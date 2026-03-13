#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h> // Nødvendig for ISR og sei()
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#define BAUD 115200
#define MYUBRRF (F_CPU / 8 / BAUD - 1)
#define SET_INPUT(DDRx, BIT)  (DDRx &= ~(1 << BIT))
#define SETBIT(ADDR, BIT) ((ADDR) |= (1 << (BIT))) // PINn

// Globale variabler
char buffer[17];    
uint8_t pos = 0;
volatile uint8_t ss = 0, mm = 0, hh = 0; // volatile da de ændres i ISR
char pre_c = 0; // Gemmer forrige tegn for at teste for \r\n

// Variabler til den "nye gemte" tid, som venter på at blive aktiveret
uint8_t ny_hh = 0, ny_mm = 0, ny_ss = 0;
volatile uint8_t knap_trykket = 0; 

// Interrupt Service Routine for knappen på Pin 2 (INT4)
ISR(INT4_vect) {
    knap_trykket = 1; // Signalér at hhen skal initialiseres til den gemte tid
}

// NY PRÆCIS TIMER: Denne køre præcis hvert sekund via hardware
ISR(TIMER1_COMPA_vect) {
    ss++;
    if (ss >= 60) { ss = 0; mm++; }
    if (mm >= 60) { mm = 0; hh++; }
}

void Init() {
    I2C_Init();
    InitializeDisplay();
    clear_display();
    uart0_Init(MYUBRRF);

    /* Knappe settings! */
    SET_INPUT(DDRE, 4); // Button(2): PE4 sættes til input
    SETBIT(PORTE, 4);    // Button(2): PE4 aktiveres intern pull-up

    // Konfigurer INT4 til "Falling Edge" (når knappen trykkes ned mod stel)
    EICRB |= (1 << ISC41);
    EICRB &= ~(1 << ISC40);

    // Aktiver External Interrupt Request 4
    EIMSK |= (1 << INT4);

    /* Hardware Timer 1 indstillinger (1 sekund) */
    TCCR1A = 0; 
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A = 15624;            // 16MHz / (1024 * 1Hz) - 1
    TCCR1B |= (1 << WGM12);   // CTC mode
    TCCR1B |= (1 << CS12) | (1 << CS10); // 1024 prescaler
    TIMSK1 |= (1 << OCIE1A);  // Aktiver timer interrupt

    sei(); // Aktiver interrupts globalt

    // Display info
    sendStrXY("Ur Projekt", 0, 3);
    sendStrXY("ny tid: 00:00:00", 2, 0);
    sendStrXY("   tid: 00:00:00", 4, 0);
}

void printString(const char* s) {
    while (*s) {
        putchUSART0(*s++);
    }
}

void UART() {
    if (UCSR0A & (1 << RXC0)) {
        char c = UDR0;

        // Tjek for Enter (\r\n)
        if (pre_c == '\r' && c == '\n') {
            buffer[pos] = '\0';
            
            int h, m, s;
            if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) == 3) {
                if (h < 24 && m < 60 && s < 60) {
                    ny_hh = h; ny_mm = m; ny_ss = s;

                    // Opdater OLED
                    char oled_nytid[20];
                    sprintf(oled_nytid, "%02d:%02d:%02d", ny_hh, ny_mm, ny_ss);
                    sendStrXY(oled_nytid, 2, 8);
                }
            } 
            
            // RYD MONITOREN OG TEGN LAYOUTET IGEN FOR AT UNDGÅ ROD
            printString("\e[2;9H");
            // printString("Skriv ny tid hh:mm:ss i monitor. Tryk knap for at aktivere.\r\n");
            
            char nytid[50];
            sprintf(nytid, "%02d:%02d:%02d\r\n", ny_hh, ny_mm, ny_ss);
            printString(nytid);
            printString("\e[4;1H\e[J");     // sletter alt fra linje 4 og ned
            
            pos = 0; 
        } 
        else if ((c == 0x08 || c == 0x7F) && pos > 0) {
            pos--;
            printString("\b \b");
        }
        else if (pos < 16 && c >= 32 && c <= 126) {
            buffer[pos++] = c;
            putchUSART0(c); // Echo tegnet direkte
        }
        
        pre_c = c;
    }
}

void ur() {
    static uint8_t sidste_ss = 255; 
    
    // Hvis knappen er trykket, initialiserer vi hhen til den gemte tid
    if (knap_trykket) {
        cli(); 
        hh = ny_hh;
        mm = ny_mm;
        ss = ny_ss;
        sei(); 
        knap_trykket = 0; 
    }

    // OPDATER KUN SKÆRMEN HVIS SEKUNDET HAR ÆNDRET SIG
    if (ss != sidste_ss) { 
        char tid_buffer[40];
        
        // 1. Gem markørposition, skriv tid, og hop tilbage
        printString("\e[s"); // Gem markør (Save position)
        sprintf(tid_buffer, "\e[3;9H\e[K%02d:%02d:%02d", hh, mm, ss);
        printString(tid_buffer);
        printString("\e[u"); // Gå tilbage til markør (Unsave/Restore position)
        
        // OLED opdatering:
        char oled_tid[20];
        sprintf(oled_tid, "%02d:%02d:%02d", hh, mm, ss);
        sendStrXY(oled_tid, 4, 8); 

        sidste_ss = ss; 
    }
}

int main(void) {
    Init(); // initialisere alt!
    
    printString("\e[2J\e[H"); // Ryd monitor
    _delay_ms(10); 
    
    printString("Skriv ny tid hh:mm:ss i monitor. Tryk knap for at aktivere.\r\n");
    printString("ny tid: 00:00:00\r\n");
    printString("   tid: 00:00:00\r\n");

    while (1) {
        UART(); 
        ur(); 
    }
}