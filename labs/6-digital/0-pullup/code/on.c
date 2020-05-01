// trivial program to set pin 21 to a pullup, 20 to a pulldown.
#include "rpi.h"

volatile void *GPPUD = (volatile void *)0x20200094;
volatile void *GPPUDCLK0 = (volatile void *)0x20200098;
volatile void *GPPUDCLK1 = (volatile void *)0x2020009c;
#define DELAY 150

void gpio_set_pullup(unsigned pin) { 
    put32(GPPUD, 0x10); // Enable pull-up
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
}
void gpio_set_pulldown(unsigned pin) {
    put32(GPPUD, 0x01); // Enable pull-up
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
 }
void gpio_pud_off(unsigned pin) { 
    put32(GPPUD, 0x00); // Enable pull-up
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
    delay_us(150);
    put32(GPPUDCLK0, 1 << pin);
 }

void notmain(void) {
    uart_init();

    unsigned up = 21, down = 20;

    // i don't believe we need to set input (since the state is sticky)
    gpio_set_input(up);
    gpio_set_pullup(up);
    printk("pullup=%d, this is sticky: touch red lead to this pin and black to ground: should see a ~3V reading on multimeter\n", up);

    gpio_set_input(down);
    gpio_set_pulldown(down);
    printk("pulldown=%d, this is sticky: touch black lead to this pin, and red to 3v: should see a ~3V reading on multimeter\n", down);

    clean_reboot();
}
