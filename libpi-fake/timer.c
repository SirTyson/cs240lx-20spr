#include "fake-pi.h"

void
delay_cycles (unsigned ticks)
{
    trace ("delay_cycles = %ucycles\n", ticks);
}

void 
delay_us (unsigned us)
{
    trace ("delay_us = %uusec\n", us);
}