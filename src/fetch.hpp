#pragma once
#include <systemc>
#include <cstdint>
#include "memory.hpp"
using namespace sc_core;

SC_MODULE(FetchInstruction)
{
    sc_in<bool> clk;
    sc_in<bool> rst;

    sc_in<bool> stall;
    sc_in<bool> flush;
    sc_in<bool> immu_flush;

    sc_in<MMU_RSP> immu_to_ifetch;
    sc_in<uint32_t> immu_pc;
    sc_out<uint32_t> if_id_inst;
    sc_out<uint32_t> if_id_pc;
    sc_out<uint8_t> if_id_fault;
    sc_out<bool> if_id_flush;

    sc_out<uint32_t> if_id_tval;
    sc_out<bool> if_id_accept;
    uint64_t buffer;
    uint64_t buffer_page_fault;
    uint64_t buffer_access_fault;
    uint32_t buffer_idx;
    uint32_t buffer_addr;
    SC_HAS_PROCESS(FetchInstruction);
    FetchInstruction(sc_module_name name) : sc_module(name)
    {
        SC_METHOD(get_inst);
        sensitive << clk.pos();
        SC_THREAD(buffer_state);
        // sensitive << stall << clk.neg();
        sensitive << clk.neg();
    }
    void buffer_state() // info prefetch to fetch next pc
    {
        wait();
        while (1)
        {
            wait();
            wait(1, SC_NS); // wait EXEC::comb_out
            bool accept = 0;
            auto input = immu_to_ifetch.read();
            if (input.mmu_rsp_ack)
            {
                if (immu_pc.read() != buffer_addr + (buffer_idx >> 3))
                {
                    accept = 1;
                }
                else if (!stall.read() && !if_id_flush.read())
                {
                    if ((buffer & 3) == 3)
                    {
                        accept = 1;
                    }
                    else
                    {
                        accept = buffer_idx <= 48;
                    }
                }
                else
                {
                    accept = (buffer_idx <= 32);
                }
            }
            // std::cout << "accept :" << accept << "\n";
            if_id_accept.write(accept);
        }
    }
    void get_inst()
    {
        auto input = immu_to_ifetch.read();
        if (rst.read() || flush.read())
        {
            buffer_idx = 0;
            buffer_addr = 0;
            buffer = 0;
            buffer_page_fault = buffer_access_fault = 0;
        }
        else if (input.mmu_rsp_ack && !immu_flush.read()) // receive instr data
        {
            uint32_t input_addr = immu_pc.read();
            uint64_t input_inst = input.mmu_rsp_rdata;
            uint64_t page_fault = (input.mmu_rsp_fault == INSTR_PAGE_FAULT) ? 0xffffffff : 0;
            uint64_t access_fault = (input.mmu_rsp_fault == INSTR_ACCESS_FAULT) ? 0xffffffff : 0;

            if (input_addr != buffer_addr + (buffer_idx >> 3)) // not continuous fetch
            {
                buffer_addr = input_addr;
                if (input_addr & 3)
                {
                    buffer_idx = 16;
                    buffer = input_inst >> 16;
                    buffer_page_fault = page_fault >> 16;
                    buffer_access_fault = access_fault >> 16;
                }
                else
                {
                    buffer_idx = 32;
                    buffer = input_inst;
                    buffer_page_fault = page_fault;
                    buffer_access_fault = access_fault;
                }
            }
            else if (!stall.read() && !if_id_flush.read()) // pop instr & accept instr data
            {
                if ((buffer & 3) == 3)
                {
                    buffer_addr += 4;
                    buffer >>= 32;
                    buffer |= (input_inst << (buffer_idx - 32));
                    buffer_page_fault >>= 32;
                    buffer_page_fault |= (page_fault << (buffer_idx - 32));
                    buffer_access_fault >>= 32;
                    buffer_access_fault |= (access_fault << (buffer_idx - 32));
                }
                else
                {
                    buffer_addr += 2;
                    buffer >>= 16;
                    buffer_access_fault >>= 16;
                    buffer_page_fault >>= 16;
                    if (buffer_idx <= 48)
                    {
                        buffer |= (input_inst << (buffer_idx - 16));
                        buffer_page_fault |= (page_fault << (buffer_idx - 16));
                        buffer_access_fault |= (access_fault << (buffer_idx - 16));
                        buffer_idx += 16;
                    }
                    else
                    {
                        buffer_idx -= 16;
                    }
                }
            }
            else if (buffer_idx <= 32) // buffer accept instr data
            {
                buffer |= (input_inst << buffer_idx);
                buffer_page_fault |= (page_fault << buffer_idx);
                buffer_access_fault |= (access_fault << buffer_idx);
                buffer_idx += 32;
            }
        }
        else if (!stall.read() && !if_id_flush.read()) // pop instr
        {
            if ((buffer & 3) == 3)
            {
                buffer_addr += 4;
                buffer >>= 32;
                buffer_page_fault >>= 32;
                buffer_access_fault >>= 32;
                buffer_idx -= 32;
            }
            else
            {
                buffer_addr += 2;
                buffer >>= 16;
                buffer_page_fault >>= 16;
                buffer_access_fault >>= 16;
                buffer_idx -= 16;
            }
        }

        // output if_id
        if (rst.read() || flush.read())
        {
            if_id_flush.write(true);
        }
        else if ((buffer & 3) == 3)
        {
            if_id_flush.write(buffer_idx < 32);
            if (buffer_access_fault & 1)
            {
                if_id_fault.write(INSTR_ACCESS_FAULT);
                if_id_tval.write(buffer_addr);
            }
            else if (buffer_page_fault & 1)
            {
                if_id_fault.write(INSTR_PAGE_FAULT);
                if_id_tval.write(buffer_addr);
            }
            else if (buffer_access_fault & (1 << 16))
            {
                if_id_fault.write(INSTR_ACCESS_FAULT);
                if_id_tval.write(buffer_addr + 2);
            }
            else if (buffer_page_fault & (1 << 16))
            {
                if_id_fault.write(INSTR_PAGE_FAULT);
                if_id_tval.write(buffer_addr + 2);
            }
            else
            {
                if_id_fault.write(0);
                if_id_tval.write(0);
            }
        }
        else
        {
            if_id_flush.write(buffer_idx < 16);
            if (buffer_access_fault & 1)
            {
                if_id_fault.write(INSTR_ACCESS_FAULT);
                if_id_tval.write(buffer_addr);
            }
            else if (buffer_page_fault & 1)
            {
                if_id_fault.write(INSTR_PAGE_FAULT);
                if_id_tval.write(buffer_addr);
            }
            else
            {
                if_id_fault.write(0);
                if_id_tval.write(0);
            }
        }
        if_id_inst.write(buffer);
        if_id_pc.write(buffer_addr);
    }
    void monitor()
    {
        auto input = immu_to_ifetch.read();

        if (immu_to_ifetch.read().mmu_rsp_ack)
            std::cout << "IMMU rsp: PC " << std::hex << immu_pc.read() << ", rdata " << immu_to_ifetch.read().mmu_rsp_rdata << ", fault : " << (int)immu_to_ifetch.read().mmu_rsp_fault << "\n";
        std::cout << "IF buffer PC: " << std::hex << buffer_addr << ", buffer size " << buffer_idx << ", buffer data : " << buffer << " page fault : " << buffer_page_fault << " access fault " << buffer_access_fault << "\n";
    }
};