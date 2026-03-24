#ifndef FTL_MAPPING_H
#define FTL_MAPPING_H

#include <stdbool.h>
#include <stdint.h>

#include "nand_flash.h"

#define INVALID_PPA 0xFFFFFFFF
// Page-Level Mapping Table (L2P)
#define NUM_LOGICAL_PAGES                                                      \
  (NAND_TOTAL_BLOCKS * NAND_PAGES_PER_BLOCK * 8 / 10) // 20% over-provisioning

// Initialize mapping table
void ftl_init(void);

// Write data to a logical page
NandStatus ftl_write(uint32_t lpa, const uint8_t *data);

// Read data from a logical page
NandStatus ftl_read(uint32_t lpa, uint8_t *buffer);

// Update the mapping for a given LPA (for GC use)
void ftl_update_mapping(uint32_t lpa, uint32_t ppa);

// The FTL's current write-active block, needed by GC
extern uint32_t current_active_block;

// Free resources
void ftl_cleanup(void);

#endif // FTL_MAPPING_H
