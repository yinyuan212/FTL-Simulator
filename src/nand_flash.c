#include "nand_flash.h"
#include "ftl_mapping.h" // For INVALID_PPA
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NandBlock *flash_memory = NULL;

void nand_init(void) {
    if (flash_memory != NULL) {
        return;
    }
    
    flash_memory = (NandBlock *)calloc(NAND_TOTAL_BLOCKS, sizeof(NandBlock));
    if (!flash_memory) {
        fprintf(stderr, "Failed to allocate memory for NAND flash.\n");
        exit(1);
    }
    
    for (int b = 0; b < NAND_TOTAL_BLOCKS; b++) {
        flash_memory[b].erase_count = 0;
        flash_memory[b].is_bad_block = false;
        
        for (int p = 0; p < NAND_PAGES_PER_BLOCK; p++) {
            memset(flash_memory[b].pages[p].data, 0xFF, NAND_BYTES_PER_PAGE);
            memset(flash_memory[b].pages[p].oob, 0xFF, NAND_OOB_BYTES_PER_PAGE);
            flash_memory[b].pages[p].status = PAGE_FREE;
            flash_memory[b].pages[p].logical_page_addr = INVALID_PPA;
        }
    }
    
    printf("NAND Flash initialized: %d Blocks, %d Pages/Block\n", NAND_TOTAL_BLOCKS, NAND_PAGES_PER_BLOCK);
}

NandStatus nand_read_page(uint32_t physical_block, uint32_t physical_page, uint8_t *buffer) {
    if (physical_block >= NAND_TOTAL_BLOCKS || physical_page >= NAND_PAGES_PER_BLOCK) {
        return NAND_ERR_OUT_OF_BOUNDS;
    }
    
    if (flash_memory[physical_block].is_bad_block) {
        return NAND_ERR_BAD_BLOCK;
    }
    
    memcpy(buffer, flash_memory[physical_block].pages[physical_page].data, NAND_BYTES_PER_PAGE);
    return NAND_SUCCESS;
}

NandStatus nand_program_page(uint32_t physical_block, uint32_t physical_page, const uint8_t *buffer, uint32_t lpa) {
    if (physical_block >= NAND_TOTAL_BLOCKS || physical_page >= NAND_PAGES_PER_BLOCK) {
        return NAND_ERR_OUT_OF_BOUNDS;
    }
    
    NandPage *page = &flash_memory[physical_block].pages[physical_page];
    
    if (page->status != PAGE_FREE) {
        return NAND_ERR_PAGE_NOT_FREE;
    }
    
    memcpy(page->data, buffer, NAND_BYTES_PER_PAGE);
    page->status = PAGE_VALID;
    page->logical_page_addr = lpa;
    
    return NAND_SUCCESS;
}

NandStatus nand_erase_block(uint32_t physical_block) {
    if (physical_block >= NAND_TOTAL_BLOCKS) {
        return NAND_ERR_OUT_OF_BOUNDS;
    }
    
    NandBlock *blk = &flash_memory[physical_block];
    
    if (blk->erase_count > NAND_MAX_ERASE_COUNT) {
        blk->is_bad_block = true;
        return NAND_ERR_BLOCK_WORN_OUT;
    }
    
    for (int p = 0; p < NAND_PAGES_PER_BLOCK; p++) {
        memset(blk->pages[p].data, 0xFF, NAND_BYTES_PER_PAGE);
        memset(blk->pages[p].oob, 0xFF, NAND_OOB_BYTES_PER_PAGE);
        blk->pages[p].status = PAGE_FREE;
        blk->pages[p].logical_page_addr = INVALID_PPA;
    }
    
    blk->erase_count++;
    return NAND_SUCCESS;
}

void nand_cleanup(void) {
    if (flash_memory) {
        free(flash_memory);
        flash_memory = NULL;
    }
}

NandStatus nand_mark_page_invalid(uint32_t physical_block, uint32_t physical_page) {
    if (physical_block >= NAND_TOTAL_BLOCKS || physical_page >= NAND_PAGES_PER_BLOCK) {
        return NAND_ERR_OUT_OF_BOUNDS;
    }
    
    // Can only invalidate a valid page
    if (flash_memory[physical_block].pages[physical_page].status != PAGE_VALID) {
        // This is not strictly an error, but might indicate an FTL bug
        // For now, we allow it
    }

    flash_memory[physical_block].pages[physical_page].status = PAGE_INVALID;
    return NAND_SUCCESS;
}

PageStatus nand_get_page_status(uint32_t physical_block, uint32_t physical_page) {
    // Note: No bounds check here for performance, assuming caller is trusted.
    // In a real-world scenario, this might need a check.
    return flash_memory[physical_block].pages[physical_page].status;
}

NandStatus nand_read_page_metadata(uint32_t physical_block, uint32_t physical_page, uint32_t *lpa) {
    if (physical_block >= NAND_TOTAL_BLOCKS || physical_page >= NAND_PAGES_PER_BLOCK) {
        return NAND_ERR_OUT_OF_BOUNDS;
    }
    if (flash_memory[physical_block].is_bad_block) {
        return NAND_ERR_BAD_BLOCK;
    }
    
    *lpa = flash_memory[physical_block].pages[physical_page].logical_page_addr;
    return NAND_SUCCESS;
}
