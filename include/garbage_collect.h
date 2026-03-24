#ifndef GARBAGE_COLLECT_H
#define GARBAGE_COLLECT_H

#include <stdint.h>
#include <stdbool.h>
#include "nand_flash.h"

// Initialize garbage collection tracking and free block list
void gc_init(void);

// Trigger garbage collection
// Returns the number of blocks reclaimed or an error code
NandStatus gc_trigger(void);

// Check if GC is needed based on the number of free blocks
bool gc_is_needed(void);

// Get a free block from the list for the FTL to use
int gc_get_free_block(void);

// Add a newly erased block back to the free list
void gc_add_free_block(uint32_t block_index);

#endif // GARBAGE_COLLECT_H
