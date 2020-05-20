#include "rpi.h"
#include "libc/helper-macros.h"
#include "ckalloc-internal.h"

// simplistic heap management: a single, contiguous heap given to us by calling
// ck_init
static uint8_t *heap = 0, *heap_end, *heap_start;

void ck_init(void *start, unsigned n) {
    assert(aligned(heap_start, 8));
    printk("sizeof hdr=%d, redzone=%d\n", sizeof(hdr_t), REDZONE);
    heap = heap_start = start;
    heap_end = heap + n;
}

// compute checksum on header.  need to do w/ cksum set to known value!
static uint32_t hdr_cksum(hdr_t *h) {
    unsigned old = h->cksum;
    h->cksum = 0;
    uint32_t cksum = fast_hash(h,sizeof *h);
    h->cksum = old;
    return cksum;
}

// check the header checksum and that its state == ALLOCED or FREED
static int check_hdr(hdr_t *h) {
    if  ((h->state != ALLOCED && h->state != FREED) || h->cksum != hdr_cksum (h))
    {
        ck_error(h, "block %p corrupted at offset %d\n", h, 0);
        return 0;
    }

    return 1;
}

// check to make sure redzone is valid
static int check_mem(char *p, unsigned nbytes, hdr_t *h) {
    int i;
    for(i = 0; i < nbytes; i++) {
        if(p[i] != SENTINAL) {
            int offs = p - (char *)b_alloc_ptr (h) + i;
            ck_error(h, "block %p corrupted at offset %d\n", p - h->nbytes_alloc, offs);
            return 0;
        }
    }

    return 1;
}

// check to make sure freed pointer is not written to, assumes free block written over by SENTINAL on free
static int check_free_write (char *p, unsigned nbytes, hdr_t *h) {
    int i;
    for(i = 0; i < nbytes; i++) {
        if(p[i] != SENTINAL) {
            int offs = p - (char *)b_alloc_ptr (h) + i;
            ck_error(h, "block %p corrupted at offset %d\n", p, offs);
            trace("\tWrote block after free!\n");
            return 0;
        }
    }

    return 1;
}

static void mark_mem(void *p, unsigned nbytes) {
    memset(p, SENTINAL, nbytes);
}

/*
 * check that:
 *  1. header is not corrupted (checksum passes).
 *  2. redzone 1 and redzone 2 are not corrupted.
 *
 */
static int check_block(hdr_t *h) {
    // short circuit the checks.
    return check_hdr(h)
        && check_mem(b_rz1_ptr(h), REDZONE, h) 
        && check_mem(b_rz2_ptr(h), b_rz2_nbytes(h), h);
}

/*
 *  give an error if so.
 *  1. header is in allocated state.
 *  2. allocated block does not pass checks.
 */
void (ckfree)(void *addr, const char *file, const char *func, unsigned lineno) {
    hdr_t *h = b_addr_to_hdr(addr);

    demand(heap, not initialized?);
    trace("freeing %p\n", addr);
    if (!check_block(h))
        return;
    h->free_loc = (src_loc_t) {
        .file = file,
        .func = func,
        .lineno = lineno,
    };
    h->state = FREED;
    mark_mem (b_alloc_ptr(h), h->nbytes_alloc);
    h->cksum = hdr_cksum (h);
}

// check if nbytes + overhead causes an overflow.
void *(ckalloc)(uint32_t nbytes, const char *file, const char *func, unsigned lineno) {
    hdr_t *h = 0;
    void *ptr = 0;

    demand(heap, not initialized?);
    trace("allocating %d bytes\n", nbytes);

    unsigned tot = pi_roundup(nbytes, 8);
    unsigned n = tot + OVERHEAD_NBYTES;
    
    // this can overflow.
    if(n < nbytes)
        trace_panic("size overflowed: %d bytes is too large\n", nbytes);

    if((heap + n) >= heap_end)
        trace_panic("out of memory!  have only %d left, need %d\n", 
            heap_end - heap, n);
    
    h = (hdr_t *) heap;
    h->nbytes_alloc = nbytes;
    h->nbytes_rem = tot - nbytes;
    h->state = ALLOCED;
    h->alloc_loc = (src_loc_t) { .file = file, .func = func, .lineno = lineno};
    h->refs_start = 1;
    h->refs_middle = 0;
    mark_mem (b_rz1_ptr (h), REDZONE);
    mark_mem (b_rz2_ptr (h), b_rz2_nbytes (h));
    h->cksum = hdr_cksum (h);

    heap += n;
    assert(check_block(h));
    ptr = b_alloc_ptr (h);
    trace("ckalloc:allocated %d bytes, (total=%d), ptr=%p\n", nbytes, n, ptr);
    return ptr;
}

// integrity check the allocated / freed blocks in the heap
// if the header of a block is corrupted, just return.
// return the error count.
int ck_heap_errors(void) {
    unsigned alloced = heap - heap_start;
    unsigned left = heap_end - heap;

    trace("going to check heap: %d bytes allocated, %d bytes left\n", 
            alloced, left);
    unsigned nerrors = 0;
    unsigned nblks = 0;


    for (hdr_t *block = ck_first_hdr(); block; block = ck_next_hdr(block))
    {
        if (!check_hdr(block))
        {
            nerrors++;
            nblks++;
            break;
        }
        
        if (!check_mem(b_rz1_ptr(block), REDZONE, block) || !check_mem(b_rz2_ptr(block), b_rz2_nbytes(block), block))
            nerrors++;
        else if (block->state == FREED)
        {
            if (!check_free_write (b_alloc_ptr (block), block->nbytes_alloc, block))
                nerrors++;
        }
        
        nblks++;
    }


    if(nerrors)
        trace("checked %d blocks, detected %d errors\n", nblks, nerrors);
    else
        trace("SUCCESS: checked %d blocks, detected no errors\n", nblks);
    return nerrors;
}

hdr_t *ck_first_hdr(void) {
    hdr_t *hdr = (hdr_t *) heap_start;
    return check_hdr(hdr) ? hdr : NULL;
}

hdr_t *ck_next_hdr(hdr_t *p) {
    hdr_t *hdr = (hdr_t *) (((char *)p) + p->nbytes_alloc + p->nbytes_rem + OVERHEAD_NBYTES);
    return hdr < (hdr_t *) heap && check_hdr(hdr) ? hdr : NULL;
}
