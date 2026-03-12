#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <utils.h>
#include "I2C.h"     // i2c driver
#include "ssd1306.h" // display driver
#include <avr/interrupt.h>

//char switch_debounce() //den gamle funktion
//{
//    if ((PINE & (1 << PE4)) == 0) // den er trykket (aktivt lav)
//    {
//        _delay_ms(30);
//        if ((PINE & (1 << PE4)) == 0) // tjek om stadig trykket (debounce)
//        {
//            PORTB |= (1 << PB7);  // blink LED'en
//            _delay_ms(50);        // vent til slip
//            PORTB &= ~(1 << PB7); // sluk LED'en igen
//            return 1;
//        }
//    }
//    else
//    {
//        return 0;
//    }
//}

void DIP_init()
{
    DDRK = 0b00000000;   // behøver man nok ikk gøre
    PORTK |= 0xFF;       // sætter pullup på dip-switch
}

//void button_init()
//{
//    DDRE &= ~(1 << PE4);// input
//    PORTE |= (1 << PE4); // aktiv lav
//    DDRB |= (1 << DDB7); // sætter LED pin til output
//
//    EICRB |= (1 << ISC41); // 1, 0 for falling edge som jeg tænker vores knap må være
//    EICRB &= ~(1 << ISC40); 
//
//    EIMSK |= (1 << INT4); // Enable INT4
//
//    sei(); // globale interrupts
//}

//void button_init_old(void) //fra da vi pollede
//{
//    DDRE &= ~(1 << PE4); // sætter til input for en sikkerheds skyld
//    PORTE |= (1 << PE4); // sætter pull-up så aktiv lav knappen virker
//}

void display_init(void){
    _i2c_address = 0X78; // write address for i2c interface
    I2C_Init();          // initialize i2c interface to display
    InitializeDisplay(); // initialize  display
    clear_display();     // use this before writing you own text
    sendStrXY("Velkommen", 3, 3);
    _delay_ms(1200);
    clear_display();
}
