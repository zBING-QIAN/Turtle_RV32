#pragma once
#include <systemc>
#include <iostream>
#include "cpu.hpp"
#include <cstring>
#include "memory.hpp"
#include <sys/mman.h> // for mmap
using namespace sc_core;

struct SharedState
{
    bool force_run;
    bool timeirq;
    bool force_stop;
    bool breakpoint;
    bool start;
    uint32_t brk_pc;
    uint32_t new_step_cycles;
    uint32_t new_print_cycles;
    char uart_buf[128];
};

SharedState *setup_shared_memory()
{
    auto *state = static_cast<SharedState *>(mmap(NULL, sizeof(SharedState),
                                                  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    if (state == MAP_FAILED)
    {
        perror("mmap failed");
        exit(1);
    }
    std::memset(state, 0, sizeof(SharedState));
    return state;
}
struct TB_State
{
    SharedState *share_state;
    bool state = 0; // 0 : stop, 1 : run
    // bool *force_run = 0, *timeirq = 0, *force_stop = 0, *breakpoint = 0;
    // uint32_t *brk_pc = 0;
    // uint32_t *new_step_cycles = 0;
    int step_cycles = 0;
    // uint32_t *new_print_cycles = 0;
    uint32_t print_cycles = 0;

    bool next_cycle(uint32_t pc)
    {
        if (share_state->new_step_cycles > 0)
        {
            std::cerr << "remain cycles : " << step_cycles << " cycles\n";
            step_cycles = share_state->new_step_cycles;
            share_state->new_step_cycles = 0;
            std::cerr << "new cycles : " << step_cycles << " cycles\n";
        }
        if (share_state->new_print_cycles > 0)
        {
            print_cycles = share_state->new_print_cycles;
            share_state->new_print_cycles = 0;
            std::cerr << "print : " << print_cycles << " cycles\n";
        }

        if (state == 0)
        {
            if (share_state->force_run)
            {
                state = 1;
                step_cycles = -1;
            }
            else if (step_cycles > 0)
            {
                state = 1;
            }
        }
        else
        {
            if (share_state->force_stop)
            {
                state = 0;
                step_cycles = 0;
            }
            else if (share_state->breakpoint && share_state->brk_pc == pc)
                state = 0;
            else if (step_cycles == 0)
            {
                state = 0;
            }
        }
        if (print_cycles > 0)
            print_cycles -= state;
        if (step_cycles > 0)
            step_cycles -= state;
        if (share_state->force_run)
        {
            std::cerr << "run\n";
            share_state->force_run = 0;
        }
        if (share_state->force_stop)
        {
            share_state->force_stop = 0;
            std::cerr << "stop\n";
        }
        return state;
    }
};
SC_MODULE(Testbench)
{
    TB_State tb_state;
    // char *uart_buf;
    sc_clock hartbeats;
    sc_signal<bool> clk;
    sc_signal<bool> rst;

    sc_signal<DMEM_IN> din;
    sc_signal<MEM_OUT> dout;
    sc_signal<IMEM_IN> iin;
    sc_signal<MEM_OUT> iout;
    sc_signal<uint64_t> mtime, mtimecmp;
    CPU cpu;
    UART_DEV uart;
    CLINT_DEV clint;
    PLIC_DEV plic;
    Memory<1024 * 1024 * 4> memory;
    SC_HAS_PROCESS(Testbench);
    Testbench(sc_module_name name, uint32_t entry) : sc_module(name), hartbeats("clk", 10, SC_NS), memory("memory"), cpu("cpu", entry, &clint)
    {
        plic.uart = &uart;
        memory.ram.base = 0x80000000;
        memory.devices.push_back(&uart);
        memory.devices.push_back(&clint);
        memory.devices.push_back(&plic);
        memory.clk(clk);
        memory.rst(rst);
        memory.din(din);
        memory.dout(dout);
        memory.iin(iin);
        memory.iout(iout);
        cpu.clk(clk);
        cpu.rst(rst);
        cpu.mem_to_dmmu(dout);
        cpu.dmmu_to_mem(din);
        cpu.mem_to_immu(iout);
        cpu.immu_to_mem(iin);
        cpu.exec->plic = &plic;
        SC_THREAD(reset);
        SC_METHOD(plat_update);
        sensitive << clk.posedge_event();
        SC_METHOD(clock);
        sensitive << hartbeats.posedge_event() << hartbeats.negedge_event();
    }
    void clock()
    {

        if (tb_state.share_state->force_run && hartbeats.read())
        {
            std::cerr << "[DEBUG] cur_time = " << clint.mtime << " mtimecmp = " << clint.mtimecmp << " stimecmp = " << ((((uint64_t)cpu.exec->csr_regs[CSR_STIMECMPH].read()) << 32) | cpu.exec->csr_regs[CSR_STIMECMP].read()) << "\n";
            std::cerr << "[DEBUG] plic mirq= " << plic.core0_m_irq << " plic sirq= " << plic.core0_s_irq << "\n";
            std::cerr << "[DEBUG] UART IIR : " << uart.iir << ", UART IER: " << uart.ier << ", UART LCR: " << uart.lcr << "\n";
        }
        cpu.print_en = tb_state.print_cycles > 0;
        clk.write(hartbeats.read() && tb_state.next_cycle(cpu.ex_pc));
        // std::cerr << hartbeats.read() << " " << clk.read() << "\n";
    }
    void plat_update()
    {
        //     std::cerr << "mtime = " << clint.mtime << std::endl;
        if (tb_state.share_state->timeirq)
        {
            uint64_t tmp = ((((uint64_t)cpu.exec->csr_regs[CSR_STIMECMPH].read()) << 32) | cpu.exec->csr_regs[CSR_STIMECMP].read());
            std::cerr << "cur_time = " << clint.mtime << " mtimecmp = " << clint.mtimecmp << " stimecmp = " << tmp << "\n";
            tb_state.share_state->timeirq = 0;

            clint.mtime = tmp;
            std::cerr << "cur_time = " << clint.mtime << " mtimecmp = " << clint.mtimecmp << " stimecmp = " << tmp << "\n";
        }
        else
            clint.mtime++;
        int len = std::strlen(tb_state.share_state->uart_buf);
        if (len)
        {
            uart.keyboard_input(tb_state.share_state->uart_buf, len);
            for (int i = len - 1; i >= 0; i--)
                tb_state.share_state->uart_buf[i] = 0;
        }
        plic.update();
    }
    void reset()
    {
        rst.write(1);
        wait(30, SC_NS); // async reset release
        rst.write(0);
    }
};

SC_MODULE(Testbench_RVARCHTEST)
{
    sc_clock clk;
    sc_signal<bool> rst;

    sc_signal<DMEM_IN> din;
    sc_signal<MEM_OUT> dout;
    sc_signal<IMEM_IN> iin;
    sc_signal<MEM_OUT> iout;
    sc_signal<uint64_t> mtime, mtimecmp;
    CPU cpu;
    CLINT_DEV clint;

    RVTEST_DEV rvtest;
    Memory<1024 * 1024 * 4> memory;
    SC_HAS_PROCESS(Testbench_RVARCHTEST);
    Testbench_RVARCHTEST(sc_module_name name, uint32_t entry, uint32_t tohost_addr) : sc_module(name), clk("clk", 10, SC_NS), memory("memory"), rvtest(tohost_addr), cpu("cpu", entry, &clint)
    {
        memory.devices.insert(memory.devices.begin(), &rvtest);
        memory.devices.push_back(&clint);
        memory.clk(clk);
        memory.rst(rst);
        memory.din(din);
        memory.dout(dout);
        memory.iin(iin);
        memory.iout(iout);
        cpu.clk(clk);
        cpu.rst(rst);
        cpu.mem_to_dmmu(dout);
        cpu.dmmu_to_mem(din);
        cpu.mem_to_immu(iout);
        cpu.immu_to_mem(iin);
        SC_THREAD(reset);
        SC_METHOD(clock);
        sensitive << clk.posedge_event();
    }
    void clock()
    {
        // std::cerr << "mtime = " << clint.mtime << std::endl;
        clint.mtime++;
    }
    void reset()
    {
        rst.write(1);
        wait(30, SC_NS); // async reset release
        rst.write(0);
    }
};
