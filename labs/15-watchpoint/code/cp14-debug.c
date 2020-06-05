// handle debug exceptions.
#include "rpi.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "cp14-debug.h"
#include "bit-support.h"

/******************************************************************
 * part1 set a watchpoint on 0.
 )*/
static handler_t watchpt_handler0 = 0;

// prefetch flush.
#define prefetch_flush() asm volatile ("mcr p15,0, r0, c7,c5,4")

cp14_asm(cp14_wvr0, c0, 0b110)
cp14_asm(cp14_wcr0, c0, 0b111)
cp14_asm(cp14_bcr0, c0, 0b101)
cp14_asm(cp14_bvr0, c0, 0b100)
cp14_asm_set(cp14_status, c1, 0b0)

// set the first watchpoint: calls <handler> on debug fault.
void watchpt_set0(void *_addr, handler_t handler) {

    uint32_t wcr0 = cp14_wcr0_get();
    wcr0 &= (uint32_t) -1;              // Clear WCR[0]
    cp14_wcr0_set(wcr0);

    cp14_wvr0_set((uint32_t) _addr);

    wcr0 &= ~((uint32_t)(1 << 20));
    wcr0 &= ~((uint32_t)(0b11 << 14));  // Might need to be 15
    
    wcr0 |= 0b1000 << 5;
    wcr0 |= 0b11 << 3;
    wcr0 |= 0b11 << 1;
    wcr0 |= 1;
    cp14_wcr0_set(wcr0);

    prefetch_flush();
    watchpt_handler0 = handler;
}

void interrupt_vector(unsigned pc){
    panic("HIT WRONG INTERRUPT HANDLER\n");
}

// check for watchpoint fault and call <handler> if so.
void data_abort_vector(unsigned pc) {
    static int nfaults = 0;
    printk("nfault=%d: data abort at %p\n", nfaults++, pc);
    if(datafault_from_ld())
        printk("was from a load\n");
    else
        printk("was from a store\n");
    if(!was_debug_datafault()) 
        panic("impossible: should get no other faults\n");

    // this is the pc
    printk("wfar address = %p, pc = %p\n", wfar_get()-8, pc);
    assert(wfar_get()-4 == pc);

    assert(watchpt_handler0);
    
    uint32_t addr = far_get();
    printk("far address = %p\n", addr);

    // should send all the registers so the client can modify.
    watchpt_handler0(0, pc, addr);
}

void cp14_enable(void) {
    static int init_p = 0;

    if(!init_p) { 
        int_init();
        init_p = 1;
    }

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t dscr = cp14_status_get();
    dscr |= 1 << 15;
    cp14_status_set(dscr);

    prefetch_flush();
    // assert(!cp14_watchpoint_occured());

}

/**************************************************************
 * part 2
 */

static handler_t brkpt_handler0 = 0;


static unsigned bvr_match(void) { return 0b00 << 21; }
static unsigned bvr_mismatch(void) { return 0b10 << 21; }

static inline uint32_t brkpt_get_va0(void) {
    return cp14_bvr0_get();
}

static void brkpt_disable0(void) {
    uint32_t bcr0 = cp14_bcr0_get();
    bcr0 &= (uint32_t) -1;              // Clear BCR[0]
    cp14_bcr0_set(bcr0);
    prefetch_flush();
}

// 13-16
// returns the 
void brkpt_set0(uint32_t addr, handler_t handler) {
    uint32_t bcr0 = cp14_bcr0_get();
    bcr0 &= (uint32_t) -1;              // Clear BCR[0]
    cp14_bcr0_set(bcr0);

    cp14_bvr0_set(addr);

    bcr0 = bits_clr(bcr0, 21, 22); // Set address mode to match
    bcr0 = bit_clr(bcr0, 20);      // Disable Linking
    bcr0 = bits_clr(bcr0, 14, 15); // Break in secure and non-secure world
    bcr0 = bits_set(bcr0, 5, 8, 1);   // Break for bytes within +0 to +4
    bcr0 = bits_set(bcr0, 1, 2, 1);   // Activated under all priveldges
    bcr0 = bit_set(bcr0, 0);       // Enable breakpoint

    cp14_bcr0_set(bcr0);

    prefetch_flush();
    brkpt_handler0 = handler;
}

// if get a breakpoint call <brkpt_handler0>
void prefetch_abort_vector(unsigned pc) {
    printk("prefetch abort at %p\n", pc);
    if(!was_debug_instfault())
        panic("impossible: should get no other faults\n");
    assert(brkpt_handler0);
    brkpt_handler0(0, pc, ifar_get());
}
