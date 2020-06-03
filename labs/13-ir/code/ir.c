// engler, cs240lx: simple scope skeleton for logic analyzer.
#include "rpi.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "../../../liblxpi/liblxpi.h"

#define TIMEOUT 40000
#define GRANDE0_PERIOD 600
#define GRANDE1_PERIOD 1600

#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098
#define DELAY       150

#define IR_PIN 16

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


// dumb log.  use your own if you like!
typedef struct {
    unsigned v;
    int time;
} log_ent_t;

static int 
wait_until_usec(unsigned pin, unsigned v, unsigned timeout)
{
    unsigned end = timer_get_usec() + timeout;
    while (timer_get_usec() < end) {
        if (gpio_read(pin) == v)
            return 1;
    }
    return 0;
}

// monitor <pin>, recording any transitions until either:
//  1. we have run for about <max_cycles>.  
//  2. we have recorded <n_max> samples.
//
// return value: the number of samples recorded.
unsigned 
scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned timeout) {
    unsigned n = 0;
    unsigned v = 0;
    wait_until_usec(pin, v, 10000000);
    for (; n < n_max; n++) {
        unsigned start = timer_get_usec();
        if (wait_until_usec(pin, !v, timeout) == 0) 
            break;
        v ^= 1;
        l[n].time = timer_get_usec() - start;
        l[n].v = v;
    }

    return n;
}

enum { BUTTON1 = 0xd0283085,
       BUTTON2 = 0xd0293085,
       BUTTON3 = 0xd02a3085,
       BUTTON4 = 0xd02b3085,
       BUTTON5 = 0xd02c3085,
       BUTTON6 = 0xd02d3085,};

enum { ir_eps = 200 };

static int abs(int x) { return x < 0 ? -x : x; }

static int within(int t, int bound, int eps) {
    return abs(t-bound) < eps;
}


// dump out the log, calculating the error at each point,
// and the absolute cumulative error.
void dump_samples(log_ent_t *l, unsigned n) {
    // for(int i = 0; i < n-1; i++) {
    //     log_ent_t *e = &l[i];
    //     int ncyc = e->time;
    //     printk(" %d: val=%d, time=%d\n", i, e->v, ncyc);
    // }
    
    unsigned val = 0;
    unsigned pos = 0;

    for(int i = 2; i < n; i++) {
        if (l[i].v) 
            continue;
        unsigned time = l[i].time; 
        if (within(time, GRANDE1_PERIOD, ir_eps))
            val |= 1 << pos;
        else if (!within(time, GRANDE0_PERIOD, ir_eps))
            panic("Invalid time rip\n");
        pos++;
    }

    printk("Value recieved: %x\n", val);
}

unsigned get_button (log_ent_t *l, unsigned n) {
    unsigned val = 0;
    unsigned pos = 0;

    for(int i = 2; i < n; i++) {
        if (l[i].v) 
            continue;
        unsigned time = l[i].time; 
        if (within(time, GRANDE1_PERIOD, ir_eps))
            val |= 1 << pos;
        else if (!within(time, GRANDE0_PERIOD, ir_eps))
            panic("Invalid time rip\n");
        pos++;
    }

    return val;
}


static volatile unsigned pos = 0;
static volatile unsigned ir_pin = 0;
static volatile int data_ready = 0;
static volatile unsigned data = 0;
static volatile unsigned last_time = 0;

void 
init_ir_interrupt (unsigned pin)
{
    int_init();
    ir_pin = pin;
    data_ready = 0;
    data = 0;
    pos = 0;
    gpio_int_falling_edge(pin);
    system_enable_interrupts();
}

void interrupt_vector(unsigned pc) {

    //printk ("IN HANDLE\n");
    if (!gpio_event_detected(IR_PIN))
        return;

    if (data_ready) {
        gpio_event_clear(ir_pin);
        return;
    }

    unsigned time = timer_get_usec();
    if (last_time) {
        if (within(time - last_time, 600 + GRANDE0_PERIOD, 200)) {
            pos++;
        } else if (within(time - last_time, 600 + GRANDE1_PERIOD, 200)) {
            data |= 1 << pos;
            pos++;
        }

        if (pos >= 32) {
            data_ready = 1;
            pos = 0;
            last_time = 0;
        }
    }

    last_time = time;
    gpio_event_clear(ir_pin);

}

unsigned read_data() {
    while (!data_ready)
        ;

    unsigned ret = data;
    data = 0;
    data_ready = 0;
    return ret;
    
}

void test() {
    unsigned ir_data = read_data();
    switch (ir_data)
    {
    case BUTTON1:
        printk("Button1 pressed\n");
        break;

    case BUTTON2:
        printk("Button2 pressed\n");
        break;
    
    case BUTTON3:
        printk("Button3 pressed\n");
        break;

    case BUTTON4:
        printk("Button4 pressed\n");
        break;

    case BUTTON5:
        printk("Button5 pressed\n");
        break;
    
    case BUTTON6:
        printk("Button6 pressed\n");
        break;

    default:
        printk("Unkown button: %x\n", ir_data);
        break;
    }
}

void notmain(void) {
    //init_gpio ();
    int pin = IR_PIN;
    gpio_set_input(pin);
    gpio_set_pullup(pin);
    cycle_cnt_init();
    init_ir_interrupt(pin);

// #   define MAXSAMPLES 100
//     log_ent_t log[MAXSAMPLES];

//     unsigned n = scope(pin, log, MAXSAMPLES, TIMEOUT);

    for (;;)
        test();

    gpio_pud_off(pin);
    clean_reboot();
}
