#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h> // Nødvendig for ISR og sei()
#include "I2C.h"
#include "ssd1306.h"
#include "UART.h"

#define F_CPU 16000000UL
#define BAUD 115200
#define MYUBRRF (F_CPU / 8 / BAUD - 1)
#define SET_INPUT(DDRx, BIT)  (DDRx &= ~(1 << BIT))
#define SETBIT(ADDR, BIT) ((ADDR) |= (1 << (BIT))) // PINn

// Globale variabler
char buffer[17];    
uint8_t pos = 0;
volatile uint8_t ss = 0, mm = 0, hh = 0; // volatile da de ændres i ISR
char pre_c = 0; // Gemmer forrige tegn for at teste for \r\n

// Variabler til den "gemte" tid, som venter på at blive aktiveret
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
    sendStrXY("ny tid: hh:mm:ss", 2, 0);
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

        if (pre_c == '\r' && c == '\n') {
            buffer[pos] = '\0';
            
            int h, m, s;
            if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) == 3) {
                if (h < 24 && m < 60 && s < 60) {
                    // Vi gemmer tiden uden at skifte tekst i toppen
                    ny_hh = h;
                    ny_mm = m;
                    ny_ss = s;

                    // NY OPDATERING: Opdater kun den gemte tid på OLED, når den faktisk modtages
                    char oled_nytid[20];
                    sprintf(oled_nytid, "%02d:%02d:%02d", ny_hh, ny_mm, ny_ss);
                    sendStrXY(oled_nytid, 2, 8);
                }
            } 
            // Alle printString og clear_display herfra er fjernet for at holde layoutet statisk
            
            pos = 0; 
        } 
        else if ((c == 0x08 || c == 0x7F) && pos > 0) {
            pos--;
            printString("\b \b");
        }
        else if (pos < 16 && c >= 32 && c <= 126) {
            buffer[pos++] = c;
            putchUSART0(c);
        }
        
        pre_c = c;
    }
}

void ur() {
    // Vi gemmer hvad ss var sidste gang, så vi ved hvornår det ændrer sig
    static uint8_t sidste_ss = 255; 
    
    // Hvis knappen er trykket, initialiserer vi hhen til den gemte tid
    if (knap_trykket) {
        cli(); // Midlertidig stop for interrupts så tiden ikke ændrer sig mens vi skriver
        hh = ny_hh;
        mm = ny_mm;
        ss = ny_ss;
        sei(); 
        knap_trykket = 0; // Nulstil knap-flaget
    }

    // OPDATER KUN SKÆRMEN HVIS SEKUNDET HAR ÆNDRET SIG
    if (ss != sidste_ss) { 
        char tid_buffer[40];
        char nytid_buffer[40];
        
        // 1. Vis den nutidige hh i terminalen (Linje 2)
        sprintf(tid_buffer, "\e[3;1H\e[K      tid: %02d:%02d:%02d", hh, mm, ss);
        printString(tid_buffer);
        
        // 2. Vis den gemte tid i terminalen (Linje 3)
        sprintf(nytid_buffer, "\e[2;1H\e[K   ny tid: %02d:%02d:%02d", ny_hh, ny_mm, ny_ss);
        printString(nytid_buffer);
        
        // OLED opdatering:
        // Vi viser kun den aktive tid her i loopet (Række 4)
        sendStrXY(tid_buffer + 19, 4, 7); 

        sidste_ss = ss; // Opdater sidste_ss så vi ikke skriver til skærmen før næste sekund
    }
}

int main(void) {
    Init(); // initialisere alt! (I2C Display, UART, Knap)
    
    printString("\e[2J\e[H"); // Ryder monitor for gamle skriverier
    _delay_ms(10); 
    
    printString("Skriv ny tid hh:mm:ss i monitor. Tryk knap for at ændre til ny tid\r\n");  // guide tekst vist låst øverst i monitor

    while (1) {
        UART(); 
        ur(); 
    }
}