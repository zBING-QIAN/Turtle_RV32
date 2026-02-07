#pragma once
#include <systemc>
#include <cstdint>
using namespace sc_core;

SC_MODULE(PreFetch)
{
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_in<bool> immu_ready;
    sc_in<uint32_t> npc;
    sc_in<bool> if_id_accept;

    sc_in<bool> branch_taken;
    sc_in<bool> flush;
    sc_out<uint32_t> fetch_pc;
    SC_HAS_PROCESS(PreFetch);
    PreFetch(sc_module_name name) : sc_module(name)
    {
        SC_METHOD(next_pc_seq);
        sensitive << clk.pos();
    }

    void next_pc_seq()
    {
        uint32_t pc = npc.read();

        // uint32_t nextpc = immu_pc.read();
        uint32_t nextpc = fetch_pc.read();

        if (branch_taken || flush || rst)
        {
            nextpc = pc;
        }
        else if (if_id_accept.read())
        {
            nextpc = (fetch_pc.read() & ~3) + 4; // TODO : predict pc base on instruction in decode stage
            // std::cout << "npc debug   " << id_ex_pc.read() << " " << if_id_pc.read() << " " << immu_pc.read() << "\n";
        }
        fetch_pc.write(nextpc);
    }
};