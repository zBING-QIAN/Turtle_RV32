

#  Booting RISC-V Linux

This guide explains how to compile the full software stack and boot Linux on the emulator. The boot process follows the standard RISC-V boot flow: **OpenSBI → Linux Kernel → BusyBox (initramfs).**

## 1. Build OpenSBI (Firmware)

OpenSBI acts as the Supervisor Binary Interface. We configure it to "jump" to the Linux kernel and locate the Device Tree Blob (DTB).


```
git clone https://github.com/riscv-software-src/opensbi.git
cd opensbi
make CROSS_COMPILE=riscv64-linux-gnu- \
     PLATFORM=generic \
     PLATFORM_RISCV_XLEN=32 \
     PLATFORM_RISCV_ISA=rv32ima_zicsr_zifencei \
     PLATFORM_RISCV_ABI=ilp32 \
     FW_JUMP_ADDR=0x80400000 \
     FW_JUMP_FDT_ADDR=0x80100000

```

-   **FW_JUMP_ADDR**: Where the Linux kernel image is loaded.
    
-   **FW_JUMP_FDT_ADDR**: Where the DTB is loaded.
    

## 2. Build BusyBox (Root Filesystem)

BusyBox provides the core user-space utilities. **Note:** Use the `linux-gnu` toolchain for user-space applications.

```
git clone https://github.com/mirror/busybox.git
cd busybox
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv defconfig

# Configuration: 
# 1. Disable Network/Mail in menuconfig
# 2. Settings -> Uncheck Hardware accelerated SHA1/SHA256
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv menuconfig
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv -j$(nproc)
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv install

# Create filesystem structure
cd _install
mkdir -p sys proc dev
sudo mknod -m 600 dev/console c 5 1
sudo mknod -m 666 dev/ttyS0 c 4 64

# Add the init script from tests/linux-test/init
cp PATH_TO_REPO/tests/linux-test/init .

# Pack the Initramfs
find . | cpio -H newc -o > ../rootfs.cpio

```

## 3. Compile Linux Kernel

We use Linux version `v6.19-rc2`.


```
git clone https://github.com/torvalds/linux.git
cd linux
git checkout v6.19-rc2-2-gb927546677c8

# Load the provided .config
cp PATH_TO_REPO/tests/linux-test/.config .config

# Set the Initramfs path in Menuconfig:
# General setup -> Initial RAM filesystem -> (Point to your rootfs.cpio)
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv menuconfig
make CROSS_COMPILE=riscv32-unknown-linux-gnu- ARCH=riscv -j$(nproc)

```

## 4. Prepare Device Tree (DTB)

The Device Tree describes the hardware (CPU, Memory, UART) to the Linux kernel.


```
dtc -I dts -O dtb -o tests/linux-test/minimal.dtb tests/linux-test/minimal_rv32.dts

```

## 5. Running the Emulator

Once all binaries are ready, launch the emulator using the `--mode=linux` flag.


```
./emu --mode=linux \
      --linux=./linux/arch/riscv/boot/Image \
      --sbi=./opensbi/build/platform/generic/firmware/fw_jump.bin \
      --dtb=./tests/linux-test/minimal.dtb
```

Type ":r" to start running the code.
Type ":p num_cycles" to print cpu status for num_cycles times.
Type ":stop" to stop running.
Type ":step num_cycles" to run num_cycles times. 
Type ":c" to continue.
Send message to UART if ":" is not the first char.

