#pragma once

#include <systemc>
#include <iostream>
#include <cstdint>
#include <getopt.h>
#include <string>
#include "tb.hpp"
#include <cstring>
#include <fstream>
#include "utils.hpp"
#include <sys/mman.h> // for mmap
#include <unistd.h>   // for fork
using namespace sc_core;

class LinuxMode : public EmulatorMode
{
    Testbench *tb;
    SharedState *share_state;

public:
    LinuxMode(Config c, SharedState *share_state)
    {
        cfg = c;
        this->share_state = share_state;
    }
    void init() override
    {
        tb = new Testbench("testbench", 0x80000000);
        tb->tb_state.share_state = share_state;
        load_to_ram(cfg.sbi_path.c_str(), tb, 0x80000000);
        load_to_ram(cfg.dtb_path.c_str(), tb, 0x80100000);
        load_to_ram(cfg.linux_path.c_str(), tb, 0x80400000);
        // OpenSBI fw_jump
        // load_to_ram("/home/hello/workspace/RISCV/opensbi/build/platform/generic/firmware/fw_jump.bin", tb, 0x80000000);

        // // Device Tree Blob (Hardware description)
        // load_to_ram("/home/hello/workspace/RISCV/sw_sim/build/minimal.dtb", tb, 0x80100000);

        // // Linux Kernel (OpenSBI jumps here)
        // load_to_ram("/home/hello/workspace/RISCV/linux/arch/riscv/boot/Image", tb, 0x80400000);

        while (!share_state->start)
        {
            sleep(1);
        }
    }
};

class ArchTestMode : public EmulatorMode
{
    Testbench_RVARCHTEST *tb;

public:
    ArchTestMode(Config c) { cfg = c; }
    void init() override
    {
        if (cfg.sig_start != 0 || cfg.sig_end != 0)
        {
            std::cout << cfg.sig_path << "\n";
            std::cout << "Signature start " << std::hex << cfg.sig_start << " end " << cfg.sig_end << "\n ";
        }

        LoadedELF elf = load_elf32(cfg.elf_path);

        tb = new Testbench_RVARCHTEST("arch_tb", elf.entry, elf.segments[1].vaddr);

        for (auto &seg : elf.segments)
        {
            uint32_t addr = seg.vaddr;
            uint32_t base = tb->memory.ram.find_seg(addr);
            tb->memory.ram.build_seg(base);
            std::vector<uint8_t> *mem = &tb->memory.ram.seg_table[base];
            for (uint32_t i = 0; i < seg.memsz; i++)
            {
                uint32_t base_tmp = tb->memory.ram.find_seg(addr + i);

                if (base != base_tmp)
                {
                    base = base_tmp;
                    tb->memory.ram.build_seg(base);
                    mem = &tb->memory.ram.seg_table[base];
                }
                (*mem)[addr + i - base] = seg.mem[i];
            }
        }
    }
    void execute() override
    {
        if (cfg.max_time <= 0)
        {
            sc_start();
        }
        else
        {
            sc_start(cfg.max_time, SC_NS);
        }
        if (cfg.sig_path.size() != 0)
        {
            std::ofstream out(cfg.sig_path);
            if (!out.is_open())
            {
                std::cerr << "Fail to Open Signature\n";
                return;
            }
            for (uint32_t addr = cfg.sig_start; addr < cfg.sig_end; addr += 4)
            {
                uint32_t base = tb->memory.ram.find_seg(addr);
                uint32_t data = 0;
                for (int i = 3; i >= 0; i--)
                {
                    data <<= 8;
                    data |= tb->memory.ram.seg_table[base][addr - base + i]; // Access your internal storage
                }

                out << std::hex << std::setw(8) << std::setfill('0') << data << std::endl;
            }
            out.close();
        }
    }
};
int sc_main(int argc, char *argv[])
{
    try
    {
        Config cfg = parse_args(argc, argv);

        std::unique_ptr<EmulatorMode> runner;

        if (cfg.mode == "linux")
        {
            SharedState *share_state = setup_shared_memory();
            pid_t pid = fork();
            if (pid == 0)
            {
                run_ui_console(share_state);
            }
            else
            {
                runner = std::make_unique<LinuxMode>(cfg, share_state);
                runner->init();
                runner->execute();
            }
            if ((munmap(share_state, sizeof(SharedState)) == -1))
            {
                perror("munmap  failed");
            }
        }
        else if (cfg.mode == "arch-test" || cfg.mode == "riscv-tests")
        {
            runner = std::make_unique<ArchTestMode>(cfg);
            runner->init();
            runner->execute();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "\x1B[31m[FATAL ERROR]\033[0m " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
