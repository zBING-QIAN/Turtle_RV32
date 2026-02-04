#pragma once
#include <systemc>
#include <cstdint>
using namespace sc_core;

SC_MODULE(PreFetch)
{
    sc_in<bool> clk;
    sc_in<bool> immu_ready;
    // sc_in<bool> stall;
    // sc_in<uint32_t> ex_pc; // ex_pc+4 / branch / trap occur / m(s)ret
    sc_in<uint32_t> npc;
    sc_in<uint32_t> immu_pc;
    sc_in<uint32_t> if_id_pc;
    sc_in<uint32_t> id_ex_pc;
    sc_in<bool> if_id_flush;
    sc_in<bool> if_id_accept;
    sc_in<bool> id_ex_flush;
    sc_in<bool> immu_flush;

    sc_out<uint32_t> fetch_pc;
    SC_HAS_PROCESS(PreFetch);
    PreFetch(sc_module_name name) : sc_module(name)
    {
        SC_METHOD(next_pc);
        sensitive << clk.pos();
    }

    void next_pc()
    {
        uint32_t pc = npc.read();

        // uint32_t nextpc = immu_pc.read();
        uint32_t nextpc = fetch_pc.read();

        // std::cout << "npc debug   accept" << if_id_accept.read() << " " << id_ex_pc.read() << " " << if_id_pc.read() << " " << immu_pc.read() << " " << pc << " " << fetch_pc << "\n";
        if (((id_ex_flush.read() || id_ex_pc.read() != pc) &&
             (if_id_flush.read() || if_id_pc.read() != pc) &&
             (immu_flush.read() || (immu_pc.read() != pc && immu_pc.read() != pc + 2)) &&
             (fetch_pc.read() != pc || !if_id_accept.read())))
        {
            // std::cout << "npc debug " << id_ex_flush.read() << " " << if_id_flush.read() << " " << immu_flush.read() << " " << pc << " " << fetch_pc << "\n";
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