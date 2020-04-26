// engler, cs240lx: skeleton for test generation.  
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"

#include "../scope-constants.h"
#include "../sw-uart.h"

unsigned cycles_per_sec(unsigned s) {
    demand(s < 2, will overflow);
    unsigned start = cycle_cnt_read();
    delay_ms(s * 1000);
    return cycle_cnt_read() - start;
}

// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
    unsigned start = cycle_cnt_read();
    unsigned v = 1;
    for (unsigned i = 0; i < N; i++) {
        printk("%d\n", v);
        write_cyc_until(pin, v, cycle_cnt_read(), CYCLE_PER_FLIP);
        v ^= 1;
    }

    unsigned end = cycle_cnt_read();

    // crude check how accurate we were ourselves.
    printk("expected %d cycles, have %d\n", ncycle*N, end-start);
}

void
server (void)
{
    gpio_set_input (IN_0);
    gpio_set_output (OUT_0);
    sw_uart_t uart;
    sw_uart_init (&uart, OUT_0, IN_0, BAUD_RATE, cycles_per_bit());
    cycle_cnt_init();

    uint8_t ball = BALL;
    gpio_write_raw (uart.tx, STOP_BIT); // Turn line on to signal server ready
    unsigned v = !gpio_read(uart.rx);
    wait_until_cyc(uart.rx, v, cycle_cnt_read(), TIMEOUT); // Wait for client connection
    for (unsigned i = 0; i < BALL_TEST_N; i++) {
        sw_uart_put8 (&uart, ball);
        ball = sw_uart_get8 (&uart);
   }

    printk("After %d iterations, ball is: %x. Ball should be %x.\n", 
            BALL_TEST_N, ball, BALL);
}

void notmain(void) {
    // int pin = 21;
    // gpio_set_output(pin);
    // sw_uart_t con1;
    // sw_uart_init(&con1, pin, pin, BAUD_RATE, cycles_per_bit());
    // cycle_cnt_init();
    
    // uint8_t msg = 0b10101010;
    // sw_uart_put8(&con1, msg);

    server ();
    clean_reboot();
}
