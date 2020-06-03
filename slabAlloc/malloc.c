#include "malloc.h"
#include "slab-malloc-int.h"

static void init_malloc (void);
static int INIT = 0;

void *
malloc (size_t size)
{
  if (!INIT)
    {
      init_heap ();
      INIT = 1;
    }

  return 0;
}

void *
realloc (void *ptr, size_t size)
{
  return 0;
}

void 
free (void *ptr)
{

}