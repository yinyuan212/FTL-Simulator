#ifndef NAND_FLASH_H
#define NAND_FLASH_H

#include <stdint.h>
#include <stdbool.h>

// NAND Flash Memory Configuration
#define NAND_PAGES_PER_BLOCK    64
#define NAND_BYTES_PER_PAGE     4096
#define NAND_OOB_BYTES_PER_PAGE 64
#define NAND_TOTAL_BLOCKS       1024
#define NAND_MAX_ERASE_COUNT    10000

// Status codes for NAND operations
typedef enum {
    NAND_SUCCESS,
    NAND_ERR_OUT_OF_BOUNDS,
    NAND_ERR_BAD_BLOCK,
    NAND_ERR_PAGE_NOT_FREE,
    NAND_ERR_BLOCK_WORN_OUT,
    NAND_ERR_NO_FREE_BLOCKS
} NandStatus;

typedef enum {
    PAGE_FREE,
    PAGE_VALID,
    PAGE_INVALID
} PageStatus;

typedef struct {
    uint8_t data[NAND_BYTES_PER_PAGE];
    uint8_t oob[NAND_OOB_BYTES_PER_PAGE];
    PageStatus status;
    uint32_t logical_page_addr; // LPN stored in OOB
} NandPage;

typedef struct {
    NandPage pages[NAND_PAGES_PER_BLOCK];
    uint32_t erase_count;
    bool is_bad_block;
} NandBlock;

// Flash Memory Interface
void nand_init(void);
NandStatus nand_read_page(uint32_t physical_block, uint32_t physical_page, uint8_t *buffer);
NandStatus nand_program_page(uint32_t physical_block, uint32_t physical_page, const uint8_t *buffer, uint32_t lpa);
NandStatus nand_erase_block(uint32_t physical_block);
void nand_cleanup(void);
NandStatus nand_mark_page_invalid(uint32_t physical_block, uint32_t physical_page);
PageStatus nand_get_page_status(uint32_t physical_block, uint32_t physical_page);
NandStatus nand_read_page_metadata(uint32_t physical_block, uint32_t physical_page, uint32_t *lpa);

#endif // NAND_FLASH_H
