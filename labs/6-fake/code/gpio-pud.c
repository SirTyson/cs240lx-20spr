// put your pull-up/pull-down implementations here.
#include "rpi.h"

#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098
#define DELAY       150

void 
gpio_set_pullup (unsigned pin) 
{ 
    put32 ((volatile void *) GPPUD, 0b10);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
}

void 
gpio_set_pulldown (unsigned pin) 
{ 
    put32 ((volatile void *) GPPUD, 0b01);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
}

void
gpio_pud_off (unsigned pin) 
{ 
    put32 ((volatile void *) GPPUD, 0);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
}
