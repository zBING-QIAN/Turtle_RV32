#pragma once
#include <systemc>
#include <cstdint>
// #include "execute.hpp"
using namespace sc_core;

SC_MODULE(RegFile)
{
    sc_vector<sc_signal<uint32_t>> regs;
    // uint32_t regs[32];
    sc_in<bool> clk;
    sc_in<bool> wb_write_rd;
    sc_in<uint32_t> wb_rd_val;
    sc_in<uint8_t> wb_rd;

    SC_HAS_PROCESS(RegFile);
    RegFile(sc_module_name name) : sc_module(name)
    {
        regs.init(32);
        SC_THREAD(write_seq);
        sensitive << clk.pos();
        for (int i = 0; i < 32; i++)
            regs[i] = 0;

        regs[0xb] = 0x80100000;
    }
    void write_seq()
    {
        wait();
        while (1)
        {
            wait();
            // wait(SC_ZERO_TIME);
            if (wb_write_rd.read() && wb_rd.read())
            {
                regs[wb_rd.read()].write(wb_rd_val.read());
            }
        }
    }
    // void monitor()
    // {
    //     if (writes_rd.read())
    //         std::cout << "write reg file at " << rd.read() << " " << rd_val.read() << "\n";
    // }
};