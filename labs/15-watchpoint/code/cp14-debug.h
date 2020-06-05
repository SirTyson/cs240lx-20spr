#ifndef __CP14_DEBUG_H__
#define __CP14_DEBUG_H__

/********************************************************************
 * co-processor helper macro (should move to main header).
 *
 * you can use it or not, depending on if it helps you.
 *
 * 
 * cp14 instructions have the form:
 *
 *  mrc p14, opcode_1, <Rd>, Crn, Crm, opcode_2
 * where 
 *  opcode_1 = 0, 
 *  crn = 0
 * and you only set opcode_2 and crm.
 *
 * so:
 *  mrc p14, 0, <Rd>, c0, Crm, opcode_2
 */

// turn <x> into a string
#define MK_STR(x) #x

// define a general co-processor inline assembly routine to set the value.
// from manual: must prefetch-flush after each set.
#define cp_asm_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline void fn_name ## _set(uint32_t v) {                    \
        asm volatile ("mcr " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) :: "r" (v));               \
        prefetch_flush();                                               \
    }

#define cp_asm_get(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline uint32_t fn_name ## _get(void) {                      \
        uint32_t ret = 0;                                                   \
        asm volatile ("mrc " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) : "=r" (ret));             \
        return ret;                                                     \
    }


// specialize to cp14
#define cp14_asm_set(fn, crm, op2) cp_asm_set(fn, p14, 0, c0, crm, op2)
#define cp14_asm_get(fn, crm, op2) cp_asm_get(fn, p14, 0, c0, crm, op2)
// do both _set and _get
#define cp14_asm(fn,crm,op2) \
    cp14_asm_set(fn,crm,op2)   \
    cp14_asm_get(fn,crm,op2)

/********************************************************************
 * part 0: get debug id register
 */

struct debug_id {
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register pairs
                                // add 1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// 13-5
cp14_asm_get(cp14_debug_id, c0, 0)
cp14_asm_get(cp14_status, c1, 0b0)

/********************************************************************
 * part 1: set a watchpoint on 0 (test2.c/test3.c)
 */
#include "bit-support.h"

cp14_asm_get(wfar, c6, 0)
cp_asm_get(dfsr, p15, 0, c5, c0, 0)
cp_asm_get(far, p15, 0, c6, c0, 0)

// was watchpoint debug fault caused by a load?
static inline unsigned datafault_from_ld(void) {
    return !(dfsr_get() & (1 << 11));
}
// ...  by a store?
static inline unsigned datafault_from_st(void) {
    return !datafault_from_ld();
}

static inline unsigned was_debugfault_r(uint32_t r) {
    // "these bits are not set on debug event.
    uint32_t status = dfsr_get();
    return !bit_isset(status, 10) && bits_eq(status, 0, 3, 0b0010);
}

// are we here b/c of a datafault?
static inline unsigned was_debug_datafault(void) {
    unsigned r = dfsr_get();
    if(!was_debugfault_r(r))
        panic("impossible: should only get datafaults\n");
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
    uint32_t dscr = cp14_status_get();
    uint32_t debug_entry = (dscr & (uint32_t)(0b1111 << 2)) >> 2;
    return debug_entry == 1 || debug_entry == 2;
}

// client supplied fault handler: give a pointer to the registers so 
// the client can modify them (for the moment pass NULL)
//  - <pc> is where the fault happened, 
//  - <addr> is the fault address.
typedef void (*handler_t)(uint32_t regs[16], uint32_t pc, uint32_t addr);

// set a watchpoint at <addr>: calls <handler> with a pointer to the registers.
void watchpt_set0(void *addr, handler_t watchpt_handle);

// enable the co-processor.
void cp14_enable(void);

/********************************************************************
 * part 2: set a breakpoint on 0 (test4.c) / foo (test5.c)
 */

cp_asm_get(ifsr, p15, 0, c5, c0, 1)
cp_asm_get(ifar, p15, 0, c6, c0, 2)

// was this a debug instruction fault?
static inline unsigned was_debug_instfault(void) {
    uint32_t status = ifsr_get();
    return !bit_isset(status, 10) && bits_eq(status, 0, 3, 0b0010);
}

// set a breakpoint at <addr>: call handler when the fault happens.
void brkpt_set0(uint32_t addr, handler_t brkpt_handler);

#endif
