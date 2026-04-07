#include "timer.h"
#include <avr/interrupt.h>

volatile uint16_t ms = 0; // Skal være volatile, da den ændres i ISR
// global variabel der fylder 16 bit Atmega har en 8 bit processor så kan kun flytte 8 bit af gangen, derfor skal vi læse til ny variabel i main, så vi ikke bruegr to arbejdsgange hver gang i læser ms
volatile uint8_t ss_flag = 0;
uint8_t ss = 0, mm = 0, hh = 0;

void timer1_init() {
    TCCR1B |= (1 << WGM12);     // Timer 1, mode 4: CTC mode (reset timer ved bestemt værdi, så vi kan synkronisere timer med et sekund f.eks.)
    
    OCR1A = OCR_VALUE;          // compileren udregner  og indstiller at timer skal reset hvert 249. ticks (ms)
    
    TIMSK1 |= (1 << OCIE1A);    // Giver timeren lov til at afbryde CPU'en, når den rammer værdien i OCR1A.
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
}

// I servicerutinen tælles en global variabel op hver gang, der er interrupt – når værdien er 999,
// sættes et flag = 1, og tællerværdien sættes til 0.
ISR(TIMER1_COMPA_vect) {  // Interrupt service routine hvert ms
  ms++;
  if  (ms >= 1000)  {
      ss_flag = 1; // "der er gået mindst et sekund flaget!"
  }
}