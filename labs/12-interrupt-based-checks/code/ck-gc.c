/*************************************************************************
 * engler, cs240lx: Purify/Boehm style leak checker/gc starter code.
 *
 * We've made a bunch of simplifications.  Given lab constraints:
 * people talk about trading space for time or vs. but the real trick 
 * is how to trade space and time for less IQ needed to see something 
 * is correct. (I haven't really done a good job of that here, but
 * it's a goal.)
 *
 */
#include "rpi.h"
#include "rpi-constants.h"
#include "ckalloc-internal.h"

/****************************************************************
 * code you implement is below.
 */

static struct heap_info info;

// quick check that the pointer is between the start of
// the heap and the last allocated heap pointer.  saves us 
// walk through all heap blocks.
//
// we could warn if the pointer is within some amount of slop
// so that you can detect some simple overruns?
static int in_heap(void *p) {
    info = heap_info();
    return p >= info.heap_start && p <= info.heap_end;
}

// given potential address <addr>, returns:
//  - 0 if <addr> does not correspond to an address range of 
//    any allocated block.
//  - the associated header otherwise (even if freed: caller should
//    check and decide what to do in that case).
//
// XXX: you'd want to abstract this some so that you can use it with
// other allocators.  our leak/gc isn't really allocator specific.
static hdr_t *is_ptr(uint32_t addr) {
    void *p = (void *)addr;
    
    if(!in_heap(p))
        return NULL;

    for (hdr_t* hdr = ck_first_hdr(); hdr; hdr = ck_next_hdr(hdr)) 
    {
        // If p is before current header, already passed and not found
        if (p < b_alloc_ptr(hdr))
            return NULL;
        // If p is in current alloacted block
        else if (p >= b_alloc_ptr(hdr) 
                 && p <= (void *)((char *)b_alloc_ptr(hdr) + hdr->nbytes_alloc))
            return hdr;
    }

    // output("could not find pointer %p\n", addr);
    return NULL;
}

// mark phase:
//  - iterate over the words in the range [p,e], marking any block 
//    potentially referenced.
//  - if we mark a block for the first time, recurse over its memory
//    as well.
//
// NOTE: if we have lots of words, could be faster with shadow memory / a lookup
// table.  however, given our small sizes, this stupid search may well be faster :)
// If you switch: measure speedup!
//
#include "libc/helper-macros.h"
static void mark(uint32_t *p, uint32_t *e) {
    assert(p<e);
    // maybe keep this same thing?
    assert(aligned(p,4));
    assert(aligned(e,4));

    for (;p <= e; p++)
    {
        
        hdr_t *block = is_ptr(*p);
        if (!block)
            continue;
        (block->mark)++;
        if (*p == (uint32_t)b_alloc_ptr(block))
            (block->refs_start)++;
        else 
            (block->refs_middle)++;
        update_checksum(block);

        // If block has not yet been explored
        if (block->mark == 1)
        {
            mark((uint32_t *)b_alloc_ptr(block),
                    (uint32_t *)((char *)b_alloc_ptr(block) + block->nbytes_alloc));

        }
    }
}

// do a sweep, warning about any leaks.
//
//
static unsigned sweep_leak(int warn_no_start_ref_p) {
	unsigned nblocks = 0, errors = 0, maybe_errors=0;
	output("---------------------------------------------------------\n");
	output("checking for leaks:\n");

    for (hdr_t *block = ck_first_hdr(); block; block = ck_next_hdr(block))
    {
        nblocks++;
        if (block->state == FREED) 
            continue;
        if (!block->mark)
        {
            trace("ERROR:GC:DEFINITE LEAK of %x\n", b_alloc_ptr(block));
            errors++;
        }
        else if (block->refs_start == 0 && warn_no_start_ref_p)
        {
            trace("ERROR:GC:MAYBE LEAK of %x (no pointer to the start)\n", b_alloc_ptr(block));
            maybe_errors++;
        }
    }

	trace("\tGC:Checked %d blocks.\n", nblocks);
	if(!errors && !maybe_errors)
		trace("\t\tGC:SUCCESS: No leaks found!\n");
	else
		trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n", 
						errors, maybe_errors);
	output("----------------------------------------------------------\n");
	return errors + maybe_errors;
}

// write the assembly to dump all registers.
// need this to be in a seperate assembly file since gcc 
// seems to be too smart for its own good.
void dump_regs(uint32_t *v, ...);

// a very slow leak checker.
static void mark_all(void) {
    // slow: should not need this: remove after your code
    // works.
    for(hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        h->mark = h->refs_start = h->refs_middle = 0;
        update_checksum(h);
    }

    // get start and end of heap so we can do quick checks
    struct heap_info i = heap_info();

	// pointers can be on the stack, in registers, or in the heap itself.

    // get all the registers.
    uint32_t regs[16];
    // should kill caller-saved registers
    dump_regs(regs);
    assert(regs == (uint32_t *)regs[0]);
    mark(regs, &regs[14]);

    // mark the stack: we are assuming only a single
    // stack.  note: stack grows down.
    uint32_t *stack_top = (void*)STACK_ADDR;
    uint32_t *stack_bottom = (void *)regs[13];

    mark(stack_bottom, stack_top);

    // these symbols are defined in our memmap
	extern uint32_t __bss_start__, __bss_end__;
	mark(&__bss_start__, &__bss_end__);

	extern uint32_t __data_start__, __data_end__;
	mark(&__data_start__, &__data_end__);
}


/***********************************************************
 * testing code: we give you this.
 */

// return number of bytes allocated?  freed?  leaked?
// how do we check people?
unsigned ck_find_leaks(int warn_no_start_ref_p) {
    mark_all();
    return sweep_leak(warn_no_start_ref_p);
}

// used for tests.  just keep it here.
void check_no_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");
    if(ck_find_leaks(1))
        panic("GC: should have no leaks!\n");
    else
        trace("GC: SUCCESS: no leaks!\n");
    gcc_mb();
}

// used for tests.  just keep it here.
unsigned check_should_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");

    unsigned nleaks = ck_find_leaks(1);
    if(!nleaks)
        panic("GC: should have leaks!\n");
    else
        trace("GC: SUCCESS: found %d leaks!\n", nleaks);
    gcc_mb();
    return nleaks;
}

void implement_this(void) { 
    panic("did not implement dump_regs!\n");
}

// similar to sweep_leak: but mark unreferenced blocks as FREED.
static unsigned sweep_free(void) {
	unsigned nblocks = 0, nfreed=0, nbytes_freed = 0;
	output("---------------------------------------------------------\n");
	output("compacting:\n");

    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        nbytes_freed += h->nbytes_alloc;
        if (h->state == ALLOCED && h->refs_start == 0 && h->refs_middle == 0) {
            void *ptr = b_alloc_ptr(h);
            // trace("\tGC:DEFINITE LEAK of %x\n", ptr);
            trace("GC:FREEing ptr=%x\n", ptr);
            ckfree(ptr);
            nfreed++;
        }

        nblocks++;
    }
    // i used this when freeing.


	trace("\tGC:Checked %d blocks, freed %d, %d bytes\n", nblocks, nfreed, nbytes_freed);

    struct heap_info info = heap_info();
	trace("\tGC:total allocated = %d, total deallocated = %d\n", 
                                info.nbytes_alloced, info.nbytes_freed);
	output("----------------------------------------------------------\n");
    return nbytes_freed;
}

unsigned ck_gc(void) {
    mark_all();
    unsigned nbytes = sweep_free();

    // perhaps coalesce these and give back to heap.  will have to modify last.

    return nbytes;
}