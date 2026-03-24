#include "garbage_collect.h"
#include "ftl_mapping.h"
#include "nand_flash.h"
#include <stdio.h>
#include <stdlib.h> // For malloc/free

#define GC_THRESHOLD_LOW 5

static uint32_t free_block_list[NAND_TOTAL_BLOCKS];
static int free_block_count = 0;

// Need to know the FTL's current active block to avoid collecting it
extern uint32_t current_active_block; // This is a bit of a hack, a better design would pass it as a parameter

void gc_init(void) {
    free_block_count = 0;
    for (uint32_t i = 1; i < NAND_TOTAL_BLOCKS; i++) {
        free_block_list[free_block_count++] = i;
    }
    printf("GC Initialized: %d free blocks available.\n", free_block_count);
}

bool gc_is_needed(void) {
    return free_block_count <= GC_THRESHOLD_LOW;
}

int gc_get_free_block(void) {
    if (free_block_count <= 0) {
        return -1;
    }
    return free_block_list[--free_block_count];
}

void gc_add_free_block(uint32_t block_index) {
    if (free_block_count >= NAND_TOTAL_BLOCKS) return;
    free_block_list[free_block_count++] = block_index;
}

/*
    Greedy Strategy: 挑選 invalid page 最多的 block
*/
NandStatus gc_trigger(void) {
    printf("--- Garbage Collection Triggered ---\n");
    
    // 1. Find Victim Block
    int victim_block = -1;
    int max_invalid_pages = -1;

    for (uint32_t b = 0; b < NAND_TOTAL_BLOCKS; b++) {
        // Don't collect the current active block
        if (b == current_active_block) {
            continue;
        }

        int invalid_pages = 0;
        for (uint32_t p = 0; p < NAND_PAGES_PER_BLOCK; p++) {
            if (nand_get_page_status(b, p) == PAGE_INVALID) {
                invalid_pages++;
            }
        }

        if (invalid_pages > max_invalid_pages) {
            max_invalid_pages = invalid_pages;
            victim_block = b;
        }
    }

    if (victim_block == -1 || max_invalid_pages == 0) {
        printf("GC: No victim block found with invalid pages. Nothing to collect.\n");
        return NAND_SUCCESS;
    }

    printf("GC: Victim block is %d with %d invalid pages.\n", victim_block, max_invalid_pages);

    // 2. Get a new block for copying live pages
    int new_block_idx = gc_get_free_block();
    if (new_block_idx < 0) {
        printf("GC FATAL: Could not get a free block for migration!\n");
        return NAND_ERR_NO_FREE_BLOCKS;
    }
    uint32_t new_block = (uint32_t)new_block_idx;
    uint32_t new_page_offset = 0;

    // 3. Live Page Migration
    uint8_t *page_buffer = (uint8_t *)malloc(NAND_BYTES_PER_PAGE);
    if (!page_buffer) return -1; // Should be a proper error

    for (uint32_t p = 0; p < NAND_PAGES_PER_BLOCK; p++) {
        if (nand_get_page_status(victim_block, p) == PAGE_VALID) {
            uint32_t lpa;
            nand_read_page(victim_block, p, page_buffer);
            nand_read_page_metadata(victim_block, p, &lpa);

            // Program to new location
            if (new_page_offset >= NAND_PAGES_PER_BLOCK) {
                 printf("GC FATAL: Ran out of space in the new block during migration!\n");
                 free(page_buffer);
                 return -1; // Should be a proper error
            }

            nand_program_page(new_block, new_page_offset, page_buffer, lpa);
            
            // Update mapping
            uint32_t new_ppa = new_block * NAND_PAGES_PER_BLOCK + new_page_offset;
            ftl_update_mapping(lpa, new_ppa);
            
            printf("GC: Migrated LPA %u to new PPA %u\n", lpa, new_ppa);
            new_page_offset++;
        }
    }
    free(page_buffer);

    // 4. Erase victim block
    printf("GC: Erasing victim block %d.\n", victim_block);
    nand_erase_block(victim_block);

    // 5. Add victim block back to free list
    gc_add_free_block(victim_block);

    printf("--- Garbage Collection Finished ---\n");
    return NAND_SUCCESS;
}
