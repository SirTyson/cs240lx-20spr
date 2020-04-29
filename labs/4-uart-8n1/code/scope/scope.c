// engler, cs240lx: simple scope skeleton for logic analyzer.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"
#include "../scope-constants.h"
#include "../sw-uart.h"

// dumb log.  use your own if you like!
typedef struct {
    unsigned v,ncycles;
} log_ent_t;


// compute the number of cycles per second
unsigned cycles_per_sec(unsigned s) {
    demand(s < 2, will overflow);
    unsigned start = cycle_cnt_read();
    delay_ms(s * 1000);
    return cycle_cnt_read() - start;
}

// monitor <pin>, recording any transitions until either:
//  1. we have run for about <max_cycles>.  
//  2. we have recorded <n_max> samples.
//
// return value: the number of samples recorded.
unsigned 
scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned n = 0;
    unsigned v = !gpio_read(pin);
    wait_until_cyc(pin, v, 0, (unsigned) -1);
    for (; n < n_max; n++) {
        unsigned start = cycle_cnt_read();
        if (wait_until_cyc(pin, !v, start, max_cycles) == 0) 
            break;
        v ^= 1;
        l[n].ncycles = cycle_cnt_read() - start;
        l[n].v = v;
    }

    return n;
}

// dump out the log, calculating the error at each point,
// and the absolute cumulative error.
void dump_samples(log_ent_t *l, unsigned n, unsigned period) {
    unsigned tot = 0, tot_err = 0;

    for(int i = 0; i < n-1; i++) {
        log_ent_t *e = &l[i];

        unsigned ncyc = e->ncycles;
        tot += ncyc;

        unsigned exp = period * (i+1);
        unsigned err = tot > exp ? tot - exp : exp - tot;
        tot_err += err;

        printk(" %d: val=%d, time=%d, tot=%d: exp=%d (err=%d, toterr=%d)\n", i, e->v, ncyc, tot, exp, err, tot_err);
    }
}

void
client (void)
{
    gpio_set_input (IN_0);
    gpio_set_output (OUT_0);
    sw_uart_t uart;
    sw_uart_init (&uart, OUT_0, IN_0, BAUD_RATE, cycles_per_bit());
    cycle_cnt_init();

    uint8_t ball;
    wait_until_cyc(uart.rx, STOP_BIT, cycle_cnt_read(), TIMEOUT); // Wait for server to turn on
    gpio_write_raw (uart.tx, 1); // Signal to server that client ready for msg
    printk("Connected!\n");
    for (unsigned i = 0; i < BALL_TEST_N; i++) {
        ball = sw_uart_get8 (&uart);
        //sw_uart_put8(&uart, ball);
    }

    printk ("GOT BALL %x\n", ball);
}

void notmain(void) {
    init_gpio ();
    int pin = IN_0;
    gpio_set_input(pin);
    cycle_cnt_init();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    // gpio_set_output(OUT_0);
    // gpio_write_raw (OUT_0, 1);
    unsigned n = scope(pin, log, MAXSAMPLES, cycles_per_sec(1));

    // <CYCLE_PER_FLIP> is in ../scope-constants.h
    dump_samples(log, n, cycles_per_bit());
    
    //client ();
    clean_reboot();
}
