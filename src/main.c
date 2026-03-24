#include "ftl_mapping.h"
#include "garbage_collect.h"
#include "nand_flash.h"
// #include "wear_leveling.h"
#include <stdio.h>
#include <stdlib.h>

// We have NUM_LOGICAL_PAGES available, but we'll write to a smaller space
// to ensure we get lots of overwrites, thus creating invalid pages.
#define LBA_SPACE (NUM_LOGICAL_PAGES / 4)

// We'll write enough data to almost certainly trigger GC.
// Total physical pages = NAND_TOTAL_BLOCKS * NAND_PAGES_PER_BLOCK
#define TOTAL_WRITES (NAND_TOTAL_BLOCKS * NAND_PAGES_PER_BLOCK)

int main(void) {
  printf("Initializing FTL Simulator...\n");

  nand_init();
  ftl_init();
  gc_init();

  uint8_t *write_buffer = (uint8_t *)malloc(NAND_BYTES_PER_PAGE);
  if (!write_buffer)
    return 1;

  printf(
      "Starting GC test workload: Writing %d pages to an LBA space of %d...\n",
      TOTAL_WRITES, LBA_SPACE);

  for (int i = 0; i < TOTAL_WRITES; i++) {
    uint32_t lba = i % LBA_SPACE;
    // Put some unique data in the buffer for this write
    write_buffer[0] = (uint8_t)lba;
    write_buffer[1] = (uint8_t)(lba >> 8);

    if (i % 1000 == 0) {
      printf("  Write #%d (LBA %u)...\n", i, lba);
    }

    NandStatus status = ftl_write(lba, write_buffer);
    if (status != NAND_SUCCESS) {
      printf("Error during ftl_write at write #%d. Status: %d. Aborting.\n", i,
             status);
      break;
    }

    // We don't call wear_leveling_check here to keep the log clean for GC test
  }

  printf("Finished workload.\n");

  // --- Verification Step ---
  // Read back the last written data for a few LBAs to see if it's correct
  printf("\nVerifying final data...\n");
  uint8_t *read_buffer = (uint8_t *)malloc(NAND_BYTES_PER_PAGE);
  if (!read_buffer) {
    free(write_buffer);
    return 1;
  }

  int errors = 0;
  for (uint32_t lba = 0; lba < LBA_SPACE; lba += LBA_SPACE / 4) {
    write_buffer[0] = (uint8_t)lba;
    write_buffer[1] = (uint8_t)(lba >> 8);

    ftl_read(lba, read_buffer);

    if (read_buffer[0] != write_buffer[0] ||
        read_buffer[1] != write_buffer[1]) {
      printf("  Verification FAILED for LBA %u!\n", lba);
      errors++;
    } else {
      printf("  Verification PASSED for LBA %u.\n", lba);
    }
  }

  if (errors == 0) {
    printf("All tested LBAs verified successfully!\n");
  } else {
    printf("%d verification errors found.\n", errors);
  }

  printf("\nCleaning up...\n");
  free(write_buffer);
  free(read_buffer);
  ftl_cleanup();
  nand_cleanup();

  return 0;
}
