#pragma once
#include "csr_cfg.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <systemc>
// #include <set>
using namespace sc_core;
#define PMPCOUNT 16
#define PMPLOCK 128
#define PMPA 24
#define PMPX 4
#define PMPW 2
#define PMPR 1
#define PMPOFF 0
#define PMPTOR 8
#define PMPNA4 16
#define PMPNAPOT 24

SC_MODULE(PMP)
{
    sc_vector<sc_signal<uint64_t>> pmpaddr;
    sc_vector<sc_signal<uint8_t>> pmpconfig;
    std::vector<int> pmpentry_id;
    SC_HAS_PROCESS(PMP);
    PMP(sc_module_name name) : sc_module(name)
    {
        pmpconfig.init(PMPCOUNT);
        pmpaddr.init(PMPCOUNT);
    }

    bool dcheck(uint64_t addr, uint8_t op, uint8_t size, uint8_t priv) const
    {
        for (auto i : pmpentry_id)
        {
            bool base_in = 0, top_in = 0;
            uint64_t base = 0, top = 0;
            uint8_t cfg = pmpconfig[i].read();
            // check pmp range
            switch (cfg & PMPA)
            {
            case PMPOFF:
                break;
            case PMPTOR: // TOR
                base = (i) ? pmpaddr[i - 1] << 2 : 0;
                top = pmpaddr[i] << 2;
                base_in = (addr >= base && addr < top);
                top_in = (addr + size - 1 >= base && addr + size - 1 < top);
                break;
            case PMPNA4: // NA4
                base = pmpaddr[i] << 2;
                top = (pmpaddr[i] << 2) + 3;
                base_in = (addr >= base && addr <= top);
                top_in = (addr + size - 1 >= base && addr + size - 1 <= top);
                break;
            case PMPNAPOT:
            { // NAPOT
                uint64_t mask = (-(((pmpaddr[i] + 1) & -(pmpaddr[i] + 1)) << 1)) << 2;
                // std::cout << "d napot mask  " << mask << " addr " << pmpaddr[i] << " config " << (int)pmpconfig[i] << "\n";
                base_in = (addr & mask) == ((pmpaddr[i] << 2) & mask);
                top_in = ((addr + size - 1) & mask) == ((pmpaddr[i] << 2) & mask);
                break;
            }
            }
            // check priority
            if ((base_in || top_in) && (priv != RV_PRIV_M_MODE || (cfg & PMPLOCK) != 0))
            {
                // std::cout << "PMP config " << (int)cfg << " addr " << addr << " operation " << (int)op << "\n";
                return ((op & (cfg | (cfg & 2) != 0)) == op && base_in && top_in);
            }
        }
        return priv == RV_PRIV_M_MODE;
    }
    bool icheck(uint64_t addr, uint8_t op, uint8_t size, uint8_t priv) const
    {

        for (auto i : pmpentry_id)
        {
            bool base_in = 0, top_in = 0;
            uint64_t base = 0, top = 0;
            uint8_t cfg = pmpconfig[i].read();
            // check pmp range
            switch (cfg & PMPA)
            {
            case PMPOFF:
                break;
            case PMPTOR: // TOR
                base = (i) ? pmpaddr[i - 1] << 2 : 0;
                top = pmpaddr[i] << 2;
                base_in = (addr >= base && addr < top);
                top_in = (addr + size - 1 >= base && addr + size - 1 < top);
                break;
            case PMPNA4: // NA4
                base = pmpaddr[i] << 2;
                top = (pmpaddr[i] << 2) + 3;
                base_in = (addr >= base && addr <= top);
                top_in = (addr + size - 1 >= base && addr + size - 1 <= top);
                break;
            case PMPNAPOT:
            { // NAPOT
                uint64_t mask = (-(((pmpaddr[i] + 1) & -(pmpaddr[i] + 1)) << 1)) << 2;
                // std::cout << "i napot mask  " << mask << " addr " << pmpaddr[i] << " config " << (int)pmpconfig[i] << "\n";
                base_in = (addr & mask) == ((pmpaddr[i] << 2) & mask);
                top_in = ((addr + size - 1) & mask) == ((pmpaddr[i] << 2) & mask);
                break;
            }
            }
            // check priority
            if ((base_in || top_in) && (priv != RV_PRIV_M_MODE || (cfg & PMPLOCK) != 0))
            {
                // std::cout << "PMP config " << (int)cfg << " pmpaddr " << pmpaddr[i] << " addr " << addr << " operation " << (int)op << "\n";
                return ((op & (cfg | (cfg & 2) != 0)) == op && base_in && top_in);
            }
        }
        // std::cout << "pmp icheck priv " << (int)priv << "\n";
        return priv == RV_PRIV_M_MODE;
    }

    void update_addr(uint32_t addr, uint32_t val)
    {
        if ((pmpconfig[addr].read() & PMPLOCK) == 0)
        {
            if (addr == PMPCOUNT - 1 ||
                ((pmpconfig[addr + 1].read() & (PMPLOCK | 0x18)) != (PMPLOCK | 0x8)))

            {
                pmpaddr[addr].write(val);
                // calc_range(addr, val, pmpconfig[addr].read());
                // std::cerr << "PMP id = " << addr << "  config  = " << (int)pmpconfig[addr].read() << "  previous addr" << pmpaddr[addr].read() << "  after " << val << "\n";
                // std::cerr << "PMP base = " << addr_base[addr] << "  PMP top  = " << addr_top[addr] << "\n";
            }
        }
    }
    void update_config(uint32_t addr, uint32_t val)
    {
        // std::cerr << "UPDATE PMPCONFIG  addr = " << addr << " VAL = " << val << "\n";
        for (int i = 0; i < 4; i++)
        {
            int a = (addr << 2) + i;
            uint8_t v = (int)(val & 0xff);
            if ((pmpconfig[a].read() & PMPLOCK) == 0)
            {
                if (a == PMPCOUNT - 1 ||
                    ((pmpconfig[a + 1].read() & (PMPLOCK | 0x18)) != (PMPLOCK | 0x8)))

                {

                    pmpconfig[a].write(v);
                    if (pmpconfig[a].read() == 0 && v != 0)
                    {
                        pmpentry_id.push_back(a);
                    }
                    if (pmpconfig[a].read() != 0 && v == 0)
                    {
                        auto new_end = std::remove(pmpentry_id.begin(), pmpentry_id.end(), a);
                        pmpentry_id.erase(new_end, pmpentry_id.end());
                    }
                                }
            }
            val >>= 8;
        }
    }
};