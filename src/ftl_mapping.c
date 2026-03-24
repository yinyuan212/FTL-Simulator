#include "ftl_mapping.h"
#include "garbage_collect.h"
#include "nand_flash.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t *l2p_table = NULL;

// Active block for incoming writes
uint32_t current_active_block = 0;
static uint32_t next_free_page_offset = 0;

void ftl_init(void) {
  l2p_table = (uint32_t *)malloc(NUM_LOGICAL_PAGES * sizeof(uint32_t));
  if (!l2p_table) {
    fprintf(stderr, "Failed to allocate L2P table.\n");
    exit(1);
  }

  // Initialize all logical pages to unmapped
  for (uint32_t i = 0; i < NUM_LOGICAL_PAGES; i++) {
    l2p_table[i] = INVALID_PPA;
  }

  current_active_block = 0;
  next_free_page_offset = 0;
  printf("FTL Mapping Table initialized. Total Logical Pages: %u\n",
         NUM_LOGICAL_PAGES);
}

static NandStatus get_next_free_page(uint32_t *pblock, uint32_t *poffset) {
  if (next_free_page_offset >= NAND_PAGES_PER_BLOCK) {
    if (gc_is_needed()) {
      // 提早檢查：在耗盡所有可用區塊前，先看是否已經達到 GC 的低水位線
      printf("Free blocks running low, triggering GC...\n");
      gc_trigger();
    }

    int new_block = gc_get_free_block();
    if (new_block < 0) {
      printf("FATAL: No free blocks even after GC!\n");
      return NAND_ERR_NO_FREE_BLOCKS;
    }
    
    current_active_block = (uint32_t)new_block;
    next_free_page_offset = 0;
  }

  *pblock = current_active_block;
  *poffset = next_free_page_offset;
  next_free_page_offset++;

  return NAND_SUCCESS;
}

NandStatus ftl_write(uint32_t lpa, const uint8_t *data) {
  if (lpa >= NUM_LOGICAL_PAGES) {
    return NAND_ERR_OUT_OF_BOUNDS;
  }

  uint32_t physical_block, physical_page;
  NandStatus status = get_next_free_page(&physical_block, &physical_page);
  if (status != NAND_SUCCESS) {
    return status;
  }

  // Program the data
  status = nand_program_page(physical_block, physical_page, data, lpa);
  if (status != NAND_SUCCESS) {
    return status;
  }

  // Update mapping table
  uint32_t old_ppa = l2p_table[lpa];
  uint32_t new_ppa = (physical_block * NAND_PAGES_PER_BLOCK) + physical_page;
  l2p_table[lpa] = new_ppa;

  // If the logical page was previously mapped, mark the old physical page as
  // invalid
  if (old_ppa != INVALID_PPA) {
    uint32_t old_block = old_ppa / NAND_PAGES_PER_BLOCK;
    uint32_t old_page = old_ppa % NAND_PAGES_PER_BLOCK;
    nand_mark_page_invalid(old_block, old_page);
  }

  return NAND_SUCCESS;
}

NandStatus ftl_read(uint32_t lpa, uint8_t *buffer) {
  if (lpa >= NUM_LOGICAL_PAGES) {
    return NAND_ERR_OUT_OF_BOUNDS;
  }

  uint32_t ppa = l2p_table[lpa];
  if (ppa == INVALID_PPA) {
    // Read unwritten page, return zeros or 0xFF
    memset(buffer, 0, NAND_BYTES_PER_PAGE);
    return NAND_SUCCESS;
  }

  uint32_t physical_block = ppa / NAND_PAGES_PER_BLOCK;
  uint32_t physical_page = ppa % NAND_PAGES_PER_BLOCK;

  return nand_read_page(physical_block, physical_page, buffer);
}

void ftl_cleanup(void) {
  if (l2p_table) {
    free(l2p_table);
    l2p_table = NULL;
  }
}

void ftl_update_mapping(uint32_t lpa, uint32_t ppa) {
  if (lpa >= NUM_LOGICAL_PAGES) {
    return;
  }
  l2p_table[lpa] = ppa;
}
