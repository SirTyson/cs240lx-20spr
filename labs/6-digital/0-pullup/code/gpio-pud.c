// put your pull-up/pull-down implementations here.
// #include "rpi.h"

// volatile void *GPPUD = (volatile void *)0x20200094;
// volatile void *GPPUDCLK0 = (volatile void *)0x20200098;
// volatile void *GPPUDCLK1 = (volatile void *)0x2020009c;
// #define DELAY 150

// void gpio_set_pullup(unsigned pin) { 
//     put32(GPPUD, 0x10); // Enable pull-up
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
// }
// void gpio_set_pulldown(unsigned pin) {
//     put32(GPPUD, 0x01); // Enable pull-up
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
//  }
// void gpio_pud_off(unsigned pin) { 
//     put32(GPPUD, 0x00); // Enable pull-up
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
//     delay_us(150);
//     put32(GPPUDCLK0, 1 << pin);
//  }
