// put your pull-up/pull-down implementations here.
#include "rpi.h"

// #define GPPUD 0x20200094
// #define GPPUDCLK0 0x20200098
// #define GPPUDCLK1 0x2020009c
// #define DELAY 150

// void gpio_set_pullup(unsigned pin) { 
//     *(volatile unsigned *)(GPPUD) = 0x10; // Enable pull-up
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUD) = 0;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1;
// }
// void gpio_set_pulldown(unsigned pin) {
//     *(volatile unsigned *)(GPPUD) = 0x01; // Enable pull-down
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUD) = 0;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1;
//  }
// void gpio_pud_off(unsigned pin) { 
//     *(volatile unsigned *)(GPPUD) = 0; // Disable pull-down
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUD) = 0;
//     delay_us(150);
//     *(volatile unsigned *)(GPPUDCLK0) = 1;
//  }
