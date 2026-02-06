#pragma once
#include "csr_cfg.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <systemc>
#include "pmp.hpp"
#include "memory.hpp"
#include "decode.hpp"
#include "tlb.hpp"
SC_MODULE(EXEC)
{
    uint32_t priv;
    PMP pmp;
    sc_in<bool> clk;
    sc_in<bool> rst;

    sc_out<uint32_t> ex_pc;
    sc_in<ID_EX> id_ex_port;
    // mext
    sc_out<bool> mext_req_ack;
    sc_in<bool> mext_rsp_ack;
    sc_in<uint32_t> mext_result;

    // dmem
    sc_in<MMU_RSP> dmmu_to_ex;

    sc_out<bool> stall;
    sc_out<bool> flush;
    sc_out<uint32_t> npc;

    sc_out<DMEM_IN> ex_to_dmmu;
    // forwarding signal
    sc_out<bool> ex_write_rd;
    sc_out<uint32_t> ex_rd_val;
    sc_out<uint8_t> ex_rd;

    // write back
    sc_out<bool> wb_stall;
    sc_out<bool> wb_write_rd;
    sc_out<uint32_t> wb_rd_val;
    sc_out<uint8_t> wb_rd;
    sc_out<uint32_t> wb_pc;
    sc_out<InstrType> wb_type;

    uint32_t _ex_rd_val;
    uint32_t entry;
    sc_vector<sc_signal<uint32_t>> csr_regs;
    PLIC_DEV *plic;
    CLINT_DEV *clint;
    TLB tlb;
    uint32_t _val = 0;
    bool _on_trap = 0, _stall = 0, _flush = 0;
    bool _csr_access = 0,
         _csr_mret = 0, _csr_sret = 0;
    uint32_t _cause = 0, _tval = 0, _epc = 0;

    bool _branch_taken;
    SC_HAS_PROCESS(EXEC);
    EXEC(sc_module_name name, CLINT_DEV * clint, uint32_t entry, PLIC_DEV *plic = 0) : sc_module(name), pmp("pmp"), clint(clint), plic(plic), entry(entry)
    {
        csr_regs.init(4096);
        priv = 3;
        reset_csr_registers();
        SC_METHOD(exec_comb);
        sensitive << clk.neg();
        SC_THREAD(exec_seq);
        sensitive << clk.pos();
        SC_METHOD(writeback_seq);
        sensitive << clk.pos();
    }
    void reset_csr_registers()
    {
        // Reset/Initial values (Non-blocking initialization)
        csr_regs[CSR_MSTATUS].write(0x00001800);
        // MISA is set in the constructor to reflect implemented ISA
        csr_regs[CSR_MEDELEG].write(0);
        csr_regs[CSR_MIDELEG].write(0);
        csr_regs[CSR_MIE].write(0);
        csr_regs[CSR_MISA].write((1u << 30) | (1 << 8) | (1 << 12) | (1 << ('U' - 'A')) | (1 << ('S' - 'A')) | (1 << ('C' - 'A')) | 1);
        csr_regs[CSR_MTVEC].write(0);
        csr_regs[CSR_MSCRATCH].write(0);
        csr_regs[CSR_MEPC].write(0);
        csr_regs[CSR_MCAUSE].write(0);
        csr_regs[CSR_MTVAL].write(0);
        csr_regs[CSR_MIP].write(0);

        csr_regs[CSR_STVEC].write(0);
        csr_regs[CSR_SSCRATCH].write(0);
        csr_regs[CSR_SEPC].write(0);
        csr_regs[CSR_SCAUSE].write(0);
        csr_regs[CSR_STVAL].write(0);
        csr_regs[CSR_SATP].write(0);
    }
    uint32_t get_mip()
    {
        uint32_t mip = csr_regs[CSR_MIP].read();
        if (clint->mtime > (((1ull * csr_regs[CSR_STIMECMPH]) << 32) | csr_regs[CSR_STIMECMP]))
            mip |= MIP_STIP;
        if (clint->mtime > clint->mtimecmp)
            mip |= MIP_MTIP;
        if (plic && plic->core0_m_irq != 0)
            mip |= MIP_MEIP;
        if (plic && plic->core0_s_irq != 0)
            mip |= MIP_SEIP;
        return mip;
    }
    void monitor()
    {
        auto id_ex = id_ex_port.read();
        std::cout << "EXE : ";
        if (id_ex.flush || id_ex.pc != ex_pc.read())
        {
            std::cout << "[NOP]";
            return;
        }
        std::cout << "PC=" << std::hex << id_ex.pc << "  ";
        std::cout << std::left << to_string(id_ex.type);

        // ----------------------------
        // ALU ops
        // ----------------------------
        switch (id_ex.type)
        {
            // ----------------------------
        // Immediate ALU ops
        // ----------------------------
        case InstrType::ADDI:
        case InstrType::SLTI:
        case InstrType::SLTIU:
        case InstrType::XORI:
        case InstrType::ORI:
        case InstrType::ANDI:
        case InstrType::SLLI:
        case InstrType::SRLI:
        case InstrType::SRAI:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " rs1=x" << unsigned(id_ex.rs1)
                      << " imm=0x" << std::hex << id_ex.imm
                      << " | op1=" << id_ex.rs1_val
                      << " op2=" << id_ex.imm << ", result=0x" << std::hex << _ex_rd_val;
            break;

        // ----------------------------
        // Register-register ALU ops
        // ----------------------------
        case InstrType::ADD:
        case InstrType::SUB:
        case InstrType::SLL:
        case InstrType::SLT:
        case InstrType::SLTU:
        case InstrType::XOR:
        case InstrType::SRL:
        case InstrType::SRA:
        case InstrType::OR:
        case InstrType::AND:
        case InstrType::MUL:
        case InstrType::MULH:
        case InstrType::MULHSU:
        case InstrType::MULHU:
        case InstrType::DIV:
        case InstrType::DIVU:
        case InstrType::REM:
        case InstrType::REMU:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " rs1=x" << unsigned(id_ex.rs1)
                      << " rs2=x" << unsigned(id_ex.rs2)
                      << " | op1=" << id_ex.rs1_val
                      << " op2=" << id_ex.rs2_val << ", result=0x" << std::hex << _ex_rd_val;
            break;

        // ----------------------------
        // Load / Store
        // ----------------------------
        case InstrType::LB:
        case InstrType::LH:
        case InstrType::LW:
        case InstrType::LBU:
        case InstrType::LHU:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " base=x" << unsigned(id_ex.rs1)
                      << " offset=0x" << std::hex << id_ex.imm
                      << " | base_val=" << id_ex.rs1_val << " addr=0x" << std::hex << ex_to_dmmu.read().dmem_addr;

            if (!dmmu_to_ex.read().mmu_rsp_ack)
                std::cout << " (loading)";
            else if (dmmu_to_ex.read().mmu_rsp_fault == 0)
                std::cout << " result = " << dmmu_to_ex.read().mmu_rsp_rdata;
            else
                std::cout << " fault = " << (uint32_t)dmmu_to_ex.read().mmu_rsp_fault;
            break;
        case InstrType::SB:
        case InstrType::SH:
        case InstrType::SW:
            std::cout << " base=x" << std::hex << unsigned(id_ex.rs1)
                      << " offset=0x" << std::hex << id_ex.imm
                      << " | base_val=" << id_ex.rs1_val
                      << " addr=0x" << std::hex << ex_to_dmmu.read().dmem_addr
                      << " src=x" << unsigned(id_ex.rs2)
                      << " store_val=0x" << id_ex.rs2_val
                      << " (store)";
            break;

        // ----------------------------
        // Branch / Jump
        // ----------------------------
        case InstrType::BEQ:
        case InstrType::BNE:
        case InstrType::BLT:
        case InstrType::BGE:
        case InstrType::BLTU:
        case InstrType::BGEU:
            std::cout << " rs1=x" << std::hex << unsigned(id_ex.rs1)
                      << " rs2=x" << unsigned(id_ex.rs2)
                      << " target=0x" << std::hex << (id_ex.pc + id_ex.imm)
                      << " | op1=" << id_ex.rs1_val
                      << " op2=" << id_ex.rs2_val
                      << " branch_taken=" << (_branch_taken ? "yes" : "no")
                      << " target=0x" << std::hex << _ex_rd_val;
            break;

        case InstrType::JAL:
        case InstrType::JALR:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " base=x" << unsigned(id_ex.rs1)
                      << " target=0x" << std::hex << _ex_rd_val
                      << " next_pc=0x" << std::hex << npc;
            break;
        // ----------------------------
        // LUI / AUIPC
        // ----------------------------
        case InstrType::LUI:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " result=0x" << std::hex << _ex_rd_val;
            break;

        case InstrType::AUIPC:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " imm=0x" << std::hex << id_ex.imm
                      << " pc=0x" << std::hex << id_ex.pc
                      << " | result=0x" << std::hex << _ex_rd_val;
            break;

        // ----------------------------
        // Atomic ops
        // ----------------------------
        case InstrType::LR_W:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " addr=rs1(x" << unsigned(id_ex.rs1)
                      << ")=" << std::hex << ex_to_dmmu.read().dmem_addr
                      << " (load-reserve)";
            if (!dmmu_to_ex.read().mmu_rsp_ack)
                std::cout << " (loading)";
            else if (dmmu_to_ex.read().mmu_rsp_fault == 0)
                std::cout << " result = 0x" << std::hex << dmmu_to_ex.read().mmu_rsp_rdata;
            else
                std::cout << " fault = 0x" << std::hex << dmmu_to_ex.read().mmu_rsp_fault;
            break;

        case InstrType::SC_W:
        case InstrType::AMOSWAP_W:
        case InstrType::AMOADD_W:
        case InstrType::AMOXOR_W:
        case InstrType::AMOAND_W:
        case InstrType::AMOOR_W:
        case InstrType::AMOMIN_W:
        case InstrType::AMOMAX_W:
        case InstrType::AMOMINU_W:
        case InstrType::AMOMAXU_W:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " base=x" << unsigned(id_ex.rs1)
                      << " | addr=0x" << std::hex << ex_to_dmmu.read().dmem_addr
                      << " src=x" << unsigned(id_ex.rs2)
                      << " atomic_val=0x" << id_ex.rs2_val;
            if (!dmmu_to_ex.read().mmu_rsp_ack)
                std::cout << " (loading)";
            else if (dmmu_to_ex.read().mmu_rsp_fault == 0)
                std::cout << " result = " << dmmu_to_ex.read().mmu_rsp_rdata;
            else
                std::cout << " fault = " << dmmu_to_ex.read().mmu_rsp_fault;

            break;
        // ----------------------------
        // CSR instructions
        // ----------------------------
        case InstrType::CSRRW:
        case InstrType::CSRRS:
        case InstrType::CSRRC:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " csr=0x" << std::hex << id_ex.csr_addr
                      << " rs1=x" << std::hex << unsigned(id_ex.rs1)
                      << " val=0x" << id_ex.rs1_val
                      << " result=0x" << _ex_rd_val
                      << " wdata=0x" << _val;
            break;

        case InstrType::CSRRWI:
        case InstrType::CSRRSI:
        case InstrType::CSRRCI:
            std::cout << " rd=x" << std::hex << unsigned(id_ex.rd)
                      << " csr=0x" << std::hex << id_ex.csr_addr
                      << " imm=0x" << std::hex << unsigned(id_ex.rs1)
                      << " result=0x" << _ex_rd_val
                      << " wdata=0x" << _val;
            break;
        // ----------------------------
        // System / Fence
        // ----------------------------
        case InstrType::ECALL:
        case InstrType::EBREAK:
        case InstrType::MRET:
        case InstrType::SRET:
            std::cout << " SYSTEM instruction";
            break;

        case InstrType::FENCE:
        case InstrType::FENCE_I:
        case InstrType::SFENCE_VMA:
            std::cout << " FENCE instruction";
            break;
        case InstrType::INVALID:
            std::cout << " [unknown or invalid]";
            break;
        default:
            break;
        }

        // // ----------------------------
        // // Common suffix
        // // ----------------------------
        if (stall.read())
        {
            uint32_t mip = get_mip();

            std::cout << std::hex << " [stalled], MIP = " << mip << ", MIE = " << csr_regs[CSR_MIE];
        }
        if (_on_trap)
        {
            std::cout << " [trapped no. " << _cause << ", tval = " << _tval << " epc=" << _epc << "]";
        }
    }

    uint32_t trap_pc(uint32_t _cause)
    {

        bool is_interrupt = (_cause & 0x80000000u) != 0;
        uint32_t _cause_id = _cause & 0x3ff; // small mask for _cause id

        // Read delegation CSRs
        uint32_t medeleg = csr_regs[CSR_MEDELEG].read();
        uint32_t mideleg = csr_regs[CSR_MIDELEG].read();
        bool delegated_to_s = false;

        if (is_interrupt)
        {
            if (priv < RV_PRIV_M_MODE && (csr_regs[CSR_MSTATUS].read() & MSTATUS_SIE))
                delegated_to_s = (mideleg >> _cause_id) & 1u;
        }
        else
        {
            if (priv < RV_PRIV_M_MODE)
                delegated_to_s = (medeleg >> _cause_id) & 1u;
        }
        if (delegated_to_s)
        {
            // Return trap handler address
            uint32_t stvec = csr_regs[CSR_STVEC].read();
            // priv = RV_PRIV_S_MODE;
            // std::cout << "   delegated_to_s and stvec is  " << stvec << " _cause " << _cause << "   \n";
            if (stvec & 1)
            {
                return (stvec & ~3) + 4 * (_cause_id & 0x3ff);
            }
            else
                return (stvec & ~1);
            // return (stvec & ~3);
        }
        else
        {
            // Return trap handler address
            uint32_t mtvec = csr_regs[CSR_MTVEC].read();
            // priv = RV_PRIV_M_MODE;
            // std::cout << " not  delegated_to_s and mtvec is  " << mtvec << " _cause " << _cause << "   \n";
            if (mtvec & 1)
            {
                return (mtvec & ~3) + 4 * (_cause_id & 0x3ff);
            }
            else
                return (mtvec & ~1);
            // return (mtvec & ~3);
        }
    }

    void exec_comb()
    {
        _on_trap = 0, _stall = 0, _flush = 0;
        _csr_access = 0, _csr_mret = 0, _csr_sret = 0;
        _cause = 0, _tval = 0, _epc = ex_pc.read();
        _branch_taken = 0;
        _ex_rd_val = 0;

        auto id_ex = id_ex_port.read();
        auto dmmu_rsp = dmmu_to_ex.read();
        uint32_t rs1_val = id_ex.rs1_val, rs2_val = id_ex.rs2_val;
        uint32_t _npc = ex_pc.read();
        bool _csr_write = 0;

        DMEM_IN mmu_req;
        if (id_ex.is_atomic)
            mmu_req.dmem_addr = rs1_val;
        else
            mmu_req.dmem_addr = rs1_val + id_ex.imm;
        mmu_req.dmem_wdata = rs2_val;
        mmu_req.dmem_is_amo = id_ex.is_atomic;
        mmu_req.dmem_amoop = id_ex.amo_type;
        mmu_req.dmem_is_load = id_ex.is_load;
        mmu_req.dmem_is_store = id_ex.is_store;
        mmu_req.dmem_req_ack = false;
        mmu_req.dmem_size = id_ex.mem_size;

        if (rst.read())
        {
            mmu_req.dmem_req_ack = 0;
            mmu_req.dmem_is_amo = 0;
            mmu_req.dmem_is_load = 0;
            mmu_req.dmem_is_store = 0;
            _npc = entry;
        }
        else if (((!wb_stall.read() || id_ex.type == InstrType::WFI) && check_interrupt(&_cause)))
        {
            _tval = 0; // ?? check
            _on_trap = 1;
            _stall = 0;
        }
        else if (!id_ex.flush)
        {
            if (id_ex.is_compressed)
                _npc = ex_pc.read() + 2;
            else
                _npc = ex_pc.read() + 4;
            if (id_ex.on_trap)
            {
                _cause = id_ex.cause;
                _tval = id_ex.tval; // ?? check
                _on_trap = true;
            }
            else
            {
                switch (id_ex.type)
                {
                // ===================================================
                // LUI / AUIPC
                // ===================================================
                case InstrType::LUI:
                    _ex_rd_val = id_ex.imm;
                    break;
                case InstrType::AUIPC:
                    _ex_rd_val = id_ex.pc + id_ex.imm;
                    break;
                // ===================================================
                // ALU Immediate
                // ===================================================
                case InstrType::ADDI:
                    _ex_rd_val = rs1_val + id_ex.imm;
                    break;
                case InstrType::SLTI:
                    _ex_rd_val = ((int32_t)rs1_val < (int32_t)id_ex.imm);
                    break;
                case InstrType::SLTIU:
                    _ex_rd_val = (rs1_val < (uint32_t)id_ex.imm);
                    break;
                case InstrType::XORI:
                    _ex_rd_val = rs1_val ^ id_ex.imm;
                    break;
                case InstrType::ORI:
                    _ex_rd_val = rs1_val | id_ex.imm;
                    break;
                case InstrType::ANDI:
                    _ex_rd_val = rs1_val & id_ex.imm;
                    break;
                case InstrType::SLLI:
                    _ex_rd_val = rs1_val << (id_ex.imm & 0x1F);
                    break;
                case InstrType::SRLI:
                    _ex_rd_val = rs1_val >> (id_ex.imm & 0x1F);
                    break;
                case InstrType::SRAI:
                    _ex_rd_val = (int32_t)rs1_val >> (id_ex.imm & 0x1F);
                    break;
                    // ===================================================
                // ALU Register
                // ===================================================
                case InstrType::ADD:
                    _ex_rd_val = rs1_val + rs2_val;
                    break;
                case InstrType::SUB:
                    _ex_rd_val = rs1_val - rs2_val;
                    break;
                case InstrType::SLL:
                    _ex_rd_val = rs1_val << (rs2_val & 0x1F);
                    break;
                case InstrType::SLT:
                    _ex_rd_val = ((int32_t)rs1_val < (int32_t)rs2_val);
                    break;
                case InstrType::SLTU:
                    _ex_rd_val = (rs1_val < rs2_val);
                    break;
                case InstrType::XOR:
                    _ex_rd_val = rs1_val ^ rs2_val;
                    break;
                case InstrType::SRL:
                    _ex_rd_val = rs1_val >> (rs2_val & 0x1F);
                    break;
                case InstrType::SRA:
                    _ex_rd_val = ((int32_t)rs1_val >> (rs2_val & 0x1F));
                    break;
                case InstrType::OR:
                    _ex_rd_val = rs1_val | rs2_val;
                    break;
                case InstrType::AND:
                    _ex_rd_val = rs1_val & rs2_val;
                    break;
                // M-extension
                case InstrType::MUL:
                case InstrType::MULH:
                case InstrType::MULHSU:
                case InstrType::MULHU:
                case InstrType::DIV:
                case InstrType::DIVU:
                case InstrType::REM:
                case InstrType::REMU:
                    mext_req_ack.write(!wb_stall.read());
                    _stall = !mext_rsp_ack;
                    _ex_rd_val = mext_result;
                    break;
                // ===================================================
                // Load / Store
                // ===================================================
                case InstrType::LB:
                case InstrType::LH:
                case InstrType::LW:
                case InstrType::LBU:
                case InstrType::LHU:
                case InstrType::SB:
                case InstrType::SH:
                case InstrType::SW:
                // ===================================================
                // Atomic operation
                // ===================================================
                case InstrType::LR_W:
                case InstrType::SC_W:
                case InstrType::AMOSWAP_W:
                case InstrType::AMOADD_W:
                case InstrType::AMOXOR_W:
                case InstrType::AMOAND_W:
                case InstrType::AMOOR_W:
                case InstrType::AMOMIN_W:
                case InstrType::AMOMAX_W:
                case InstrType::AMOMINU_W:
                case InstrType::AMOMAXU_W:
                    _tval = dmmu_rsp.mmu_rsp_tval;
                    _stall = !dmmu_rsp.mmu_rsp_ack;
                    _cause = dmmu_rsp.mmu_rsp_fault;
                    _on_trap = _cause != 0;
                    mmu_req.dmem_req_ack = !wb_stall.read();
                    break;
                // ===================================================
                // Jump
                // ===================================================
                case InstrType::JAL:
                    if (id_ex.is_compressed)
                        _ex_rd_val = id_ex.pc + 2;
                    else
                        _ex_rd_val = id_ex.pc + 4;
                    _npc = id_ex.pc + id_ex.imm;
                    if (_npc & 1)
                    {
                        _stall = 0;
                        _tval = _npc;
                        _cause = INSTR_ADDR_MISALIGNED;
                        _on_trap = true;
                    }
                    break;

                case InstrType::JALR:
                    if (id_ex.is_compressed)
                        _ex_rd_val = id_ex.pc + 2;
                    else
                        _ex_rd_val = id_ex.pc + 4;
                    _npc = (rs1_val + id_ex.imm) & ~1u;
                    if (_npc & 1)
                    {
                        _stall = 0;
                        _tval = _npc;
                        _cause = INSTR_ADDR_MISALIGNED;
                        _on_trap = true;
                    }
                    break;
                // ===================================================
                // Branch
                // ===================================================
                case InstrType::BEQ:
                    if (rs1_val == rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                case InstrType::BNE:
                    if (rs1_val != rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                case InstrType::BLT:
                    if ((int32_t)rs1_val < (int32_t)rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                case InstrType::BGE:
                    if ((int32_t)rs1_val >= (int32_t)rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                case InstrType::BLTU:
                    if (rs1_val < rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                case InstrType::BGEU:
                    if (rs1_val >= rs2_val)
                    {
                        _npc = _ex_rd_val = id_ex.pc + id_ex.imm;
                        _branch_taken = 1;
                        if (_npc & 1)
                        {
                            _stall = 0;
                            _tval = _npc;
                            _cause = INSTR_ADDR_MISALIGNED;
                            _on_trap = true;
                        }
                    }
                    break;

                // ===================================================
                // System instructions
                // ===================================================
                case InstrType::FENCE_I:
                    _flush = 1;
                    _cause = 0;
                    _on_trap = 0;
                    break;
                case InstrType::SFENCE_VMA:
                    if (priv == RV_PRIV_U_MODE ||
                        (priv == RV_PRIV_S_MODE && (csr_regs[CSR_MSTATUS].read() & MSTATUS_TVM)))
                    {
                        _cause = ILLEGAL_INSTR;
                        _on_trap = true;
                    }
                    else
                    {
                        tlb.flush(id_ex.rs2_val, id_ex.rs1_val);
                        _flush = 1;
                        _cause = 0;
                        _on_trap = 0;
                    }
                    break;
                case InstrType::ECALL:
                    // csr.raise__on_trap(11, id_ex.pc); // ECALL from M-mode
                    _epc = id_ex.pc;
                    if (priv == RV_PRIV_M_MODE)
                        _cause = ECALL_M;
                    else if (priv == RV_PRIV_S_MODE)
                        _cause = ECALL_S;
                    else
                        _cause = ECALL_U;
                    _on_trap = true;
                    break;

                case InstrType::EBREAK:
                    _cause = BREAKPOINT;
                    _on_trap = true;
                    _tval = id_ex.pc; // TODO : CHECK THIS (just for match spike ouput on riscv-arch-tests)
                    break;
                case InstrType::WFI:
                    if (priv < RV_PRIV_S_MODE ||
                        ((priv == RV_PRIV_S_MODE) &&
                         (csr_regs[CSR_MSTATUS].read() & MSTATUS_TW) != 0))
                    {
                        _cause = ILLEGAL_INSTR;
                        _on_trap = true;
                    }
                    else
                    {
                        _stall = (csr_regs[CSR_MIE] & get_mip()) == 0;
                    }
                    break;
                case InstrType::SRET:
                    if ((priv == RV_PRIV_S_MODE) &&
                        (csr_regs[CSR_MSTATUS].read() & MSTATUS_TSR) != 0)
                    {
                        _cause = ILLEGAL_INSTR;
                        _on_trap = true;
                    }
                    else
                    {
                        _csr_sret = true;
                        _flush = true;
                    }
                    break;

                case InstrType::MRET:
                {
                    _csr_mret = true;
                    _flush = true;
                    break;
                }
                // ===================================================
                // CSR operations
                // ===================================================
                case InstrType::CSRRW:
                case InstrType::CSRRWI:
                    _csr_write = 1;
                case InstrType::CSRRS:
                case InstrType::CSRRC:
                case InstrType::CSRRSI:
                case InstrType::CSRRCI:
                {
                    if ((((id_ex.csr_addr >> 8) & 3) <= priv) &&
                        (((id_ex.csr_addr & 0xc00) != 0xc00) ||
                         (!_csr_write && id_ex.csr_val == 0)))
                    // if ((((id_ex.csr_addr >> 8) & 3) <= priv))
                    {
                        _csr_access = 1;
                        switch (id_ex.csr_addr)
                        {
                        // Machine Mode
                        case CSR_MSTATUS:
                            _ex_rd_val = csr_regs[CSR_MSTATUS].read() & MSTATUS_MASK;
                            break;
                        case CSR_MIP:
                            _ex_rd_val = (get_mip() & MIP_RMASK);
                            break;
                        case CSR_MIDELEG:
                            _ex_rd_val = csr_regs[CSR_MIDELEG].read();
                            break;
                        case CSR_MCYCLE:
                            if (priv > RV_PRIV_S_MODE || ((priv == RV_PRIV_S_MODE) && (csr_regs[CSR_MCOUNTEREN].read() & 1)))
                                _ex_rd_val = csr_regs[CSR_MCYCLE].read();
                            else
                            {
                                _on_trap = 1;
                                _cause = ILLEGAL_INSTR;
                                _tval = id_ex.inst;
                            }
                            break;
                        case CSR_MCYCLEH:
                            if (priv > RV_PRIV_S_MODE || ((priv == RV_PRIV_S_MODE) && (csr_regs[CSR_MCOUNTEREN].read() & 1)))
                                _ex_rd_val = csr_regs[CSR_MCYCLEH].read();
                            else
                            {
                                _on_trap = 1;
                                _cause = ILLEGAL_INSTR;
                                _tval = id_ex.inst;
                            }
                            break;
                        case CSR_MINSTRET:
                            if (priv > RV_PRIV_S_MODE || ((priv == RV_PRIV_S_MODE) && (csr_regs[CSR_MCOUNTEREN].read() & 4)))
                                _ex_rd_val = csr_regs[CSR_MINSTRET].read();
                            else
                            {
                                _on_trap = 1;
                                _cause = ILLEGAL_INSTR;
                                _tval = id_ex.inst;
                            }
                            break;
                        case CSR_MINSTRETH:
                            if (priv > RV_PRIV_S_MODE || ((priv == RV_PRIV_S_MODE) && (csr_regs[CSR_MCOUNTEREN].read() & 4)))
                                _ex_rd_val = csr_regs[CSR_MINSTRETH].read();
                            else
                            {
                                _on_trap = 1;
                                _cause = ILLEGAL_INSTR;
                                _tval = id_ex.inst;
                            }
                            break;
                        // Supervisor Mode
                        case CSR_SSTATUS:
                            _ex_rd_val = (csr_regs[CSR_MSTATUS].read() & SSTATUS_MASK);
                            break;
                        case CSR_SIE:
                            // TODO:check
                            _ex_rd_val = (csr_regs[CSR_MIE].read() & csr_regs[CSR_MIDELEG].read());
                            break;
                        case CSR_SIP:
                            _ex_rd_val = csr_regs[CSR_MIP].read();
                            if (clint->mtime > (((1ull * csr_regs[CSR_STIMECMPH]) << 32) | csr_regs[CSR_STIMECMP]))
                                _ex_rd_val |= MIP_STIP;
                            if (plic && plic->core0_s_irq)
                                _ex_rd_val |= MIP_SEIP;
                            _ex_rd_val &= csr_regs[CSR_MIDELEG].read();
                            break;
                        case CSR_SATP:
                            if (priv > RV_PRIV_S_MODE || ((priv == RV_PRIV_S_MODE) && (csr_regs[CSR_MSTATUS].read() & MSTATUS_TVM) == 0))
                            {
                                _ex_rd_val = csr_regs[CSR_SATP].read();
                                _flush = true;
                            }
                            else
                            {
                                _on_trap = 1;
                                _cause = ILLEGAL_INSTR;
                                _tval = id_ex.inst;
                            }
                            break;
                        case CSR_CYCLE:
                            _ex_rd_val = csr_regs[CSR_MCYCLE].read();
                            break;
                        case CSR_CYCLEH:
                            _ex_rd_val = csr_regs[CSR_MCYCLEH].read();
                            break;
                        default:
                            if (id_ex.csr_addr >= PMPCONFIG0 && id_ex.csr_addr < PMPCONFIG0 + (PMPCOUNT >> 2))
                            {
                                for (int t = 3, b = (id_ex.csr_addr - PMPCONFIG0) << 2; t >= 0; t--)
                                {
                                    _ex_rd_val <<= 8;
                                    _ex_rd_val = pmp.pmpconfig[b + t];
                                }
                            }
                            else if (id_ex.csr_addr >= PMPADDR0 && id_ex.csr_addr < PMPADDR0 + PMPCOUNT)
                                _ex_rd_val = pmp.pmpaddr[id_ex.csr_addr - PMPADDR0];

                            else
                            {
                                _ex_rd_val = csr_regs[id_ex.csr_addr].read();
                            }

                            _flush = priv == 3 && ((id_ex.csr_addr >= PMPCONFIG0 && id_ex.csr_addr < PMPCONFIG0 + (PMPCOUNT >> 2)) ||
                                                   (id_ex.csr_addr >= PMPADDR0 && id_ex.csr_addr < PMPADDR0 + PMPCOUNT));
                            break;
                        }
                    }
                    else
                    {
                        _on_trap = 1;
                        _cause = ILLEGAL_INSTR;
                        _tval = id_ex.inst;
                    }
                }
                break;
                case InstrType::INVALID:
                    _cause = ILLEGAL_INSTR;
                    _on_trap = true;
                    _tval = id_ex.inst;
                    break;
                default:
                    break;
                }
            }
        }

        if (_on_trap)
        {
            npc.write(trap_pc(_cause));
            ex_write_rd.write(0);
            ex_rd_val.write(0);
        }
        else if (_csr_sret)
        {
            npc.write(csr_regs[CSR_SEPC].read() & ~1);
            ex_write_rd.write(0);
            ex_rd_val.write(0);
        }
        else if (_csr_mret)
        {
            npc.write(csr_regs[CSR_MEPC].read() & ~1);
            ex_write_rd.write(0);
            ex_rd_val.write(0);
        }
        else if (_stall)
        {
            npc.write(ex_pc.read());
            ex_write_rd.write(0);
            ex_rd_val.write(0);
        }
        else
        {

            npc.write(_npc);
            ex_write_rd.write(id_ex.writes_rd);
            if (dmmu_rsp.mmu_rsp_ack)
            {
                if (id_ex.type == InstrType::LB)
                    ex_rd_val.write(sign_extend(dmmu_rsp.mmu_rsp_rdata, 8));
                else if (id_ex.type == InstrType::LH)
                    ex_rd_val.write(sign_extend(dmmu_rsp.mmu_rsp_rdata, 16));
                else
                    ex_rd_val.write(dmmu_rsp.mmu_rsp_rdata);
            }
            else
            {
                ex_rd_val.write(_ex_rd_val);
            }
        }
        ex_to_dmmu.write(mmu_req);
        ex_rd.write(id_ex.rd);
        stall.write(_stall);
        flush.write(_on_trap || _flush);
    }

    uint32_t csr_mask(uint32_t addr)
    {

        switch (addr)
        {
        case CSR_MSTATUS:
            return MSTATUS_MASK;
        case CSR_MIP:
            return MIP_WMASK;
        case CSR_SSTATUS:
            return SSTATUS_MASK;
        case CSR_SIP:
            return SIP_WMASK;
        case CSR_SIE:
            return csr_regs[CSR_MIDELEG].read();
        case CSR_MISA:
        case CSR_TSELECT:
        case CSR_TDATA1:
        case CSR_TDATA2:
        case CSR_TDATA3:
        case CSR_MCONTEXT:
        case CSR_MHARTID:
            return 0;
        default:
            return -1;
        }
    }
    uint32_t csr_wdata(uint32_t addr, uint32_t cur_val)
    {

        auto id_ex = id_ex_port.read();
        uint32_t mask = csr_mask(addr);
        // std::cout << "mask  " << mask << "\n";
        switch (id_ex.type)
        {
        case InstrType::CSRRW:
            return (cur_val & ~mask) | (id_ex.csr_val & mask);
        case InstrType::CSRRS:
            return cur_val | (id_ex.csr_val & mask);
        case InstrType::CSRRC:
            return cur_val & ~(id_ex.csr_val & mask);
        case InstrType::CSRRWI:
            return (cur_val & ~mask) | (id_ex.csr_val & mask);
        case InstrType::CSRRSI:
            return cur_val | (id_ex.csr_val & mask);
        case InstrType::CSRRCI:
            return cur_val & ~(id_ex.csr_val & mask);
        default:
            return cur_val;
        }
    }

    void exec_seq()
    {
        wait();
        while (1)
        {
            wait();
            bool mcycle_write = 0, minstret_write = 0, mcycleh_write = 0, minstreth_write = 0;
            auto id_ex = id_ex_port.read();
            uint32_t addr = id_ex.csr_addr;
            bool inst_ret = (_on_trap) ? ((_cause == ECALL_M || _cause == ECALL_S || _cause == ECALL_U || _cause == BREAKPOINT) ||
                                          ((_cause & 0x80000000) != 0 && id_ex.type == InstrType::WFI))
                                       : !stall.read();

            _val = csr_wdata(addr, _ex_rd_val);
            if (_on_trap)
            {
                trap();
            }
            else if (_csr_sret || _csr_mret)
            {
                return_from_trap(_csr_mret);
            }
            else
            {
                if (_csr_access)
                {
                    switch (addr)
                    {
                    case CSR_SSTATUS:
                        _val = csr_wdata(addr, csr_regs[CSR_MSTATUS]);
                        csr_regs[CSR_MSTATUS].write(_val);
                        break;
                    case CSR_SIE:
                        _val = csr_wdata(addr, csr_regs[CSR_MIE]);
                        csr_regs[CSR_MIE].write(_val);
                        break;
                    case CSR_SIP:
                    case CSR_MIP:
                        _val = csr_wdata(addr, csr_regs[CSR_MIP]);
                        csr_regs[CSR_MIP].write(_val);
                        break;
                    // external update
                    case CSR_MCYCLE:
                    case CSR_MCYCLEH:
                    case CSR_MINSTRET:
                    case CSR_MINSTRETH:

                    // fixed of unimplement
                    case CSR_MISA:
                    case CSR_TSELECT:
                    case CSR_TDATA1:
                    case CSR_TDATA2:
                    case CSR_TDATA3:
                    case CSR_MCONTEXT:
                    case CSR_MHARTID:
                        break;
                    default:
                    {

                        // TODO : PMP register update
                        if (addr >= PMPCONFIG0 && addr < PMPCONFIG0 + (PMPCOUNT >> 2) && priv == 3)
                        {
                            pmp.update_config(addr - PMPCONFIG0, _val);
                        }

                        else if (addr >= PMPADDR0 && addr < PMPADDR0 + PMPCOUNT && priv == 3)
                        {
                            pmp.update_addr(addr - PMPADDR0, _val);
                        }
                        else if (
                            (id_ex.csr_addr >= PMPADDR0 + (PMPCOUNT >> 2) && id_ex.csr_addr < PMPCONFIG0 + 16) ||
                            (id_ex.csr_addr >= PMPADDR0 + PMPCOUNT && id_ex.csr_addr < PMPADDR0 + 64) ||
                            (addr > CSR_MCYCLE + 2 && addr < CSR_MCYCLE + 32) ||
                            (addr > CSR_MCYCLEH + 2 && addr < CSR_MCYCLEH + 32))
                        {
                            // hard wired 0
                        }
                        else
                        {
                            csr_regs[addr].write(_val);
                        }
                    }
                    }
                }
            }
            // update time
            csr_regs[CSR_TIME].write(clint->mtime);
            csr_regs[CSR_TIMEH].write(clint->mtime >> 32);

            // update csr_mcycle and csr_mcycleh
            bool mcycle_cout = 0;
            if (CSR_MCYCLE == addr)
            {
                csr_regs[CSR_MCYCLE].write(_val);
            }
            else if (CSR_MCYCLEH == addr)
            {
                csr_regs[CSR_MCYCLEH].write(_val);
            }
            else if ((csr_regs[CSR_MCOUNTERINHIBIT].read() & 1) == 0)
            {
                if (csr_regs[CSR_MCYCLE].read() == -1)
                    mcycle_cout = 1;
                csr_regs[CSR_MCYCLE].write(csr_regs[CSR_MCYCLE].read() + 1);
                csr_regs[CSR_MCYCLEH].write(csr_regs[CSR_MCYCLEH].read() + mcycle_cout);
            }

            // update csr_minstret and csr_minstreth
            bool minstret_cout = 0;
            if (CSR_MINSTRET == addr)
            {
                csr_regs[CSR_MINSTRET].write(_val);
            }
            else if (CSR_MINSTRETH == addr)
            {
                csr_regs[CSR_MINSTRETH].write(_val);
            }
            else if ((csr_regs[CSR_MCOUNTERINHIBIT].read() & 4) == 0 && inst_ret) // inst_ret
            {
                if (csr_regs[CSR_MINSTRET] == -1)
                    minstret_cout = 1;
                csr_regs[CSR_MINSTRET].write(csr_regs[CSR_MINSTRET].read() + 1);
                csr_regs[CSR_MINSTRETH].write(csr_regs[CSR_MINSTRETH].read() + minstret_cout);
            }
        }
    }

    // Trap
    void trap()
    {

        bool is_interrupt = (_cause & 0x80000000u) != 0;
        uint32_t _cause_id = _cause & 0x3ff; // small mask for _cause id

        // Read delegation CSRs
        uint32_t medeleg = csr_regs[CSR_MEDELEG].read();
        uint32_t mideleg = csr_regs[CSR_MIDELEG].read();
        bool delegated_to_s = false;

        if (is_interrupt)
        {
            if (priv < RV_PRIV_M_MODE && (csr_regs[CSR_MSTATUS].read() & MSTATUS_SIE))
                delegated_to_s = (mideleg >> _cause_id) & 1u;
        }
        else
        {
            if (priv < RV_PRIV_M_MODE)
                delegated_to_s = (medeleg >> _cause_id) & 1u;
        }

        if (delegated_to_s)
        {
            // Save context into machine CSRs
            csr_regs[CSR_SCAUSE].write(_cause);
            csr_regs[CSR_SEPC].write(_epc & ~1);
            csr_regs[CSR_STVAL].write(_tval);
            uint32_t mstatus = csr_regs[CSR_MSTATUS].read();
            // Set SIE -> SPIE (mstatus)
            const uint32_t mstatus_sie =
                (mstatus & MSTATUS_SIE) >> MSTATUS_SIE_SHIFT;
            mstatus &= ~(MSTATUS_SPIE);
            mstatus |= (mstatus_sie << MSTATUS_SPIE_SHIFT);
            // Set supervisor interrupt enable to 0 (mstatus)
            mstatus &= ~(MSTATUS_SIE);
            // Set previous privilege
            uint32_t spp_val = (priv == RV_PRIV_S_MODE) ? 1u : 0u;
            mstatus = (mstatus & ~MSTATUS_SPP) | (spp_val << MSTATUS_SPP_SHIFT);
            csr_regs[CSR_MSTATUS].write(mstatus);
            priv = RV_PRIV_S_MODE;
        }
        else
        {
            // Save context into machine CSRs
            csr_regs[CSR_MCAUSE].write(_cause);
            csr_regs[CSR_MEPC].write(_epc & ~1);
            csr_regs[CSR_MTVAL].write(_tval);
            uint32_t mstatus = csr_regs[CSR_MSTATUS].read();
            // Set MIE -> MPIE (mstatus)
            const uint32_t mstatus_mie =
                (mstatus & MSTATUS_MIE) >> MSTATUS_MIE_SHIFT;
            mstatus &= ~(MSTATUS_MPIE);
            mstatus |= (mstatus_mie << MSTATUS_MPIE_SHIFT);
            // Set machine interrupt enable to 0 (mstatus)
            mstatus &= ~(MSTATUS_MIE);
            // Set previous privilege
            mstatus |= (priv << MSTATUS_MPP_SHIFT);
            csr_regs[CSR_MSTATUS].write(mstatus);
            priv = RV_PRIV_M_MODE;
        }
    }

    // Trap Return
    void return_from_trap(bool mmode)
    {
        if (mmode) // mret
        {
            uint32_t mstatus = csr_regs[CSR_MSTATUS].read();
            // restore privilege/ mstatus
            priv = (mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;

            // clear mpriv when mpp != M
            if (priv != RV_PRIV_M_MODE)
                mstatus &= ~MSTATUS_MPRV;

            // clear MPP
            mstatus &= ~(MSTATUS_MPP);

            // mpie -> mie, mpie = 1;
            const uint32_t mstatus_mpie =
                (mstatus & MSTATUS_MPIE) >> MSTATUS_MPIE_SHIFT;
            mstatus |= (mstatus_mpie << MSTATUS_MIE_SHIFT);
            mstatus |= MSTATUS_MPIE;
            csr_regs[CSR_MSTATUS].write(mstatus);
        }
        else // sret
        {

            uint32_t mstatus = csr_regs[CSR_MSTATUS].read();
            // restore privilege/ mstatus
            priv = (csr_regs[CSR_MSTATUS].read() & MSTATUS_SPP) >> MSTATUS_SPP_SHIFT;

            // clear mpriv when mpp != M
            if (priv != RV_PRIV_M_MODE)
                mstatus &= ~MSTATUS_MPRV;

            // clear SPP
            mstatus &= ~(MSTATUS_SPP);

            // spie -> sie, spie = 1;
            const uint32_t mstatus_spie =
                (mstatus & MSTATUS_SPIE) >> MSTATUS_SPIE_SHIFT;
            mstatus |= (mstatus_spie << MSTATUS_SIE_SHIFT);
            mstatus |= MSTATUS_SPIE;
            csr_regs[CSR_MSTATUS].write(mstatus);
        }
    }

    bool check_interrupt(uint32_t *cause)
    {
        uint32_t mip = get_mip();
        uint32_t pending_interrupt = mip & csr_regs[CSR_MIE].read();

        uint32_t s_int = (pending_interrupt & csr_regs[CSR_MIDELEG].read());
        bool s_en((priv == RV_PRIV_S_MODE && (csr_regs[CSR_MSTATUS].read() & MSTATUS_SIE)) || priv < RV_PRIV_S_MODE);

        uint32_t m_int = (pending_interrupt & ~csr_regs[CSR_MIDELEG].read());
        bool m_en(((csr_regs[CSR_MSTATUS].read() & MSTATUS_MIE) && priv == RV_PRIV_M_MODE) || priv < RV_PRIV_M_MODE);

        uint32_t final_int = 0;
        if (m_en)
            final_int |= m_int;
        if (s_en)
            final_int |= s_int;

        if (final_int) // TODO: check priority
        {
            if (final_int & MIP_MEIP)
                final_int = MIP_MEIP;
            else if (final_int & MIP_MSIP)
                final_int = MIP_MSIP;
            else if (final_int & MIP_MTIP)
                final_int = MIP_MTIP;
            else if (final_int & MIP_SEIP)
                final_int = MIP_SEIP;
            else if (final_int & MIP_SSIP)
                final_int = MIP_SSIP;
            else if (final_int & MIP_STIP)
                final_int = MIP_STIP;
            // std::cout << "interrupt !!!!  mstatus = " << csr_regs[CSR_MSTATUS].read() << "    cause     " << __builtin_ctz(final_int) << "    ";
            *cause = ((1 << 31) | __builtin_ctz(final_int));

            // std::cout << "interrupt, mip " << mip << " mie " << csr_regs[CSR_MIE].read() << "\n";
            return true;
        }
        // std::cout << "no interrupt, mip " << mip << " mie " << csr_regs[CSR_MIE].read() << "\n";
        return false;
    }
    bool pmp_dcheck(uint64_t addr, uint8_t op, uint8_t size)
    {
        if ((csr_regs[CSR_MSTATUS].read() & MSTATUS_MPRV))
            return pmp.dcheck(addr, op, size, (csr_regs[CSR_MSTATUS].read() & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT);
        else
            return pmp.dcheck(addr, op, size, priv);
    }
    bool pmp_icheck(uint64_t addr, uint8_t op, uint8_t size)
    {
        return pmp.icheck(addr, op, size, priv);
    }

    void writeback_seq()
    {
        auto id_ex = id_ex_port.read();
        if (rst.read())
        {
            wb_write_rd.write(0);
            wb_rd_val.write(0);
            wb_rd.write(0);
            wb_pc.write(0);
            wb_type.write(InstrType::NOP);
            wb_stall.write(0);
            ex_pc.write(entry);
        }
        else
        {
            wb_write_rd.write(ex_write_rd.read());
            wb_rd_val.write(ex_rd_val.read());
            wb_rd.write(ex_rd.read());
            wb_pc.write(id_ex.pc);
            wb_type.write(id_ex.type);
            wb_stall.write(stall.read());
            ex_pc.write(npc.read());
        }
    }
};
