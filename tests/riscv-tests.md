# RISC-V ISA Unit Testing

This section describes how to build and run the official [riscv-tests](https://github.com/riscv/riscv-tests) suite to verify the core instruction set of the emulator.

## 1. Prerequisites

You must have the RISC-V GNU Toolchain (`riscv64-unknown-elf-`) installed and added to your system path.

    # Set your RISCV environment variable to point to your toolchain installation
    export RISCV=/path/to/riscv64-unknown-elf
    export PATH=$PATH:$RISCV/bin

## 2. Building the Test Suite

    # Clone the repository and submodules 
    git clone https://github.com/riscv/riscv-tests cd riscv-tests 
    git submodule update --init --recursive 
    # Configure and build 
    autoconf 
    ./configure --prefix=$RISCV/target 
    make XLEN=32

##  3. Running Tests
Running a Single Test

    ./emu --mode=unit-test --elf=path/to/riscv-tests/isa/rv32ui-p-add
 Automated Batch Testing

    python3 tests/run_riscv_tests.py

> The two test cases, rv32ui-p-ma_data and rv32ui-v-ma_data, fail because my emulator does not support misaligned access.
