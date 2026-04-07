#ifndef EXTERN_INT_H_
#define EXTERN_INT_H_

#include <stdint.h>

extern volatile uint8_t button_pressed;

void extint4_init(void);

#endif
