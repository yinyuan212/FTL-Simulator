# FTL Simulator

This project is a C-based Flash Translation Layer (FTL) simulator designed to emulate the core functionalities of an SSD controller managing NAND flash memory. It provides a software-level representation of NAND flash operations, address mapping, and essential background tasks like garbage collection and wear leveling.

## Features

- **NAND Flash Emulation (`nand_flash.c` / `nand_flash.h`)**:
  - Simulates physical pages, blocks, and out-of-band (OOB) areas.
  - Implements basic NAND operations: page read, page program, and block erase.
  - Tracks page statuses (free, valid, invalid) and block erase counts.
- **Page-Level FTL Mapping (`ftl_mapping.c` / `ftl_mapping.h`)**:
  - Maintains a logical-to-physical (L2P) mapping table.
  - Transparently handles logical page writes.
  - Includes ~20% over-provisioning (configurable) to aid garbage collection.
- **Garbage Collection (`garbage_collect.c` / `garbage_collect.h`)**:
  - Monitors the pool of free physical blocks.
  - Automatically triggers when the number of free blocks falls below a threshold.
  - Selects victim blocks, copies valid pages to new blocks, updates the L2P mapping, and erases the victim block.
- **Wear Leveling (`wear_leveling.c` / `wear_leveling.h`)**:
  - Framework added for tracking block wear and performing wear leveling operations to evenly distribute erases across the flash memory.
- **Test Workload (`main.c`)**:
  - A comprehensive test suite that simulates writing multiple logical pages to trigger continuous garbage collection.
  - Read-back verification ensures data integrity across GC cycles.

## Project Structure

```
FTL-Simulator/
├── include/
│   ├── ftl_mapping.h
│   ├── garbage_collect.h
│   ├── nand_flash.h
│   └── wear_leveling.h
└── src/
    ├── ftl_mapping.c
    ├── garbage_collect.c
    ├── main.c
    ├── nand_flash.c
    └── wear_leveling.c
```

## How to Build

Since the project mainly consists of standard C source files, you can compile it using `gcc` or `clang`. The executable will be generated inside the `build` folder:

```bash
# From the root directory of the project
gcc -Iinclude src/*.c -o build/ftl_simulator.exe
```

## How to Run

Navigate to the `build` directory and run the executable to see the initialization, simulated workload, garbage collection triggering, and the final verification results:

```bash
cd build
./ftl_simulator.exe
```

## Python Host Emulator (ctypes)

We have also created an industry-style test architecture where the FTL runs as a Windows shared library (`.dll`) and is controlled by a Python script using `ctypes`.

### 1. Compile as a Shared Library

Compile the C code into `build/ftl_simulator.dll`:

```bash
# Ensure build directory exists
mkdir build
gcc -shared -Iinclude src/*.c -o build/ftl_simulator.dll
```

### 2. Run the Test Suites (Host Emulator)

We use Python's built-in `unittest` framework to validate the FTL DLL structure. The tests simulate an industry-standard host sending read/write requests, triggering garbage collection, and verifying data integrity.

To run all test suites, simply execute:

```bash
python -m unittest discover tests -v
```

> **Note on Architecture:** Ensure your `gcc` and `python` share the same architecture (e.g., both 64-bit or both 32-bit). A 64-bit Python environment cannot load a 32-bit `.dll` built with a 32-bit MinGW compiler, so make sure they match!
