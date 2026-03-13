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
uint8_t sekunder = 0, minutter = 0, timer = 0;
char pre_c = 0; // Gemmer forrige tegn for at teste for \r\n

// Variabler til den "gemte" tid, som venter på at blive aktiveret
uint8_t gemt_h = 0, gemt_m = 0, gemt_s = 0;
volatile uint8_t knap_trykket = 0; 

// Interrupt Service Routine for knappen på Pin 2 (INT4)
ISR(INT4_vect) {
    knap_trykket = 1; // Signalér at timeren skal initialiseres til den gemte tid
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

    sei(); // Aktiver interrupts globalt

    // Display info
    sendStrXY("Ur Projekt", 0, 3);
    sendStrXY("ny tid:", 2, 0);
    sendStrXY("   tid:", 4, 0);
    
}

void printString(const char* s) {
    while (*s) {
        putchUSART0(*s++);
    }
}

void tjekUART() {
    if (UCSR0A & (1 << RXC0)) {
        char c = UDR0;

        if (pre_c == '\r' && c == '\n') {
            buffer[pos] = '\0';
            
            int h, m, s;
            if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) == 3) {
                if (h < 24 && m < 60 && s < 60) {
                    // Vi gemmer tiden uden at skifte tekst i toppen
                    gemt_h = h;
                    gemt_m = m;
                    gemt_s = s;
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

void opdaterUr() {
    static unsigned long sidste_tik = 0;
    
    // Hvis knappen er trykket, initialiserer vi timeren til den gemte tid
    if (knap_trykket) {
        timer = gemt_h;
        minutter = gemt_m;
        sekunder = gemt_s;
        knap_trykket = 0; // Nulstil knap-flaget
    }

    if (sidste_tik > 400000) { 
        char vis_buffer[40];
        char gemt_buffer[40];
        
        // 1. Vis den nutidige timer i terminalen (Linje 2)
        sprintf(vis_buffer, "\e[2;1H\e[K      tid: %02d:%02d:%02d", timer, minutter, sekunder);
        printString(vis_buffer);
        
        // 2. Vis den gemte tid i terminalen (Linje 3)
        sprintf(gemt_buffer, "\e[3;1H\e[K   ny tid: %02d:%02d:%02d", gemt_h, gemt_m, gemt_s);
        printString(gemt_buffer);
        
        // OLED opdatering:
        // Vi viser den aktive tid øverst (Række 2)
        sendStrXY(vis_buffer + 19, 4, 7); 
        // Vi viser den gemte tid nederst (Række 5)
        char oled_gemt[20];
        sprintf(oled_gemt, "%02d:%02d:%02d", gemt_h, gemt_m, gemt_s);
        sendStrXY(oled_gemt, 2, 8);

        // Tæller den nuværende timer op
        sekunder++;
        if (sekunder >= 60) { sekunder = 0; minutter++; }
        if (minutter >= 60) { minutter = 0; timer++; }
        
        sidste_tik = 0; 
    }
    
    sidste_tik++; 
}

int main(void) {
    Init(); 
    
    printString("\e[2J\e[H"); // Ryd skærm og sæt markør i top
    _delay_ms(10); 
    
    // Denne tekst bliver nu stående fast øverst i monitoren
    printString("Skriv ny tid HH:MM:SS i monitor. Tryk knap for at ændre til ny tid\r\n");

    while (1) {
        tjekUART(); 
        opdaterUr(); 
    }
}