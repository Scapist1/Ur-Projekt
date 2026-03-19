#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#define BAUD 115200
#define MYUBRRF (F_CPU / 8 / BAUD - 1)

#define SET_INPUT(DDRx, BIT)  (DDRx &= ~(1 << BIT))
#define SETBIT(ADDR, BIT) ((ADDR) |= (1 << (BIT)))

// --- Globale variabler (VIGTIGT: volatile når de ændres i ISR) ---
volatile char rx_buffer[17];    
volatile uint8_t rx_pos = 0;    
volatile char pre_c = 0;
volatile uint8_t ny_data_klar = 0; // Flag til at signalere færdig besked

volatile uint8_t ss = 0, mm = 0, hh = 0; 
volatile uint8_t knap_trykket = 0; 

// Gemt tid (venter på aktivering)
uint8_t ny_hh = 0, ny_mm = 0, ny_ss = 0;

// --- Interrupt Service Routines (ISR) ---

// 1. UART MODTAGE INTERRUPT (Kører ved hvert tegn)
ISR(USART0_RX_vect) {
    char c = UDR0; // Læs hardware register (nulstiller interrupt)

    // Tjek for Enter (\r\n)
    if (pre_c == '\r' && c == '\n') {
        rx_buffer[rx_pos] = '\0';
        ny_data_klar = 1; // Giv besked til main
        rx_pos = 0; 
    } 
    // Backspace håndtering
    else if ((c == 0x08 || c == 0x7F) && rx_pos > 0) {
        rx_pos--;
        putchUSART0('\b'); putchUSART0(' '); putchUSART0('\b');
    }
    // Almindelige tegn (printbare)
    else if (rx_pos < 16 && c >= 32 && c <= 126) {
        rx_buffer[rx_pos++] = c;
        putchUSART0(c); // Echo direkte tilbage til terminalen
    }
    pre_c = c;
}

// 2. KNAP INTERRUPT (INT4)
ISR(INT4_vect) {
    knap_trykket = 1; 
}

// 3. TIMER INTERRUPT (Hvert 1. sekund)
ISR(TIMER1_COMPA_vect) {
    ss++;
    if (ss >= 60) { ss = 0; mm++; }
    if (mm >= 60) { mm = 0; hh++; } 
}

// --- Hjælpefunktioner ---

void printString(const char* s) {
    while (*s) {
        putchUSART0(*s++);
    }
}

void Init() {
    I2C_Init();
    InitializeDisplay();
    clear_display();
    uart0_Init(MYUBRRF);

    // Aktiver UART Receive Interrupt
    UCSR0B |= (1 << RXCIE0);

    // Knap: PE4 som input med pull-up
    SET_INPUT(DDRE, 4);
    SETBIT(PORTE, 4);
    EICRB |= (1 << ISC41); // Falling edge
    EIMSK |= (1 << INT4);  // Enable INT4

    // Hardware Timer 1 (1 sekund CTC mode)
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC & 1024 Prescaler
    OCR1A = 15624;
    TIMSK1 |= (1 << OCIE1A);

    sei(); // Global interrupt enable

    sendStrXY("Ur Projekt", 0, 3);
    sendStrXY("ny tid: 00:00:00", 2, 0);
    sendStrXY("   tid: 00:00:00", 4, 0);
}

// Håndterer data fra UART bufferen når en hel linje er modtaget
void opdater_ny_tid() {
    if (ny_data_klar) {
        uint8_t h, m, s;
        
        // sscanf læser fra den volatile buffer
        if (sscanf((char*)rx_buffer, "%d:%d:%d", &h, &m, &s) == 3) {
            if (h < 24 && m < 60 && s < 60) {
                ny_hh = h; ny_mm = m; ny_ss = s;

                // Opdater OLED
                char oled_buf[20];
                sprintf(oled_buf, "%02d:%02d:%02d", ny_hh, ny_mm, ny_ss);
                sendStrXY(oled_buf, 2, 8);
                
                // Opdater Terminal
                printString("\e[2;9H");
                char term_buf[20];
                sprintf(term_buf, "%02d:%02d:%02d", ny_hh, ny_mm, ny_ss);
                printString(term_buf);
            }
        }
        ny_data_klar = 0; // Nulstil flag
    }
}

void ur_logik() {
    static uint8_t sidste_ss = 255; 
    
    if (knap_trykket) {
        cli(); // Stop interrupts kortvarigt mens vi sætter tiden
        hh = ny_hh; mm = ny_mm; ss = ny_ss;
        sei(); 
        knap_trykket = 0; 
    }

    if (ss != sidste_ss) { 
        char buf[20];
        // Opdater terminal (gemmer/henter markør position)
        printString("\e[s\e[3;9H");
        sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
        printString(buf);
        printString("\e[u");

        // Opdater OLED
        sendStrXY(buf, 4, 8); 
        sidste_ss = ss; 
    }
}

int main(void) {
    Init();
    
    printString("\e[2J\e[H"); // Ryd terminal
    printString("Indtast tid (hh:mm:ss) og tryk knap:\r\n");
    printString("ny tid: 00:00:00\r\n");
    printString("   tid: 00:00:00\r\n");

    while (1) {
        opdater_ny_tid(); 
        ur_logik(); 
    }
}