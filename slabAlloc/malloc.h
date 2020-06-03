#ifndef __SLABMALLOC_H__
#define __SLABMALLOC_H__

#include <stddef.h>
#include "slab-malloc-int.h"
// Inspiration: http://www.scs.stanford.edu/20wi-cs140/sched/readings/bonwick:slab.pdf

typedef void (*constructor)(void *, size_t);
typedef void (*destructor)(void *, size_t);

typedef struct mem_cache
  {
    size_t size;
    constructor construct;
    destructor destruct;
    char pad[8];
  } mem_cache;

/* Public malloc operations */
void *malloc (size_t);
void *realloc (void *, size_t);
void free (void *ptr);

/* Object malloc operations */
mem_cache *create_obj (size_t, constructor, destructor);
void *obj_alloc (mem_cache *);
void obj_free (void *, mem_cache *);


#endif