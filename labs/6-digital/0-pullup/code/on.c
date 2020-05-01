// trivial program to set pin 21 to a pullup, 20 to a pulldown.
#include "rpi.h"

#define GPPUD 0x20200094
#define GPPUDCLK0 0x20200098
#define GPPUDCLK1 0x2020009c
#define DELAY 150

void gpio_set_pullup(unsigned pin) { 
    *(volatile unsigned *)(GPPUD) = 0x10; // Enable pull-up
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
    // delay_us(150);
    // *(volatile unsigned *)(GPPUD) = 0;
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1;
}
void gpio_set_pulldown(unsigned pin) {
    *(volatile unsigned *)(GPPUD) = 0x01; // Enable pull-down
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
    // delay_us(150);
    // *(volatile unsigned *)(GPPUD) = 0;
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1;
 }
void gpio_pud_off(unsigned pin) { 
    *(volatile unsigned *)(GPPUD) = 0; // Disable pull-down
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1 << pin;
    // delay_us(150);
    // *(volatile unsigned *)(GPPUD) = 0;
    delay_us(150);
    *(volatile unsigned *)(GPPUDCLK0) = 1;
 }

void notmain(void) {
    uart_init();

    unsigned up = 21, down = 20;

    // i don't believe we need to set input (since the state is sticky)
    gpio_set_input(up);
    gpio_set_pullup(up);
    printk("pullup=%d, this is sticky: touch red lead to this pin and black to ground: should see a ~3V reading on multimeter\n", up);

    gpio_set_input(up);
    gpio_set_pulldown(down);
    printk("pulldown=%d, this is sticky: touch black lead to this pin, and red to 3v: should see a ~3V reading on multimeter\n", down);

    clean_reboot();
}
