#pragma once
#include "instr_cfg.hpp"
#include "csr_cfg.hpp"
#include "memory.hpp"
#include "regfile.hpp"
#include <iomanip>
#include <string>
#include <systemc>
using namespace sc_core;
struct ID_EX
{
    InstrType type = InstrType::NOP;
    uint32_t inst = 0;

    uint8_t rd = 0;
    bool writes_rd = 0;

    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint32_t rs1_val = 0;
    uint32_t rs2_val = 0;

    uint32_t imm = 0;
    uint32_t pc = 0;

    // CSR
    uint32_t csr_val = 0;
    bool is_csr = 0;
    uint16_t csr_addr = 0;

    bool is_compressed = 0;
    InstrType ctype = InstrType::C_NOP;
    // control flags
    bool is_branch = 0;
    bool is_jump = 0;
    bool is_load = 0;
    bool is_store = 0;
    bool is_muldiv = 0;
    bool is_system = 0;

    bool is_atomic = 0;
    uint8_t amo_type = 0;
    uint8_t mem_size = 0;

    bool flush = 0;
    bool get_next_inst = 0;
    uint32_t cause = 0;
    uint32_t tval = 0;
    bool on_trap = 0;
    bool operator==(const ID_EX &rhs)
    { // equality operator, needed for value_changed_event()
        return type == rhs.type &&
               inst == rhs.inst &&

               rd == rhs.rd &&
               writes_rd == rhs.writes_rd &&

               rs1 == rhs.rs1 &&
               rs2 == rhs.rs2 &&
               rs1_val == rhs.rs1_val &&
               rs2_val == rhs.rs2_val &&

               imm == rhs.imm &&
               pc == rhs.pc &&

               // CSR
               csr_val == rhs.csr_val &&
               is_csr == rhs.is_csr &&
               csr_addr == rhs.csr_addr &&

               // control flags
               is_branch == rhs.is_branch &&
               is_jump == rhs.is_jump &&
               is_load == rhs.is_load &&
               is_store == rhs.is_store &&
               is_muldiv == rhs.is_muldiv &&
               is_system == rhs.is_system &&

               is_atomic == rhs.is_atomic &&
               amo_type == rhs.amo_type &&
               mem_size == rhs.mem_size &&

               flush == rhs.flush &&
               get_next_inst == rhs.get_next_inst &&
               cause == rhs.cause &&
               tval == rhs.tval &&
               on_trap == rhs.on_trap;
    }
    std::string debug_str() const
    {
        std::ostringstream oss;
        if (flush)
            return "[NOP]\n";

        oss << std::hex << std::setfill('0');
        oss << "PC=" << pc << "  inst = 0x" << inst << " ";

        if (is_compressed)
            oss << std::left << to_string(ctype);
        else
            oss << std::left << to_string(type);
        switch (type)
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
            oss << " rd=x" << std::hex << unsigned(rd)
                << " rs1=x" << unsigned(rs1)
                << " imm=0x" << std::hex << imm
                << " | op1=" << rs1_val
                << " op2=" << imm;
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
            oss << " rd=x" << std::hex << unsigned(rd)
                << " rs1=x" << unsigned(rs1)
                << " rs2=x" << unsigned(rs2)
                << " | op1=" << rs1_val
                << " op2=" << rs2_val;
            break;

        // ----------------------------
        // Load / Store
        // ----------------------------
        case InstrType::LB:
        case InstrType::LH:
        case InstrType::LW:
        case InstrType::LBU:
        case InstrType::LHU:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " base=x" << unsigned(rs1)
                << " offset=0x" << std::hex << imm
                << " size=" << (int)mem_size
                << " | base_val=" << rs1_val;
            break;

        case InstrType::SB:
        case InstrType::SH:
        case InstrType::SW:
            oss << " base=x" << std::hex << unsigned(rs1)
                << " src=x" << unsigned(rs2)
                << " offset=0x" << std::hex << imm
                << " size=" << (int)mem_size
                << " | base_val=" << rs1_val
                << " store_val=" << rs2_val;
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
            oss << " rs1=x" << std::hex << unsigned(rs1)
                << " rs2=x" << unsigned(rs2)
                << " target=0x" << std::hex << (pc + imm)
                << " | op1=" << rs1_val
                << " op2=" << rs2_val;
            break;

        case InstrType::JAL:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " target=0x" << std::hex << (pc + imm);
            break;

        case InstrType::JALR:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " base=x" << unsigned(rs1)
                << " target=rs1+imm=0x" << std::hex << (rs1_val + imm);
            break;

        // ----------------------------
        // LUI / AUIPC
        // ----------------------------
        case InstrType::LUI:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " imm=0x" << std::hex << imm;
            break;

        case InstrType::AUIPC:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " imm=0x" << std::hex << imm
                << " | pc+imm=0x" << (pc + imm);
            break;

        // ----------------------------
        // CSR instructions
        // ----------------------------
        case InstrType::CSRRW:
        case InstrType::CSRRS:
        case InstrType::CSRRC:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " csr=0x" << std::hex << csr_addr
                << " rs1=x" << std::dec << unsigned(rs1)
                << " val=" << rs1_val;
            break;

        case InstrType::CSRRWI:
        case InstrType::CSRRSI:
        case InstrType::CSRRCI:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " csr=0x" << std::hex << csr_addr
                << " imm=" << std::dec << unsigned(rs1);
            break;

        // ----------------------------
        // Atomic ops
        // ----------------------------
        case InstrType::LR_W:
            oss << " rd=x" << std::hex << unsigned(rd)
                << " addr=rs1(x" << unsigned(rs1)
                << ")=" << std::hex << rs1_val;
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
            oss << " rd=x" << std::hex << unsigned(rd)
                << " size=" << (int)mem_size
                << " addr=rs1(x" << unsigned(rs1)
                << ")=" << std::hex << rs1_val
                << " val=rs2(x" << unsigned(rs2)
                << ")=" << rs2_val;
            break;

        // ----------------------------
        // System / Fence
        // ----------------------------
        case InstrType::ECALL:
        case InstrType::EBREAK:
        case InstrType::MRET:
            oss << " SYSTEM/CSR inst";
            break;

        case InstrType::FENCE:
        case InstrType::FENCE_I:
        case InstrType::SFENCE_VMA:
            oss << " FENCE instruction (memory ordering)";
            break;
        case InstrType::WFI:
            oss << "Wait for interrupt";
            break;
        case InstrType::NOP:
            oss << "[NOP]";
            break;
        default:
            oss << " [unknown or invalid]";
            break;
        }

