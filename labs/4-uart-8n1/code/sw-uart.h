#ifndef __SW_UART_H__
#define __SW_UART_H__
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"
#include "scope-constants.h"

typedef struct {
    uint8_t tx,rx;
    uint32_t baud;
    uint32_t cycle_per_bit; 
} sw_uart_t;

static inline void 
sw_uart_init (sw_uart_t *uart, uint8_t tx, uint8_t rx, uint32_t baud, uint32_t cycle_per_bit) 
{
    uart->tx = tx;
    uart->rx = rx;
    uart->baud = baud;
    uart->cycle_per_bit = cycle_per_bit;
}

static inline int 
sw_uart_put8 (sw_uart_t *uart, uint8_t b) {
    write_cyc_until(uart->tx, START_BIT, cycle_cnt_read(), uart->cycle_per_bit);
    for (unsigned i = 0; i < 8; i++) 
        write_cyc_until(uart->tx, b & (1 << i), cycle_cnt_read(), uart->cycle_per_bit);    
    write_cyc_until(uart->tx, STOP_BIT, cycle_cnt_read(), uart->cycle_per_bit);
    return 1;
}

// blocks until it receives a character, returns < 0 on error.
static inline int 
sw_uart_get8 (sw_uart_t *uart) {
    int result = 0;
    if (!wait_until_cyc(uart->rx, START_BIT, cycle_cnt_read(), TIMEOUT))
        return -1;
    delay_ncycles(cycle_cnt_read(), uart->cycle_per_bit + DELAY);
    for (unsigned i = 0; i < 8; i++)
        result |= wait_until_cyc(uart->rx, 1, cycle_cnt_read(), uart->cycle_per_bit) << i;
    if (!wait_until_cyc(uart->rx, STOP_BIT, cycle_cnt_read(), uart->cycle_per_bit))
        return -1;
    return result;
}

#endif