#include "slab-malloc-int.h"
#include <unistd.h>

static int PG_SIZE;
static void *HEAP;

void 
init_heap (void)
{
    PG_SIZE = getpagesize ();
    HEAP = sbrk (PG_SIZE);
}
