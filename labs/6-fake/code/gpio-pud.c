// put your pull-up/pull-down implementations here.
#include "rpi.h"
#include "../../../liblxpi/liblxpi.h"

#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098
#define DELAY       150

void 
gpio_set_pullup (unsigned pin) 
{
    dev_barrier ();
    put32 ((volatile void *) GPPUD, 0b10);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUD, 0);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
    delay_us (DELAY);
    dev_barrier ();
}

void 
gpio_set_pulldown (unsigned pin) 
{ 
    dev_barrier ();
    put32 ((volatile void *) GPPUD, 0b01);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUD, 0);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
    delay_us (DELAY);
    dev_barrier ();
}

void
gpio_pud_off (unsigned pin) 
{ 
    dev_barrier ();
    put32 ((volatile void *) GPPUD, 0);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 1 << pin);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUD, 0);
    delay_us (DELAY);
    put32 ((volatile void *) GPPUDCLK0, 0);
    delay_us (DELAY);
    dev_barrier ();
}
