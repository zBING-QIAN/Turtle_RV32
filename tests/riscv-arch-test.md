# RISC-V Architectural Compliance Testing

This mode uses the [RISCOF](https://github.com/riscv-software-src/riscof) framework to ensure your emulator complies with the RISC-V ISA specifications by comparing its execution signature against the **Spike** reference simulator.
## 1. Prerequisites

You must have `riscof`, `spike`, and the RISC-V GNU toolchain installed.

    # Install RISC-V Config and RISCOF 
    pip3 install git+https://github.com/riscv-software-src/riscv-config.git@dev  

## 2. Environment Setup


    git clone https://github.com/riscv-non-isa/riscv-arch-test.git 
    cd riscv-arch-test
    # Setup Spike as the Reference (DUT) 
    riscof setup --dutname spike

##  3. Configuration
1. Change target and reference model.\
Modify riscv-arch-test/config.ini (see tests/riscv-arch-test/config.ini)
2. Update Spike YAMLs\
`cp tests/riscv-arch-test/spike/spike_isa.yaml riscv-arch-test/spike/spike_isa.yaml`
`cp tests/riscv-arch-test/spike/spike_platform.yaml riscv-arch-test/spike/spike_platform.yaml`
3. Disable Misaligned Access: Spike must be told to behave like my emulator.\
`cp tests/riscv-arch-test/spike/riscof_spike.py riscv-arch-test/spike/riscof_spike.py`
4. Register my emulator as a target for RISCOF\
`cp -r tests/riscv-arch-test/my_cpu riscv-arch-test/my_cpu`

## 4. Running Tests
Run all RV32I_M Tests

    riscof run --config=config.ini  --suite=riscv-test-suite/rv32i_m  --env=riscv-test-suite/env
Run a Specific Test Set

    riscof run --config=config.ini  --suite=riscv-test-suite/rv32i_m/privilege  --env=riscv-test-suite/env