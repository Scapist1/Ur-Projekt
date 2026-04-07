#include "externInt.h"
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t button_pressed = 0;

void extint4_init(void)
{
    // INT4 på falling edge (aktiv-lav button)
    EICRB |= (1 << ISC41);
    EICRB &= ~(1 << ISC40);
    EIMSK |= (1 << INT4);
}

ISR(INT4_vect)
{
    button_pressed = 1;
}
