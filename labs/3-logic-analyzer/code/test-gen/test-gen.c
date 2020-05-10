// engler, cs240lx: skeleton for test generation.  
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"

#include "../scope-constants.h"

// Best = 66199

// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
    pin = 1 << pin;
    unsigned start = cycle_cnt_read();
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_clr0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_clr0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_clr0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_clr0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_clr0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);
    write_on0_until (pin, cycle_cnt_read(), CYCLE_PER_FLIP);

    unsigned end = cycle_cnt_read();

    // crude check how accurate we were ourselves.
    printk("expected %d cycles, have %d\n", ncycle*N, end-start);
}

void notmain(void) {
    enable_cache ();
    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen (pin, 11, CYCLE_PER_FLIP);

    clean_reboot();
}