        return oss.str() + "\n";
    }
};
void sc_trace(sc_trace_file *tf, const ID_EX &v, const std::string &NAME)
{
    // sc_trace(tf, v.dmem_resp_ack, NAME + ".dmem_resp_ack");
    // sc_trace(tf, v.dmem_rdata, NAME + ".dmem_rdata");
    // sc_trace(tf, v.dmem_fault, NAME + ".dmem_fault");
}
std::ostream &operator<<(std::ostream &os, const ID_EX &val)
{ // streaming output, needed for printing
    os << val.debug_str() << std::endl;
    return os;
}

SC_MODULE(Decode)
{
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_in<bool> stall;
    sc_in<bool> flush;
    sc_in<uint32_t> npc;
    sc_in<uint32_t> if_id_inst;
    sc_in<uint32_t> if_id_pc;
    sc_in<uint8_t> if_id_fault;
    sc_in<uint32_t> if_id_tval;
    sc_in<bool> if_id_flush;
    sc_in<bool> ex_write_rd;
    sc_in<uint32_t> ex_rd_val;
    sc_in<uint8_t> ex_rd;
    sc_in<bool> wb_write_rd;
    sc_in<uint32_t> wb_rd_val;
    sc_in<uint8_t> wb_rd;
    sc_out<ID_EX> id_ex_port;
    sc_out<uint32_t> id_ex_pc;
    sc_out<bool> id_ex_flush;

    ID_EX id_ex_tmp;
    RegFile *regs;
    SC_HAS_PROCESS(Decode);
    Decode(sc_module_name name, RegFile * r) : sc_module(name), regs(r)
    {
        SC_METHOD(inst_decode_seq);
        sensitive << clk.pos();
    }
    void inst_decode_seq();
    ID_EX decompress(uint32_t);
    ID_EX decode(uint32_t);
    void monitor()
    {
        std::cout << "ID : " << id_ex_tmp.debug_str();
    }
};
void Decode::inst_decode_seq()
{

    id_ex_tmp = ID_EX();
    if (flush.read() || rst.read() || (npc.read() != if_id_pc.read() && !stall.read()))
    {
        id_ex_flush.write(1);
        id_ex_tmp.flush = 1;
    }
    else if (!stall.read())
    {
        if (if_id_flush.read())
        {
            id_ex_flush.write(1);
            id_ex_tmp.flush = 1;
        }
        else
        {
            if ((if_id_inst.read() & 3) == 3)
                id_ex_tmp = decode(if_id_inst.read());
            else
                id_ex_tmp = decompress(if_id_inst.read() & 0x0000ffff);

            switch (if_id_fault.read())
            {
            case INSTR_ACCESS_FAULT:
            case INSTR_PAGE_FAULT:
                id_ex_tmp.tval = if_id_tval.read();
                id_ex_tmp.on_trap = true;
                id_ex_tmp.cause = if_id_fault.read();
                break;
            default:
                if (id_ex_tmp.type == InstrType::INVALID)
                {
                    id_ex_tmp.on_trap = true;
                    id_ex_tmp.cause = ILLEGAL_INSTR;
                    id_ex_tmp.tval = 0;
                }
            }
            id_ex_flush.write(0);
            id_ex_tmp.flush = 0;
        }
    }
    if (!stall.read())
    {
        id_ex_port.write(id_ex_tmp);
        id_ex_pc.write(if_id_pc.read());
    }
}
ID_EX Decode::decode(uint32_t inst)
{
    ID_EX id_ex;
    uint32_t opcode = get_bits(inst, 6, 0);
    uint32_t rd = get_bits(inst, 11, 7);
    uint32_t funct3 = get_bits(inst, 14, 12);
    uint32_t rs1 = get_bits(inst, 19, 15);
    uint32_t rs2 = get_bits(inst, 24, 20);
    uint32_t funct7 = get_bits(inst, 31, 25);
    uint32_t funct5 = funct7 >> 2;
    id_ex.inst = inst;
    id_ex.rd = rd;
    id_ex.rs1 = rs1;
    id_ex.rs2 = rs2;
    id_ex.is_compressed = 0;
    id_ex.rs1_val = (ex_write_rd.read() && id_ex.rs1 && id_ex.rs1 == ex_rd.read())   ? ex_rd_val.read()
                    : (wb_write_rd.read() && id_ex.rs1 && id_ex.rs1 == wb_rd.read()) ? wb_rd_val.read()
                                                                                     : regs->regs[id_ex.rs1];
    id_ex.rs2_val = (ex_write_rd.read() && id_ex.rs2 && id_ex.rs2 == ex_rd.read())   ? ex_rd_val.read()
                    : (wb_write_rd.read() && id_ex.rs2 && id_ex.rs2 == wb_rd.read()) ? wb_rd_val.read()
                                                                                     : regs->regs[id_ex.rs2];
    id_ex.pc = if_id_pc.read();
    switch (opcode)
    {
    // -------------------- LUI / AUIPC --------------------
    case 0b0110111: // LUI
        id_ex.type = InstrType::LUI;
        id_ex.imm = inst & 0xFFFFF000;
        id_ex.writes_rd = rd != 0;
        break;
    case 0b0010111: // AUIPC
        id_ex.type = InstrType::AUIPC;
        id_ex.imm = inst & 0xFFFFF000;
        id_ex.writes_rd = rd != 0;
        break;

    // -------------------- ALU immediate --------------------
    case 0b0010011:

        id_ex.writes_rd = rd != 0;
        id_ex.imm = sign_extend(get_bits(inst, 31, 20), 12);
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::ADDI;
            break;
        case 0b010:
            id_ex.type = InstrType::SLTI;
            break;
        case 0b011:
            id_ex.type = InstrType::SLTIU;
            break;
        case 0b100:
            id_ex.type = InstrType::XORI;
            break;
        case 0b110:
            id_ex.type = InstrType::ORI;
            break;
        case 0b111:
            id_ex.type = InstrType::ANDI;
            break;
        case 0b001:
            if (id_ex.imm > 31)
            {
                id_ex.type = InstrType::INVALID;
            }
            else
            {
                id_ex.type = InstrType::SLLI;
            }
            break;
        case 0b101:
            if (funct7 == 0b0000000)
                id_ex.type = InstrType::SRLI;
            else
                id_ex.type = InstrType::SRAI;
            break;
        }
        break;

    // -------------------- ALU (register) --------------------
    case 0b0110011:

        id_ex.writes_rd = rd != 0;
        switch ((funct7 << 3) | funct3)
        {
        case 0b0000000000:
            id_ex.type = InstrType::ADD;
            break;
        case 0b0100000000:
            id_ex.type = InstrType::SUB;
            break;
        case 0b0000000001:
            id_ex.type = InstrType::SLL;
            break;
        case 0b0000000010:
            id_ex.type = InstrType::SLT;
            break;
        case 0b0000000011:
            id_ex.type = InstrType::SLTU;
            break;
        case 0b0000000100:
            id_ex.type = InstrType::XOR;
            break;
        case 0b0000000101:
            id_ex.type = InstrType::SRL;
            break;
        case 0b0100000101:
            id_ex.type = InstrType::SRA;
            break;
        case 0b0000000110:
            id_ex.type = InstrType::OR;
            break;
        case 0b0000000111:
            id_ex.type = InstrType::AND;
            break;

        // M-extension
        case 0b0000001000:
            id_ex.type = InstrType::MUL;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001001:
            id_ex.type = InstrType::MULH;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001010:
            id_ex.type = InstrType::MULHSU;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001011:
            id_ex.type = InstrType::MULHU;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001100:
            id_ex.type = InstrType::DIV;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001101:
            id_ex.type = InstrType::DIVU;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001110:
            id_ex.type = InstrType::REM;
            id_ex.is_muldiv = true;
            break;
        case 0b0000001111:
            id_ex.type = InstrType::REMU;
            id_ex.is_muldiv = true;
            break;
        }
        break;

    // -------------------- Load --------------------
    case 0b0000011:

        id_ex.is_load = true;
        id_ex.writes_rd = rd != 0;
        id_ex.imm = sign_extend(get_bits(inst, 31, 20), 12);
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::LB;
            id_ex.mem_size = 1;
            break;
        case 0b001:
            id_ex.type = InstrType::LH;
            id_ex.mem_size = 2;
            break;
        case 0b010:
            id_ex.type = InstrType::LW;
            id_ex.mem_size = 4;
            break;
        case 0b100:
            id_ex.type = InstrType::LBU;
            id_ex.mem_size = 1;
            break;
        case 0b101:
            id_ex.type = InstrType::LHU;
            id_ex.mem_size = 2;
            break;
        }
        break;

    // -------------------- Store --------------------
    case 0b0100011:

        id_ex.is_store = true;
        id_ex.imm = sign_extend(
            (get_bits(inst, 31, 25) << 5) | get_bits(inst, 11, 7), 12);
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::SB;
            id_ex.mem_size = 1;
            break;
        case 0b001:
            id_ex.type = InstrType::SH;
            id_ex.mem_size = 2;
            break;
        case 0b010:
            id_ex.type = InstrType::SW;
            id_ex.mem_size = 4;
            break;
        }
        break;

    // -------------------- JAL / JALR --------------------
    case 0b1101111: // JAL
        id_ex.type = InstrType::JAL;
        id_ex.is_jump = true;
        id_ex.writes_rd = rd != 0;
        id_ex.imm = sign_extend(
            (get_bits(inst, 31, 31) << 20) |
                (get_bits(inst, 19, 12) << 12) |
                (get_bits(inst, 20, 20) << 11) |
                (get_bits(inst, 30, 21) << 1),
            21);
        break;

    case 0b1100111: // JALR
        id_ex.writes_rd = rd != 0;

        id_ex.type = InstrType::JALR;
        id_ex.is_jump = true;
        id_ex.imm = sign_extend(get_bits(inst, 31, 20), 12);
        break;

    // -------------------- Branch --------------------
    case 0b1100011:
        id_ex.writes_rd = false;
        id_ex.is_branch = true;

        id_ex.imm = sign_extend(
            (get_bits(inst, 31, 31) << 12) |
                (get_bits(inst, 7, 7) << 11) |
                (get_bits(inst, 30, 25) << 5) |
                (get_bits(inst, 11, 8) << 1),
            13);
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::BEQ;
            break;
        case 0b001:
            id_ex.type = InstrType::BNE;
            break;
        case 0b100:
            id_ex.type = InstrType::BLT;
            break;
        case 0b101:
            id_ex.type = InstrType::BGE;
            break;
        case 0b110:
            id_ex.type = InstrType::BLTU;
            break;
        case 0b111:
            id_ex.type = InstrType::BGEU;
            break;
        }
        break;

    // -------------------- System --------------------
    case 0b1110011:
    {
        id_ex.is_system = true;
        id_ex.csr_addr = get_bits(inst, 31, 20);

        switch (funct3)
        {
        case 0b000:
            if (funct7 == 0b0001001)
            {
                id_ex.type = InstrType::SFENCE_VMA;
            }
            else // ECALL, EBREAK, MRET
                switch (get_bits(inst, 31, 20))
                {
                case 0x000:
                    id_ex.type = InstrType::ECALL;
                    break;
                case 0x001:
                    id_ex.type = InstrType::EBREAK;
                    break;
                case 0x105:
                    id_ex.type = InstrType::WFI;
                    break; //  wait for interrupt
                case 0x102:
                    id_ex.type = InstrType::SRET;
                    break; //  supervisor return
                case 0x302:
                    id_ex.type = InstrType::MRET;
                    break; //  machine return
                default:
                    id_ex.type = InstrType::INVALID;
                    break;
                }
            break;

        case 0b001:
            id_ex.type = InstrType::CSRRW;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1_val;
            break;
        case 0b010:
            id_ex.type = InstrType::CSRRS;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1_val;
            break;
        case 0b011:
            id_ex.type = InstrType::CSRRC;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1_val;
            break;
        case 0b101:
            id_ex.type = InstrType::CSRRWI;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1 & 0x1F;
            break;
        case 0b110:
            id_ex.type = InstrType::CSRRSI;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1 & 0x1F;
            break;
        case 0b111:
            id_ex.type = InstrType::CSRRCI;
            id_ex.is_csr = true;
            id_ex.writes_rd = rd != 0;
            id_ex.csr_val = id_ex.rs1 & 0x1F;
            break;

        default:
            id_ex.type = InstrType::INVALID;
        }
        break;
    }
    // --------------------FENCE / FENCE.I --------------------
    case 0b0001111:
        id_ex.is_system = true;
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::FENCE;
            break;
        case 0b001:
            id_ex.type = InstrType::FENCE_I;
            break;
        default:
            id_ex.type = InstrType::INVALID;
            break;
        }
        break;
    // -------------------- Atomic --------------------
    case 0b0101111:
        id_ex.is_atomic = true;

        id_ex.writes_rd = rd != 0; // write rd
        id_ex.mem_size = 4;
        if (funct3 != 0b010)
        {
            id_ex.type = InstrType::INVALID;
            break;
        }

        switch (funct5)
        {
        case 0b00010:
            id_ex.type = InstrType::LR_W;
            id_ex.amo_type = 0;
            break;
        case 0b00011:
            id_ex.type = InstrType::SC_W;
            id_ex.amo_type = 1;
            break;
        case 0b00001:
            id_ex.type = InstrType::AMOSWAP_W;
            id_ex.amo_type = 2;
            break;
        case 0b00000:
            id_ex.type = InstrType::AMOADD_W;
            id_ex.amo_type = 3;
            break;
        case 0b00100:
            id_ex.type = InstrType::AMOXOR_W;
            id_ex.amo_type = 4;
            break;
        case 0b01100:
            id_ex.type = InstrType::AMOAND_W;
            id_ex.amo_type = 5;
            break;
        case 0b01000:
            id_ex.type = InstrType::AMOOR_W;
            id_ex.amo_type = 6;
            break;
        case 0b10000:
            id_ex.type = InstrType::AMOMIN_W;
            id_ex.amo_type = 7;
            break;
        case 0b10100:
            id_ex.type = InstrType::AMOMAX_W;
            id_ex.amo_type = 8;
            break;
        case 0b11000:
            id_ex.type = InstrType::AMOMINU_W;
            id_ex.amo_type = 9;
            break;
        case 0b11100:
            id_ex.type = InstrType::AMOMAXU_W;
            id_ex.amo_type = 10;
            break;
        default:
            id_ex.type = InstrType::INVALID;
            break;
        }
        break;
    default:
        id_ex.type = InstrType::INVALID;
        break;
    }

    return id_ex;
}
ID_EX Decode::decompress(uint32_t inst)
{
    int quadrant = get_bits(inst, 1, 0);
    int funct3 = get_bits(inst, 15, 13);
    uint32_t funct_11_10 = get_bits(inst, 11, 10);
    uint32_t funct_6_5 = get_bits(inst, 6, 5);
    uint32_t funct_12 = get_bits(inst, 12, 12);
    uint32_t funct_6_2 = get_bits(inst, 6, 2);
    uint32_t funct_11_7 = get_bits(inst, 11, 7);
    ID_EX id_ex;
    id_ex.inst = inst;
    id_ex.is_compressed = 1;
    id_ex.pc = if_id_pc.read();
    switch (quadrant)
    {
    case 0b00:
        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::ADDI;
            id_ex.ctype = InstrType::C_ADDI4SPN;
            id_ex.rs1 = 2;
            id_ex.imm = (get_bits(inst, 12, 11) << 4) |
                        (get_bits(inst, 10, 7) << 6) |
                        (get_bits(inst, 6, 6) << 2) |
                        (get_bits(inst, 5, 5) << 3);
            id_ex.rd = get_bits(inst, 4, 2) + 8;
            id_ex.writes_rd = 1;
            if (id_ex.imm == 0)
            {
                id_ex.on_trap = 1;
                id_ex.cause = ILLEGAL_INSTR;
                id_ex.tval = inst;
            }
            break;
        case 0b010:
            id_ex.type = InstrType::LW;
            id_ex.ctype = InstrType::C_LW;
            id_ex.rs1 = get_bits(inst, 9, 7) + 8;
            id_ex.rd = get_bits(inst, 4, 2) + 8;
            id_ex.imm = (get_bits(inst, 5, 5) << 6) |
                        (get_bits(inst, 12, 10) << 3) |
                        (get_bits(inst, 6, 6) << 2);
            id_ex.writes_rd = 1;
            id_ex.is_load = 1;
            id_ex.mem_size = 4;
            break;
        case 0b110:
            id_ex.type = InstrType::SW;
            id_ex.ctype = InstrType::C_SW;
            id_ex.rs1 = get_bits(inst, 9, 7) + 8;
            id_ex.rs2 = get_bits(inst, 4, 2) + 8;
            id_ex.imm = (get_bits(inst, 5, 5) << 6) |
                        (get_bits(inst, 12, 10) << 3) |
                        (get_bits(inst, 6, 6) << 2);
            id_ex.writes_rd = 0;
            id_ex.is_store = 1;
            id_ex.mem_size = 4;
            break;
        default:
            id_ex.type = InstrType::INVALID;
            id_ex.on_trap = 1;
            id_ex.cause = ILLEGAL_INSTR;
            id_ex.tval = inst;
        }
        break;

    case 0b01:

        switch (funct3)
        {
        case 0b000:
            if (inst == 1)
            {
                id_ex.type = InstrType::NOP;
                id_ex.ctype = InstrType::C_NOP;
                id_ex.writes_rd = 0;
            }
            else
            {
                id_ex.type = InstrType::ADDI;
                id_ex.ctype = InstrType::C_ADDI;
                id_ex.rd = funct_11_7;
                id_ex.rs1 = funct_11_7;
                id_ex.imm = sign_extend((funct_12 << 5) |
                                            get_bits(inst, 6, 2),
                                        6);

                id_ex.writes_rd = 1;
                // if (id_ex.rd == 0)
                // {
                //     id_ex.on_trap = 1;
                //     id_ex.cause = ILLEGAL_INSTR;
                //     id_ex.tval = inst;
                // }
            }
            break;
        case 0b001:
            id_ex.type = InstrType::JAL;
            id_ex.ctype = InstrType::C_JAL;
            id_ex.rd = 1;
            id_ex.writes_rd = 1;
            id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 11) |
                                        (get_bits(inst, 11, 11) << 4) |
                                        (get_bits(inst, 10, 9) << 8) |
                                        (get_bits(inst, 8, 8) << 10) |
                                        (get_bits(inst, 7, 7) << 6) |
                                        (get_bits(inst, 6, 6) << 7) |
                                        (get_bits(inst, 5, 3) << 1) |
                                        (get_bits(inst, 2, 2) << 5),
                                    12);
            break;
        case 0b010:
            id_ex.type = InstrType::ADDI;
            id_ex.ctype = InstrType::C_LI;
            id_ex.rd = funct_11_7;
            id_ex.rs1 = 0;
            id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 5) |
                                        get_bits(inst, 6, 2),
                                    6);
            id_ex.writes_rd = 1;
            // if (id_ex.rd == 0)
            // {
            //     id_ex.on_trap = 1;
            //     id_ex.cause = ILLEGAL_INSTR;
            //     id_ex.tval = inst;
            // }
            break;
        case 0b011:
            if (funct_11_7 == 2)
            {
                id_ex.type = InstrType::ADDI;
                id_ex.ctype = InstrType::C_ADDI16SP;
                id_ex.rs1 = 2;
                id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 9) |
                                            (get_bits(inst, 6, 6) << 4) |
                                            (get_bits(inst, 5, 5) << 6) |
                                            (get_bits(inst, 4, 3) << 7) |
                                            (get_bits(inst, 2, 2) << 5),
                                        10);
                id_ex.rd = 2;
                id_ex.writes_rd = 1;
                if (id_ex.imm == 0)
                {
                    id_ex.on_trap = 1;
                    id_ex.cause = ILLEGAL_INSTR;
                    id_ex.tval = inst;
                }
                break;
            }
            else
            {
                id_ex.type = InstrType::LUI;
                id_ex.ctype = InstrType::C_LUI;
                id_ex.rd = funct_11_7;
                id_ex.rs1 = 0;
                id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 17) |
                                            (get_bits(inst, 6, 2) << 12),
                                        18);

                id_ex.writes_rd = 1;
                // if (id_ex.rd == 0 || id_ex.rd == 2 || id_ex.imm == 0)
                // {
                //     id_ex.on_trap = 1;
                //     id_ex.cause = ILLEGAL_INSTR;
                //     id_ex.tval = inst;
                // }
                // break;
            }

            break;
        case 0b100:
        {
            switch (funct_11_10)
            {
            case 0b00:
                id_ex.type = InstrType::SRLI;
                id_ex.ctype = InstrType::C_SRLI;
                id_ex.rd = get_bits(inst, 9, 7) + 8;
                id_ex.rs1 = id_ex.rd;
                id_ex.imm = get_bits(inst, 6, 2);
                id_ex.writes_rd = 1;

                if (get_bits(inst, 12, 12) != 0)
                {
                    id_ex.on_trap = 1;
                    id_ex.cause = ILLEGAL_INSTR;
                    id_ex.tval = inst;
                }
                break;
            case 0b01:
                id_ex.type = InstrType::SRAI;
                id_ex.ctype = InstrType::C_SRAI;
                id_ex.rd = get_bits(inst, 9, 7) + 8;
                id_ex.rs1 = id_ex.rd;
                id_ex.imm = get_bits(inst, 6, 2);
                id_ex.writes_rd = 1;

                if (get_bits(inst, 12, 12) != 0)
                {
                    id_ex.on_trap = 1;
                    id_ex.cause = ILLEGAL_INSTR;
                    id_ex.tval = inst;
                }
                break;
            case 0b10:
                id_ex.type = InstrType::ANDI;
                id_ex.ctype = InstrType::C_ANDI;
                id_ex.rd = get_bits(inst, 9, 7) + 8;
                id_ex.rs1 = id_ex.rd;
                id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 5) |
                                            get_bits(inst, 6, 2),
                                        6);
                id_ex.writes_rd = 1;
                break;
            case 0b11:
                if ((inst & (1 << 12)) == 0)
                    switch (funct_6_5)
                    {
                    case 0b00:
                        id_ex.type = InstrType::SUB;
                        id_ex.ctype = InstrType::C_SUB;
                        id_ex.rd = get_bits(inst, 9, 7) + 8;
                        id_ex.rs1 = id_ex.rd;
                        id_ex.rs2 = get_bits(inst, 4, 2) + 8;
                        id_ex.writes_rd = 1;

                        break;
                    case 0b01:
                        id_ex.type = InstrType::XOR;
                        id_ex.ctype = InstrType::C_XOR;
                        id_ex.rd = get_bits(inst, 9, 7) + 8;
                        id_ex.rs1 = id_ex.rd;
                        id_ex.rs2 = get_bits(inst, 4, 2) + 8;
                        id_ex.writes_rd = 1;
                        break;
                    case 0b10:
                        id_ex.type = InstrType::OR;
                        id_ex.ctype = InstrType::C_OR;
                        id_ex.rd = get_bits(inst, 9, 7) + 8;
                        id_ex.rs1 = id_ex.rd;
                        id_ex.rs2 = get_bits(inst, 4, 2) + 8;
                        id_ex.writes_rd = 1;
                        break;
                    case 0b11:
                        id_ex.type = InstrType::AND;
                        id_ex.ctype = InstrType::C_AND;
                        id_ex.rd = get_bits(inst, 9, 7) + 8;
                        id_ex.rs1 = id_ex.rd;
                        id_ex.rs2 = get_bits(inst, 4, 2) + 8;
                        id_ex.writes_rd = 1;
                        break;
                    default:
                        id_ex.type = InstrType::INVALID;
                        id_ex.on_trap = 1;
                        id_ex.cause = ILLEGAL_INSTR;
                        id_ex.tval = inst;
                        break;
                    }
                else
                {
                    id_ex.type = InstrType::INVALID;
                    id_ex.on_trap = 1;
                    id_ex.cause = ILLEGAL_INSTR;
                    id_ex.tval = inst;
                }
                break;

            default:
                id_ex.type = InstrType::INVALID;
                id_ex.on_trap = 1;
                id_ex.cause = ILLEGAL_INSTR;
                id_ex.tval = inst;
                break;
            }
            break;
        }
        case 0b101:
            id_ex.type = InstrType::JAL;
            id_ex.ctype = InstrType::C_J;
            id_ex.rd = 0;
            id_ex.rs1 = 0;
            id_ex.rs2 = 0;
            id_ex.writes_rd = 0;
            id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 11) |
                                        (get_bits(inst, 11, 11) << 4) |
                                        (get_bits(inst, 10, 9) << 8) |
                                        (get_bits(inst, 8, 8) << 10) |
                                        (get_bits(inst, 7, 7) << 6) |
                                        (get_bits(inst, 6, 6) << 7) |
                                        (get_bits(inst, 5, 3) << 1) |
                                        (get_bits(inst, 2, 2) << 5),
                                    12);
            break;
        case 0b110:
            id_ex.type = InstrType::BEQ;
            id_ex.ctype = InstrType::C_BEQZ;
            id_ex.rd = 0;
            id_ex.writes_rd = 0;
            id_ex.rs1 = get_bits(inst, 9, 7) + 8;
            id_ex.rs2 = 0;
            id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 8) |
                                        (get_bits(inst, 11, 10) << 3) |
                                        (get_bits(inst, 6, 5) << 6) |
                                        (get_bits(inst, 4, 3) << 1) |
                                        (get_bits(inst, 2, 2) << 5),
                                    9);
            break;
        case 0b111:
            id_ex.type = InstrType::BNE;
            id_ex.ctype = InstrType::C_BNEZ;
            id_ex.rd = 0;
            id_ex.writes_rd = 0;
            id_ex.rs1 = get_bits(inst, 9, 7) + 8;
            id_ex.rs2 = 0;
            id_ex.imm = sign_extend((get_bits(inst, 12, 12) << 8) |
                                        (get_bits(inst, 11, 10) << 3) |
                                        (get_bits(inst, 6, 5) << 6) |
                                        (get_bits(inst, 4, 3) << 1) |
                                        (get_bits(inst, 2, 2) << 5),
                                    9);
            break;
        }
        break;

    case 0b10:

        switch (funct3)
        {
        case 0b000:
            id_ex.type = InstrType::SLLI;
            id_ex.ctype = InstrType::C_SLLI;
            id_ex.rd = funct_11_7;
            id_ex.rs1 = id_ex.rd;
            id_ex.imm = get_bits(inst, 6, 2);
            id_ex.writes_rd = 1;

            if (get_bits(inst, 12, 12) != 0 && id_ex.rd == 0)
            {
                id_ex.on_trap = 1;
                id_ex.cause = ILLEGAL_INSTR;
                id_ex.tval = inst;
            }
            break;
        case 0b001:
            id_ex.type = InstrType::INVALID; // without F ext
            id_ex.on_trap = 1;
            id_ex.cause = ILLEGAL_INSTR;
            id_ex.tval = inst;
            break;
        case 0b010:
            id_ex.type = InstrType::LW;
            id_ex.ctype = InstrType::C_LWSP;
            id_ex.rd = funct_11_7;
            id_ex.rs1 = 2;
            id_ex.imm = (get_bits(inst, 12, 12) << 5) |
                        (get_bits(inst, 6, 4) << 2) |
                        (get_bits(inst, 3, 2) << 6);
            id_ex.writes_rd = 1;
            id_ex.is_load = 1;
            id_ex.mem_size = 4;
            if (id_ex.rd == 0)
            {
                id_ex.on_trap = 1;
                id_ex.cause = ILLEGAL_INSTR;
                id_ex.tval = inst;
            }
            break;
        case 0b011:
            id_ex.type = InstrType::INVALID; // without F ext
            id_ex.on_trap = 1;
            id_ex.cause = ILLEGAL_INSTR;
            id_ex.tval = inst;
            break;

        case 0b100:
        {
            if (funct_12 == 0)
            {
                if (funct_6_2 == 0)
                {
                    id_ex.type = InstrType::JALR;
                    id_ex.ctype = InstrType::C_JR;
                    id_ex.rd = 0;
                    id_ex.rs1 = funct_11_7;
                    id_ex.imm = 0;
                    id_ex.writes_rd = 0;
                    if (id_ex.rs1 == 0)
                    {
                        id_ex.on_trap = 1;
                        id_ex.cause = ILLEGAL_INSTR;
                        id_ex.tval = inst;
                    }
                }
                else
                {
                    id_ex.type = InstrType::ADD;
                    id_ex.ctype = InstrType::C_MV;
                    id_ex.rd = funct_11_7;
                    id_ex.rs1 = 0;
                    id_ex.rs2 = funct_6_2;
                    id_ex.writes_rd = 1;
                    // if (id_ex.rd == 0)
                    // {
                    //     id_ex.on_trap = 1;
                    //     id_ex.cause = ILLEGAL_INSTR;
                    //     id_ex.tval = inst;
                    // }
                }
            }
            else
            {
                if (funct_6_2 == 0)
                {
                    if (funct_11_7 == 0)
                    {
                        id_ex.type = InstrType::EBREAK;
                        id_ex.ctype = InstrType::C_EBREAK;
                    }
                    else
                    {
                        id_ex.type = InstrType::JALR;
                        id_ex.ctype = InstrType::C_JALR;
                        id_ex.rd = 1;
                        id_ex.rs1 = funct_11_7;
                        id_ex.imm = 0;
                        id_ex.writes_rd = 1;
                        if (id_ex.rs1 == 0)
                        {
                            id_ex.on_trap = 1;
                            id_ex.cause = ILLEGAL_INSTR;
                            id_ex.tval = inst;
                        }
                    }
                }
                else
                {
                    id_ex.type = InstrType::ADD;
                    id_ex.ctype = InstrType::C_ADD;
                    id_ex.rd = funct_11_7;
                    id_ex.rs1 = funct_11_7;
                    id_ex.rs2 = funct_6_2;
                    id_ex.writes_rd = 1;
                    // if (id_ex.rd == 0)
                    // {
                    //     id_ex.on_trap = 1;
                    //     id_ex.cause = ILLEGAL_INSTR;
                    //     id_ex.tval = inst;
                    // }
                }
            }
            break;
        }
        case 0b101:
            id_ex.type = InstrType::INVALID; // without F ext
            break;
        case 0b110:
            id_ex.type = InstrType::SW;
            id_ex.ctype = InstrType::C_SWSP;
            id_ex.imm = (get_bits(inst, 12, 9) << 2) |
                        (get_bits(inst, 8, 7) << 6);
            id_ex.rs1 = 2;
            id_ex.rs2 = funct_6_2;
            id_ex.writes_rd = 0;
            id_ex.is_store = 1;
            id_ex.mem_size = 4;
            break;
        case 0b111:
            id_ex.type = InstrType::INVALID; // without F ext
            break;
        }
        break;

    default:
        id_ex.type = InstrType::INVALID;
        break;
    }

    id_ex.rs1_val = (ex_write_rd.read() && id_ex.rs1 && id_ex.rs1 == ex_rd.read())   ? ex_rd_val.read()
                    : (wb_write_rd.read() && id_ex.rs1 && id_ex.rs1 == wb_rd.read()) ? wb_rd_val.read()
                                                                                     : regs->regs[id_ex.rs1];
    id_ex.rs2_val = (ex_write_rd.read() && id_ex.rs2 && id_ex.rs2 == ex_rd.read())   ? ex_rd_val.read()
                    : (wb_write_rd.read() && id_ex.rs2 && id_ex.rs2 == wb_rd.read()) ? wb_rd_val.read()
                                                                                     : regs->regs[id_ex.rs2];

    return id_ex;
}