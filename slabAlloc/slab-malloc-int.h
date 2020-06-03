#ifndef __SLABMALLOCINT_H__
#define __SLABMALLOCINT_H__

/*   Allocated block set up as follows:
 *   +--------+-----------------+------+------------------+-------------------+
 *   | header | redzone         | data | unused remainder | redzone           |
 *   +--------+-----------------+------+------------------+-------------------+
 * 
 *   Header: Stores meta information about block, defined below
 *   Redzone: Buffer areas with a repeated canary pattern. Patern can be checked to detect memory erros.
 *   Data: Data segment users can write to.
 *   Unused remainder: Additional memory added to user request to maintain byte alignment
 */


typedef struct header
  {
    
    
  } header;

void init_heap (void);

#endif