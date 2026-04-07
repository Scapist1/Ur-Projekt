#ifndef UTILS_H_
#define UTILS_H_

#include <avr/io.h>

// char switch_debounce(void); //debounce funktionen - bruges ikke mere
void button_init(void); //til pins og porte til button
void display_init(void); // display porte
void DIP_init(); // til DIP switch

#endif