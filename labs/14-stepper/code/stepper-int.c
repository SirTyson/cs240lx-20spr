#include "stepper-int.h"
#include "timer-interrupt.h"
#include "cycle-count.h"
#include "math-helpers.h"

// you can/should play around with this
#define STEPPER_INT_TIMER_INT_PERIOD 100 

static int first_init = 1;

#define MAX_STEPPERS 16
static stepper_int_t *my_stepper;
static stepper_position_t *cur_job = 0;
static unsigned num_steppers = 0;

void stepper_int_handler(unsigned pc) {
    // check and clear timer interrupt
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();  

    // No job
    if (!cur_job) {
        if (Q_empty(&my_stepper->positions_Q)) {
            my_stepper->status = NOT_IN_JOB;
            return;
        } else {
            cur_job = Q_pop(&my_stepper->positions_Q);
            cur_job->status = STARTED;
            // Start time usec
            if (cur_job->goal_steps < my_stepper->stepper->step_count) {
                gpio_write(my_stepper->stepper->dir, 0);
                gpio_write(my_stepper->stepper->step, 1);
                gpio_write(my_stepper->stepper->step, 0);
                (my_stepper->stepper->step_count)--;
            } else {
                gpio_write(my_stepper->stepper->dir, 1);
                gpio_write(my_stepper->stepper->step, 1);
                gpio_write(my_stepper->stepper->step, 0);
                (my_stepper->stepper->step_count)++;
            }

            //cur_job->usec_at_prev_step = timer_get_usec();
            //return;
        }
    }

    if (cur_job->goal_steps == my_stepper->stepper->step_count) {
        cur_job->status = FINISHED;
        cur_job = 0;
        stepper_int_handler(pc);
        return;
    }

    unsigned cur = timer_get_usec();
    if (cur > cur_job->usec_at_prev_step + cur_job->usec_between_steps) {

        if (cur_job->goal_steps < my_stepper->stepper->step_count) {
            gpio_write(my_stepper->stepper->dir, 0);
            gpio_write(my_stepper->stepper->step, 1);
            gpio_write(my_stepper->stepper->step, 0);
            (my_stepper->stepper->step_count)--;
        } else {
            gpio_write(my_stepper->stepper->dir, 1);
            gpio_write(my_stepper->stepper->step, 1);
            gpio_write(my_stepper->stepper->step, 0);
            (my_stepper->stepper->step_count)++;
        }  
        cur_job->usec_at_prev_step = timer_get_usec();     
    }
    
}

// void interrupt_vector(unsigned pc){
//     stepper_int_handler(pc);
// }

stepper_int_t * stepper_init_with_int(unsigned dir, unsigned step){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }

    num_steppers++;
    if(first_init){
        first_init = 0;
        kmalloc_init();
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        system_enable_interrupts();
    }

    stepper_int_t *stepper_int = kmalloc(sizeof(stepper_int_t));
    stepper_t * stepper = kmalloc(sizeof(stepper_t));
    
    gpio_set_output(dir);
    gpio_set_output(step);
    stepper->dir = dir;
    stepper->step = step;
    stepper->step_count = 0;

    stepper_int->stepper = stepper;
    stepper_int->status = NOT_IN_JOB;
    stepper_int->positions_Q = (Q_t) {
        .head = 0,
        .tail = 0,
        .cnt = 0,
    };

    my_stepper = stepper_int;
    return stepper_int;
}

stepper_int_t * stepper_int_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    unimplemented();
}

/* retuns the enqueued position. perhaps return the queue of positions instead? */
stepper_position_t * stepper_int_enqueue_pos(stepper_int_t * stepper, int goal_steps, unsigned usec_between_steps){
    stepper_position_t *job = kmalloc(sizeof(stepper_position_t));
    job->next = 0;
    job->goal_steps = goal_steps;
    job->usec_between_steps = usec_between_steps;
    job->usec_at_prev_step = 0;
    job->status = NOT_STARTED;

    Q_append(&stepper->positions_Q, job);
    stepper->status = IN_JOB;
    return job;
}

int stepper_int_get_position_in_steps(stepper_int_t * stepper){
    return stepper_get_position_in_steps(stepper->stepper);
}

int stepper_int_is_free(stepper_int_t * stepper){
    return stepper->status == NOT_IN_JOB;
}

int stepper_int_position_is_complete(stepper_position_t * pos){
    return pos->status == FINISHED;
}

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

typedef enum {
    A,
    C,
    D,
    G,
    Gf,
    F,
} notes_t;

void play(stepper_t * stepper) {
    unsigned ir_data = read_data();
    switch (ir_data)
    {
    // A
    case BUTTON1:
        printk("Button1 pressed\n");
        for(int i = 0; i < 100; i++){
            stepper_get_position_in_steps(stepper);
            stepper_step_forward(stepper);
            delay_us(2250);
        }
        break;

    case BUTTON2:
        printk("Button2 pressed\n");
        break;
    
    case BUTTON3:
        printk("Button3 pressed\n");
        break;

    // D
    case BUTTON4:
        printk("Button4 pressed\n");
        for(int i = 0; i < 80; i++){
            stepper_get_position_in_steps(stepper);
            stepper_step_forward(stepper);
            delay_us(3360);
        }
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


    // for(int i = 0; i < 1000; i++){
    //     stepper_step_backward(stepper);
    //     delay_us(1500);
    //     assert(stepper_get_position_in_steps(stepper) == 999- i);
    // }
}