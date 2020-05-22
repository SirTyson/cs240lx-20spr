#include "rpi-asm.h"

MK_FN(dump_regs)
    @ str r0, [r0], #4
    @ str r1, [r0], #4
    @ str r2, [r0], #4
    @ str r3, [r0], #4
    @ str r4, [r0], #4
    @ str r5, [r0], #4
    @ str r6, [r0], #4
    @ str r7, [r0], #4
    @ str r8, [r0], #4
    @ str r9, [r0], #4
    @ str r10, [r0], #4
    @ str r11, [r0], #4
    @ str r12, [r0], #4
    @ str r13, [r0], #4
    @ str r14, [r0], #4
    @ str r15, [r0], #4
    stm r0, {r0-r15}
    bx lr;
