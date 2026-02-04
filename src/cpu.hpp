#pragma once
#include <cstdint>
#include "exec.hpp"
#include "prefetch.hpp"
#include "mmu.hpp"
#include "fetch.hpp"
#include "decode.hpp"
#include "regfile.hpp"
#include "memory.hpp"
#include <systemc>
using namespace sc_core;

SC_MODULE(CPU)
{

    bool print_en = 0;
    // cpu io
    sc_in<bool> clk;
    sc_in<bool> rst;

    sc_out<IMEM_IN> immu_to_mem;
    sc_in<MEM_OUT> mem_to_immu;
    sc_out<DMEM_IN> dmmu_to_mem;
    sc_in<MEM_OUT> mem_to_dmmu;

    // interconnect
    // clint

    // exec
    EXEC *exec;
    // npc
    PreFetch *prefetch;
    // mmu
    MMU *mmu;
    // fetchinstruction
    FetchInstruction *fetch_inst;
    // decode
    Decode *decode;
    // regfile
    RegFile *regs;

    sc_signal<bool> wb_stall;

    sc_signal<uint32_t> ex_pc;
    sc_signal<ID_EX> id_ex_port;

    sc_signal<MMU_RSP> dmmu_to_ex;

    sc_signal<bool> stall;
    sc_signal<bool> flush;

    // forwarding
    sc_signal<uint32_t> npc;
    sc_signal<bool> ex_write_rd;
    sc_signal<uint32_t> ex_rd_val;
    sc_signal<uint8_t> ex_rd;
    // wb
    sc_signal<uint8_t> wb_rd;
    sc_signal<bool> wb_write_rd;
    sc_signal<uint32_t> wb_rd_val;
    sc_signal<uint32_t> wb_pc;
    sc_signal<InstrType> wb_type;

    sc_signal<DMEM_IN> ex_to_dmmu;

    sc_signal<bool> immu_ready;
    sc_signal<uint32_t> immu_pc;
    sc_signal<uint32_t> if_id_pc;
    sc_signal<uint32_t> id_ex_pc;
    sc_signal<uint32_t> if_id_tval;
    sc_signal<bool> if_id_flush;
    sc_signal<bool> if_id_accept;
    sc_signal<bool> id_ex_flush;
    sc_signal<bool> immu_flush;
    sc_signal<uint32_t> fetch_pc;

    sc_signal<MMU_RSP> immu_to_ifetch;
    sc_signal<uint32_t> if_id_inst;
    sc_signal<uint8_t> if_id_fault;

    SC_HAS_PROCESS(CPU);
    CPU(sc_module_name name, uint32_t entry, CLINT_DEV *clint) : sc_module(name)
    {
        regs = new RegFile("regs");
        exec = new EXEC("csr", clint, entry);
        prefetch = new PreFetch("prefetch");
        mmu = new MMU("mmu", exec);
        fetch_inst = new FetchInstruction("fetch_inst");
        decode = new Decode("decode", regs);
        // csr  TODO: csr to mmu (pmpcheck)
        exec->clk(clk);
        exec->rst(rst);

        exec->ex_pc(ex_pc);
        exec->id_ex_port(id_ex_port);

        // dmem
        exec->dmmu_to_ex(dmmu_to_ex);
        exec->ex_to_dmmu(ex_to_dmmu);

        // ctrl
        exec->stall(stall);
        exec->flush(flush);
        exec->npc(npc);

        // forwarding signal
        exec->ex_write_rd(ex_write_rd);
        exec->ex_rd_val(ex_rd_val);
        exec->ex_rd(ex_rd);

        // write back
        exec->wb_stall(wb_stall);
        exec->wb_write_rd(wb_write_rd);
        exec->wb_rd_val(wb_rd_val);
        exec->wb_rd(wb_rd);
        exec->wb_pc(wb_pc);
        exec->wb_type(wb_type);

        // prefetch
        prefetch->clk(clk);
        prefetch->immu_ready(immu_ready);
        // prefetch->ex_pc(ex_pc);
        prefetch->npc(npc);
        prefetch->immu_pc(immu_pc);
        prefetch->if_id_pc(if_id_pc);
        prefetch->id_ex_pc(id_ex_pc);
        prefetch->if_id_flush(if_id_flush);
        prefetch->if_id_accept(if_id_accept);
        prefetch->id_ex_flush(id_ex_flush);
        prefetch->immu_flush(immu_flush);
        prefetch->fetch_pc(fetch_pc);
        // mmu
        mmu->clk(clk);
        mmu->rst(rst);
        mmu->immu_ready(immu_ready);
        mmu->if_id_accept(if_id_accept);
        mmu->fetch_pc(fetch_pc);
        mmu->flush(flush);
        mmu->immu_to_ifetch(immu_to_ifetch);
        mmu->immu_pc(immu_pc);
        mmu->immu_flush(immu_flush);
        mmu->immu_to_mem(immu_to_mem);
        mmu->mem_to_immu(mem_to_immu);
        mmu->ex_to_dmmu(ex_to_dmmu);
        mmu->dmmu_to_ex(dmmu_to_ex);
        mmu->dmmu_to_mem(dmmu_to_mem);
        mmu->mem_to_dmmu(mem_to_dmmu);

        // fetch
        fetch_inst->clk(clk);
        fetch_inst->rst(rst);
        fetch_inst->stall(stall);
        fetch_inst->flush(flush);
        fetch_inst->immu_flush(immu_flush);
        fetch_inst->immu_to_ifetch(immu_to_ifetch);
        fetch_inst->immu_pc(immu_pc);
        fetch_inst->if_id_inst(if_id_inst);
        fetch_inst->if_id_pc(if_id_pc);
        fetch_inst->if_id_fault(if_id_fault);
        fetch_inst->if_id_flush(if_id_flush);
        fetch_inst->if_id_accept(if_id_accept);
        fetch_inst->if_id_tval(if_id_tval);
        // decode
        decode->clk(clk);
        decode->rst(rst);
        decode->stall(stall);
        decode->flush(flush);
        decode->npc(npc);
        decode->if_id_inst(if_id_inst);
        decode->if_id_pc(if_id_pc);
        decode->if_id_fault(if_id_fault);
        decode->if_id_flush(if_id_flush);
        decode->if_id_tval(if_id_tval);
        decode->id_ex_port(id_ex_port);
        decode->id_ex_pc(id_ex_pc);
        decode->id_ex_flush(id_ex_flush);
        decode->ex_write_rd(ex_write_rd);
        decode->ex_rd_val(ex_rd_val);
        decode->ex_rd(ex_rd);
        decode->wb_write_rd(wb_write_rd);
        decode->wb_rd_val(wb_rd_val);
        decode->wb_rd(wb_rd);
        // regs
        regs->clk(clk);
        regs->wb_write_rd(wb_write_rd);
        regs->wb_rd_val(wb_rd_val);
        regs->wb_rd(wb_rd);

        SC_THREAD(monitor);
        sensitive << clk.pos();
    }

    void monitor()
    {
        wait();
        while (1)
        {
            wait();
            if (print_en)
            {
                if (rst.read())
                    std::cout << "RESET CPU !!!\n";
                else if (flush.read())
                    std::cout << "FLUSH PIPELINE !!!\n";
                else if (stall.read())
                    std::cout << "CPU STALLING !!!\n";
                std::cout << "Target PC = " << std::hex << ex_pc.read() << "  time " << sc_time_stamp();
                std::cout << ", fetching PC = " << immu_pc.read();
                std::cout << ", next fetching PC = " << fetch_pc.read() << "\n";

                std::cout << "Current priv = " << exec->priv << " mstatus = " << exec->csr_regs[CSR_MSTATUS].read() << "\n";
                mmu->inst_monitor();
                fetch_inst->monitor();
                decode->monitor();
                std::cout << "\n";
                exec->monitor();
                mmu->data_monitor();
                std::cout << "\nWB : PC=" << std::hex << wb_pc.read()
                          << " " << to_string(wb_type.read())
                          << " RDVAL=" << std::hex << wb_rd_val.read()
                          << " rd=" << unsigned(wb_rd.read())
                          << ((wb_write_rd.read())
                                  ? " write back"
                                  : "")
                          << "\n\n";
            }
        }
    }
};
