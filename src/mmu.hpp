#pragma once
#include "memory.hpp"
#include "exec.hpp"
#include "pmp.hpp"
#include <systemc>
#include <vector>
using namespace sc_core;

#define DIRTY_BIT 0x80
#define ACCESS_BIT 0x40

// TODO : MXR (allow executable  page to be load, for debugging)
#define MXR ((exec->csr_regs[CSR_MSTATUS].read() & MSTATUS_MXR) >> MSTATUS_MXR_SHIFT)
#define MPRV ((exec->csr_regs[CSR_MSTATUS].read() & MSTATUS_MPRV) >> MSTATUS_MPRV_SHIFT)
#define MPP ((exec->csr_regs[CSR_MSTATUS].read() & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT)
#define SUM ((exec->csr_regs[CSR_MSTATUS].read() & MSTATUS_SUM) >> MSTATUS_SUM_SHIFT)
#define SATP_ON(satp) ((satp >> 31) & 0x1)
#define IS_LEAF_PTE(pte) ((pte & 0xE) != 0)
#define USERBIT(pte) ((pte >> 4) & 1)

bool is_pma_valid(uint32_t phys_addr)
{
    // Define your platform's valid ranges (for riscv-arch-test)
    // Example: RAM is at 0x80000000 - 0x80010000
    if (phys_addr >= 0x80000000)
    {
        return true; // Valid RAM access
    }
    // Example: Timer is at 0x20000000
    if (phys_addr >= CLINT_BASE && phys_addr < CLINT_TOP)
    {
        return true; // Valid IO access
    }
    if (phys_addr >= UART_BASE && phys_addr < UART_TOP)
    {
        return true; // Valid IO access
    }
    if (phys_addr >= PLIC_BASE && phys_addr < PLIC_TOP)
    {
        return true; // Valid IO access
    }
    return false;
    // return true;
}
SC_MODULE(MMU)
{
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_out<bool> immu_ready;

    sc_in<bool> if_id_accept;
    // npc to mmu
    sc_in<uint32_t> fetch_pc;
    sc_in<bool> flush;
    // mmu to ifetch
    sc_out<MMU_RSP> immu_to_ifetch;
    sc_out<uint32_t> immu_pc;
    sc_out<bool> immu_flush;
    // mmu to mem
    sc_out<IMEM_IN> immu_to_mem;
    // mem to mmu
    sc_in<MEM_OUT> mem_to_immu;

    // ex to mmu
    sc_in<DMEM_IN> ex_to_dmmu;
    // mmu to csr
    sc_out<MMU_RSP> dmmu_to_ex;
    // mmu to mem
    sc_out<DMEM_IN> dmmu_to_mem;
    // mem to mmu
    sc_in<MEM_OUT> mem_to_dmmu;

    EXEC *exec;

    sc_signal<uint8_t> d_state;
    sc_signal<bool> d_page_fault;
    sc_signal<bool> d_pmp_fault;
    sc_signal<bool> d_misaligned_fault;
    bool _d_page_fault;
    bool _d_pmp_fault;
    uint8_t d_priv = 3;
    uint32_t d_satp = 0;
    uint32_t d_root_ppn = 0;
    uint32_t d_root_addr = 0;
    uint32_t d_vpn1 = 0;
    uint32_t d_vpn0 = 0;
    uint32_t d_offset = 0;
    uint32_t d_op = 0;
    uint32_t d_pte = 0;
    uint32_t d_ppn = 0;

    sc_signal<uint8_t> i_state;
    sc_signal<bool> i_page_fault;
    sc_signal<bool> i_pmp_fault;
    bool _i_page_fault;
    bool _i_pmp_fault;
    uint32_t i_satp = 0;
    uint32_t i_root_ppn = 0;
    uint32_t i_root_addr = 0;
    uint32_t i_vpn1 = 0;
    uint32_t i_vpn0 = 0;
    uint32_t i_offset = 0;
    uint32_t i_pte = 0;
    uint32_t i_ppn = 0;

    DMEM_IN d_mmu_mem;
    IMEM_IN i_mmu_mem;

    SC_HAS_PROCESS(MMU);
    MMU(sc_module_name name, EXEC * e) : sc_module(name), exec(e)
    {
        i_state.write(0);
        d_state.write(0);
        // processing flow :
        // dmem_rsp -> dmem_req -> dpmp_rsp
        // 1. read pte or data from mem
        // 2. solve pte from mem or handle request from lsu
        // 3. check pte valid and send request address to pmp check
        // 4. send valid request address (when pmp valid) to mem and wait memory response
        // SC_METHOD(dmem_rsp);
        // sensitive << mem_to_dmmu << d_pmp_fault << d_page_fault << d_state;
        SC_THREAD(dmem_rsp);
        sensitive << clk.pos();
        SC_THREAD(dmem_req);
        sensitive << clk.pos();
        // SC_METHOD(imem_rsp);
        // sensitive << mem_to_immu << i_pmp_fault << i_page_fault << i_state;
        SC_THREAD(imem_rsp);
        sensitive << clk.pos();
        SC_THREAD(imem_req);
        sensitive << clk.pos();
    }

    void dmem_rsp()
    {
        wait();
        while (1)
        {
            wait();
            wait(1, SC_NS); // wait for mem response
            auto rsp = mem_to_dmmu.read();
            MMU_RSP dmmu_rsp;
            // d_page_fault & d_pmp_fault are saved in reg previously

            dmmu_rsp.mmu_rsp_ack = (d_state.read() == 3 && rsp.mem_rsp_ack) || d_page_fault.read() || d_pmp_fault.read() || d_misaligned_fault.read() || rsp.mem_rsp_fault != 0;
            if (d_misaligned_fault.read())
            {
                if (d_op == 1)
                    dmmu_rsp.mmu_rsp_fault = LOAD_ADDR_MISALIGNED; // load misaligned fault
                else
                    dmmu_rsp.mmu_rsp_fault = STORE_ADDR_MISALIGNED; // store misaligned fault
            }
            else if (d_page_fault.read())
            {
                if (d_op == 1)
                    dmmu_rsp.mmu_rsp_fault = LOAD_PAGE_FAULT; // load page fault
                else
                    dmmu_rsp.mmu_rsp_fault = STORE_PAGE_FAULT; // store page fault
            }
            else
            {
                if (rsp.mem_rsp_fault == 0 && !d_pmp_fault.read())
                    dmmu_rsp.mmu_rsp_fault = 0;
                else
                {
                    if (d_op == 1)
                        dmmu_rsp.mmu_rsp_fault = LOAD_ACCESS_FAULT; // load access fault
                    else
                        dmmu_rsp.mmu_rsp_fault = STORE_ACCESS_FAULT; // store access fault
                }
                dmmu_rsp.mmu_rsp_rdata = rsp.mem_rsp_rdata;
            }
            if (dmmu_rsp.mmu_rsp_fault)
                dmmu_rsp.mmu_rsp_tval = ex_to_dmmu.read().dmem_addr;
            dmmu_to_ex.write(dmmu_rsp);
        }
    }
    void dmem_req()
    {
        wait();
        while (1)
        {
            wait();
            auto req = ex_to_dmmu.read();
            auto rsp = mem_to_dmmu.read();
            if (rst.read() || flush.read())
            {
                _d_page_fault = 0;
                d_state.write(0);
                d_satp = 0;
                d_root_ppn = 0;
                d_root_addr = 0;
                d_vpn1 = 0;
                d_vpn0 = 0;
                d_offset = 0;
                d_op = 0;
                d_mmu_mem.dmem_req_ack = 0;
                d_priv = 3;
            }
            else if (d_state.read() == 0) // IDLE state
            {
                _d_page_fault = 0;
                if (req.dmem_req_ack && !flush.read())
                {
                    d_satp = exec->csr_regs[CSR_SATP].read();
                    d_root_ppn = d_satp & 0x003FFFFF;
                    d_root_addr = d_root_ppn << 12;

                    d_vpn1 = (req.dmem_addr >> 22) & 0x3FF;
                    d_vpn0 = (req.dmem_addr >> 12) & 0x3FF;
                    d_offset = req.dmem_addr & 0xFFF;
                    d_priv = (MPRV) ? MPP : exec->priv;
                    if (req.dmem_is_load || (req.dmem_is_amo && req.dmem_amoop == 0))
                        d_op = 1;
                    else
                        d_op = 2;

                    if (SATP_ON(d_satp) &&
                        (exec->priv == RV_PRIV_S_MODE || exec->priv == RV_PRIV_U_MODE ||
                         (d_priv != RV_PRIV_M_MODE)))
                    {
                        // load pte

                        auto tlbentry = exec->tlb.find((d_satp >> 20) & 0x1ff, req.dmem_addr >> 12);
                        d_pte = tlbentry.pte;
                        // d_pte = 0;
                        if (d_pte & 1)
                        {
                            d_ppn = (d_pte >> 10) & 0xFFFFF;
                            _d_page_fault = ((((1 << d_op) & d_pte) == 0 && (d_op != 1 || MXR == 0 || (d_pte & 4) == 0)) ||
                                             ((req.dmem_is_store || req.dmem_is_amo) &&
                                              ((d_pte & DIRTY_BIT) == 0 || (d_pte & ACCESS_BIT) == 0)) ||
                                             (req.dmem_is_load && (d_pte & ACCESS_BIT) == 0) ||
                                             (d_priv == RV_PRIV_S_MODE && USERBIT(d_pte) && SUM == 0) ||
                                             (d_priv == RV_PRIV_U_MODE && USERBIT(d_pte) == 0) ||
                                             ((d_pte & 6) == 4));
                            if (_d_page_fault)
                            {
                                d_state.write(0);
                                d_mmu_mem.dmem_req_ack = 0;
                            }
                            else if (tlbentry.is_megapage)
                            {
                                d_mmu_mem.dmem_req_ack = 1;
                                d_mmu_mem.dmem_is_load = req.dmem_is_load;
                                d_mmu_mem.dmem_is_store = req.dmem_is_store;
                                d_mmu_mem.dmem_is_amo = req.dmem_is_amo;
                                d_mmu_mem.dmem_amoop = req.dmem_amoop;
                                d_mmu_mem.dmem_addr = (d_ppn << 12) + (req.dmem_addr & 0x3FFFFF);
                                d_mmu_mem.dmem_wdata = req.dmem_wdata;
                                d_mmu_mem.dmem_size = req.dmem_size;
                                d_state.write(3);
                            }
                            else
                            {
                                d_mmu_mem.dmem_req_ack = 1;
                                d_mmu_mem.dmem_is_load = req.dmem_is_load;
                                d_mmu_mem.dmem_is_store = req.dmem_is_store;
                                d_mmu_mem.dmem_is_amo = req.dmem_is_amo;
                                d_mmu_mem.dmem_amoop = req.dmem_amoop;
                                d_mmu_mem.dmem_addr = (d_ppn << 12) + (req.dmem_addr & 0xFFF);
                                d_mmu_mem.dmem_wdata = req.dmem_wdata;
                                d_mmu_mem.dmem_size = req.dmem_size;
                                d_state.write(3);
                            }
                        }
                        else
                        {

                            d_state.write(1);
                            d_mmu_mem.dmem_req_ack = 1;
                            d_mmu_mem.dmem_is_load = 1;
                            d_mmu_mem.dmem_is_store = 0;
                            d_mmu_mem.dmem_is_amo = 0;
                            d_mmu_mem.dmem_amoop = 0;
                            d_mmu_mem.dmem_addr = d_root_addr + d_vpn1 * 4;
                            d_mmu_mem.dmem_wdata = 0;
                            d_mmu_mem.dmem_size = 4;
                        }
                        // std::cout << "virtual memory for Data load/store satp is " << exec->csr_regs[CSR_SATP].read() << " " << d_root_addr << " " << d_vpn1 << " " << d_mmu_mem.dmem_addr << "\n";
                    }
                    else
                    {
                        // execute mem operation
                        d_state.write(3);
                        d_mmu_mem.dmem_req_ack = 1;
                        d_mmu_mem.dmem_is_load = req.dmem_is_load;
                        d_mmu_mem.dmem_is_store = req.dmem_is_store;
                        d_mmu_mem.dmem_is_amo = req.dmem_is_amo;
                        d_mmu_mem.dmem_amoop = req.dmem_amoop;
                        d_mmu_mem.dmem_addr = req.dmem_addr;
                        d_mmu_mem.dmem_wdata = req.dmem_wdata;
                        d_mmu_mem.dmem_size = req.dmem_size;
                    }
                }
                else
                {
                    d_state.write(0);
                    d_satp = 0;
                    d_root_ppn = 0;
                    d_root_addr = 0;
                    d_vpn1 = 0;
                    d_vpn0 = 0;
                    d_offset = 0;
                    d_op = 0;
                    d_mmu_mem.dmem_req_ack = 0;
                }
            }
            else if (rsp.mem_rsp_ack)
            {
                if (d_state == 1)
                {
                    d_pte = rsp.mem_rsp_rdata;
                    d_ppn = (d_pte >> 10) & 0xFFFFF;
                    uint8_t DAUGXWRV = rsp.mem_rsp_rdata & 0xFF;
                    _d_page_fault = (d_pte & 1) == 0 ||
                                    (DAUGXWRV != 1 && (((d_ppn & 0x3FF) != 0) || // megapage
                                                       (((1 << d_op) & d_pte) == 0 && (d_op != 1 || MXR == 0 || (d_pte & 4) == 0)) ||
                                                       ((req.dmem_is_store || req.dmem_is_amo) &&
                                                        ((d_pte & DIRTY_BIT) == 0 || (d_pte & ACCESS_BIT) == 0)) ||
                                                       (req.dmem_is_load && (d_pte & ACCESS_BIT) == 0) ||
                                                       (d_priv == RV_PRIV_S_MODE && USERBIT(d_pte) && SUM == 0) ||
                                                       (d_priv == RV_PRIV_U_MODE && USERBIT(d_pte) == 0) ||
                                                       ((d_pte & 6) == 4)));

                    if (_d_page_fault)
                    {
                        d_state.write(0);
                        d_mmu_mem.dmem_req_ack = 0;
                    }
                    else
                    {
                        d_mmu_mem.dmem_req_ack = 1;
                        if ((DAUGXWRV & 0xE) != 0) // megapage
                        {
                            d_mmu_mem.dmem_is_load = req.dmem_is_load;
                            d_mmu_mem.dmem_is_store = req.dmem_is_store;
                            d_mmu_mem.dmem_is_amo = req.dmem_is_amo;
                            d_mmu_mem.dmem_amoop = req.dmem_amoop;
                            d_mmu_mem.dmem_addr = (d_ppn << 12) + (req.dmem_addr & 0x3FFFFF);
                            d_mmu_mem.dmem_wdata = req.dmem_wdata;
                            d_mmu_mem.dmem_size = req.dmem_size;
                            d_state.write(3);
                            exec->tlb.store((d_satp >> 20) & 0x1ff, (req.dmem_addr >> 12), d_pte, 1);
                            // std::cout << "MAGAPAGE\n";
                        }
                        else
                        {
                            d_mmu_mem.dmem_is_load = 1;
                            d_mmu_mem.dmem_is_store = 0;
                            d_mmu_mem.dmem_is_amo = 0;
                            d_mmu_mem.dmem_amoop = 0;
                            d_mmu_mem.dmem_addr = (d_ppn << 12) + d_vpn0 * 4;
                            d_mmu_mem.dmem_wdata = 0;
                            d_mmu_mem.dmem_size = 4;
                            d_state.write(2);
                        }
                    }
                }
                else if (d_state == 2)
                {
                    d_pte = rsp.mem_rsp_rdata;
                    d_ppn = (d_pte >> 10) & 0xFFFFF;
                    _d_page_fault = (d_pte & 1) == 0 || ((((1 << d_op) & d_pte) == 0 && (d_op != 1 || MXR == 0 || (d_pte & 4) == 0)) ||
                                                         ((req.dmem_is_store || req.dmem_is_amo) &&
                                                          ((d_pte & DIRTY_BIT) == 0 || (d_pte & ACCESS_BIT) == 0)) ||
                                                         (req.dmem_is_load && (d_pte & ACCESS_BIT) == 0) ||
                                                         (d_priv == RV_PRIV_S_MODE && USERBIT(d_pte) && SUM == 0) ||
                                                         (d_priv == RV_PRIV_U_MODE && USERBIT(d_pte) == 0) ||
                                                         ((d_pte & 6) == 4));
                    if (_d_page_fault)
                    {
                        d_state.write(0);
                        d_mmu_mem.dmem_req_ack = 0;
                    }
                    else
                    {
                        d_mmu_mem.dmem_req_ack = 1;
                        d_mmu_mem.dmem_is_load = req.dmem_is_load;
                        d_mmu_mem.dmem_is_store = req.dmem_is_store;
                        d_mmu_mem.dmem_is_amo = req.dmem_is_amo;
                        d_mmu_mem.dmem_amoop = req.dmem_amoop;
                        d_mmu_mem.dmem_addr = (d_ppn << 12) + d_offset;
                        d_mmu_mem.dmem_wdata = req.dmem_wdata;
                        d_mmu_mem.dmem_size = req.dmem_size;
                        d_state.write(3);
                        exec->tlb.store((d_satp >> 20) & 0x1ff, (req.dmem_addr >> 12), d_pte, 0);
                    }
                }
                else
                {
                    _d_page_fault = 0;
                    d_state.write(0);
                    d_mmu_mem.dmem_req_ack = 0;
                }
            }
            else
            {
                d_mmu_mem.dmem_req_ack = 0;
                _d_page_fault = false;
            }

            bool _d_misaligned_fault = 0;
            if (d_mmu_mem.dmem_req_ack)
            {
                _d_misaligned_fault = (d_mmu_mem.dmem_addr & (-d_mmu_mem.dmem_size)) != d_mmu_mem.dmem_addr;
                if (d_mmu_mem.dmem_is_store || d_mmu_mem.dmem_is_amo)
                    _d_pmp_fault = !exec->pmp_dcheck(d_mmu_mem.dmem_addr,
                                                     (uint8_t)2,
                                                     d_mmu_mem.dmem_size) ||
                                   (!is_pma_valid(d_mmu_mem.dmem_addr));
                else
                    _d_pmp_fault = !exec->pmp_dcheck(d_mmu_mem.dmem_addr,
                                                     1,
                                                     4) ||
                                   (!is_pma_valid(d_mmu_mem.dmem_addr));
            }
            else
            {
                _d_pmp_fault = false;
            }
            // std::cout << "d_state " << (int)d_state << "\n";
            if (_d_pmp_fault || _d_page_fault || _d_misaligned_fault)
            {
                dmmu_to_mem.write(DMEM_IN());
            }
            else
            {
                dmmu_to_mem.write(d_mmu_mem);
            }
            d_page_fault.write(_d_page_fault);
            // std::cout << "DDDDDDDEUBBBBBGGGGd page fault   " << _d_page_fault << " dstate   " << (int)d_state << "\n";
            d_pmp_fault.write(_d_pmp_fault);
            d_misaligned_fault.write(_d_misaligned_fault);
        }
    }

    // imem_rsp -> immu_ready -> pmp_check -> update
    void imem_rsp()
    {
        wait();
        while (1)
        {
            wait();
            wait(1, SC_NS); // wait for mem response
            auto rsp = mem_to_immu.read();
            MMU_RSP immu_rsp;
            // d_page_fault & d_pmp_fault are saved in reg previously
            immu_rsp.mmu_rsp_ack = ((i_state.read() == 3 || immu_flush.read()) && rsp.mem_rsp_ack) || i_page_fault.read() || i_pmp_fault.read() || rsp.mem_rsp_fault != 0;
            if (i_page_fault.read())
            {
                immu_rsp.mmu_rsp_fault = INSTR_PAGE_FAULT; // inst page fault
            }
            else
            {
                if (rsp.mem_rsp_fault == 0 && !i_pmp_fault.read())
                    immu_rsp.mmu_rsp_fault = 0;
                else
                {
                    immu_rsp.mmu_rsp_fault = 1; // inst access fault
                }
                immu_rsp.mmu_rsp_rdata = rsp.mem_rsp_rdata;
            }
            if (immu_rsp.mmu_rsp_fault)
                immu_rsp.mmu_rsp_tval = immu_pc.read();
            immu_to_ifetch.write(immu_rsp);
            immu_ready.write(immu_rsp.mmu_rsp_ack || i_state.read() == 0);
        }
    }
    void imem_req()
    {
        wait();
        while (1)
        {
            wait();
            auto rsp = mem_to_immu.read();
            bool is_fetch = 0;
            if (immu_ready.read())
            {
                immu_flush.write(0);
            }
            else if (flush.read())
            {
                immu_flush.write(1);
            }
            if (rst.read() || flush.read())
            {
                _i_page_fault = 0;
                i_state.write(0);
                i_satp = 0;
                i_root_ppn = 0;
                i_root_addr = 0;
                i_vpn1 = 0;
                i_vpn0 = 0;
                i_offset = 0;
                i_mmu_mem.imem_req_ack = 0;
            }
            else if (immu_ready.read()) // IDLE state
            {

                if (!flush.read())
                {
                    auto pc = (if_id_accept.read()) ? fetch_pc.read() : immu_pc.read();
                    immu_pc.write(pc);
                    _i_page_fault = 0;
                    i_satp = exec->csr_regs[CSR_SATP].read();
                    i_root_ppn = i_satp & 0x003FFFFF;
                    i_root_addr = i_root_ppn << 12;

                    i_vpn1 = (pc >> 22) & 0x3FF;
                    i_vpn0 = (pc >> 12) & 0x3FF;
                    i_offset = pc & 0xFFF;

                    if (SATP_ON(i_satp) &&
                        (exec->priv == RV_PRIV_S_MODE || exec->priv == RV_PRIV_U_MODE))
                    {
                        auto tlbentry = exec->tlb.find((i_satp >> 20) & 0x1ff, pc >> 12);
                        i_pte = tlbentry.pte;
                        // i_pte = 0;
                        if (i_pte & 1)
                        {
                            i_ppn = (i_pte >> 10) & 0xFFFFF;
                            _i_page_fault = (8 & i_pte) == 0 || (i_pte & ACCESS_BIT) == 0 ||
                                            (exec->priv == RV_PRIV_S_MODE && USERBIT(i_pte)) ||
                                            (exec->priv == RV_PRIV_U_MODE && USERBIT(i_pte) == 0) ||
                                            ((i_pte & 6) == 4);
                            if (_i_page_fault)
                                i_mmu_mem.imem_req_ack = 0;
                            else if (tlbentry.is_megapage)
                            {
                                i_mmu_mem.imem_req_ack = 1;
                                is_fetch = 1;
                                i_mmu_mem.imem_addr = (i_ppn << 12) + (pc & 0x3FFFFC); // 0x3FFFFF
                                i_state.write(3);
                            }
                            else
                            {
                                is_fetch = 1;
                                i_mmu_mem.imem_req_ack = 1;
                                i_mmu_mem.imem_addr = (i_ppn << 12) + (i_offset & ~3);
                                i_state.write(3);
                            }
                        }

                        else
                        {
                            // load pte
                            i_state.write(1);
                            i_mmu_mem.imem_req_ack = 1;
                            i_mmu_mem.imem_addr = i_root_addr + i_vpn1 * 4;
                        }
                    }
                    else
                    {
                        // execute mem operation
                        i_state.write(3);
                        i_mmu_mem.imem_req_ack = 1;
                        i_mmu_mem.imem_addr = (pc & ~3);
                        is_fetch = 1;
                    }
                }
                else
                {
                    i_state.write(0);
                    i_mmu_mem.imem_req_ack = 0;
                }
            }
            else if (rsp.mem_rsp_ack)
            {
                if (i_state.read() == 1)
                {
                    i_pte = rsp.mem_rsp_rdata;
                    i_ppn = (i_pte >> 10) & 0xFFFFF;
                    uint8_t DAUGXWRV = rsp.mem_rsp_rdata & 0xFF;
                    _i_page_fault = (i_pte & 1) == 0 ||
                                    (DAUGXWRV != 1 &&
                                     (((i_ppn & 0x3FF) != 0) || (8 & i_pte) == 0 || (i_pte & ACCESS_BIT) == 0 ||
                                      (exec->priv == RV_PRIV_S_MODE && USERBIT(i_pte)) ||
                                      (exec->priv == RV_PRIV_U_MODE && USERBIT(i_pte) == 0) ||
                                      ((i_pte & 6) == 4)));

                    if (_i_page_fault)
                        i_mmu_mem.imem_req_ack = 0;
                    else
                    {
                        i_mmu_mem.imem_req_ack = 1;
                        if ((DAUGXWRV & 0xE) != 0) // megapage
                        {
                            is_fetch = 1;
                            i_mmu_mem.imem_addr = (i_ppn << 12) + (immu_pc.read() & 0x3FFFFC); // 0x3FFFFF
                            i_state.write(3);
                            exec->tlb.store((i_satp >> 20) & 0x1ff, (immu_pc.read() >> 12), i_pte, 1);
                        }
                        else
                        {
                            i_mmu_mem.imem_addr = (i_ppn << 12) + i_vpn0 * 4;
                            i_state.write(2);
                        }
                    }
                }
                else if (i_state == 2)
                {
                    i_pte = rsp.mem_rsp_rdata;
                    i_ppn = (i_pte >> 10) & 0xFFFFF;
                    // uint8_t DAUGXWRV = rsp.mem_rsp_rdata & 0xFF;
                    // TODO : check pte reserve bits setting
                    _i_page_fault = (i_pte & 1) == 0 || (8 & i_pte) == 0 || (i_pte & ACCESS_BIT) == 0 ||
                                    (exec->priv == RV_PRIV_S_MODE && USERBIT(i_pte)) ||
                                    (exec->priv == RV_PRIV_U_MODE && USERBIT(i_pte) == 0) ||
                                    ((i_pte & 6) == 4);
                    if (_i_page_fault)
                        i_mmu_mem.imem_req_ack = 0;
                    else
                    {
                        is_fetch = 1;
                        i_mmu_mem.imem_req_ack = 1;
                        i_mmu_mem.imem_addr = (i_ppn << 12) + (i_offset & ~3);
                        i_state.write(3);
                        exec->tlb.store((i_satp >> 20) & 0x1ff, (immu_pc.read() >> 12), i_pte, 0);
                    }
                }
                else
                {
                    i_state.write(0);
                    i_mmu_mem.imem_req_ack = 0;
                }
            }
            else
            {
                i_mmu_mem.imem_req_ack = 0;
                _i_page_fault = 0;
            }

            if (i_mmu_mem.imem_req_ack)
            {
                if (is_fetch)
                    _i_pmp_fault = (!exec->pmp_icheck(i_mmu_mem.imem_addr,
                                                      4,
                                                      4)) ||
                                   (!is_pma_valid(i_mmu_mem.imem_addr));
                else
                    _i_pmp_fault = !exec->pmp_icheck(i_mmu_mem.imem_addr,
                                                     1,
                                                     4) ||
                                   (!is_pma_valid(i_mmu_mem.imem_addr));
            }
            else
                _i_pmp_fault = false;
            if (_i_pmp_fault || _i_page_fault)
            {
                immu_to_mem.write(IMEM_IN());
            }
            else
            {
                immu_to_mem.write(i_mmu_mem);
            }
            i_page_fault.write(_i_page_fault);
            i_pmp_fault.write(_i_pmp_fault);
            // if (i_mmu_mem.imem_addr == 0x807e71f4)
            // {
            //     std::cerr << "Kernel start page fault = " << _i_page_fault << "  pmp fault = " << _i_pmp_fault << "TTTTTTTTTTTTTTT\n";
            // }
        }
    }
    void inst_monitor()
    {
        std::cout << "Instruction paddr req " << immu_to_mem.read() << "\n";
        std::cout << "Instruction get " << mem_to_immu.read() << "\n";
    }
    void data_monitor()
    {
        std::cout << "\nData paddr req " << dmmu_to_mem << " Dmmu state " << (int)d_state;
        std::cout << "\nData access rsp " << mem_to_dmmu;
        std::cout << "\nTo LSU rsp " << dmmu_to_ex;
    }
};
