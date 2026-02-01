#pragma once
#include <string>
enum class InstrType
{
    // Base RV32I
    LUI,
    AUIPC,
    ADDI,
    SLTI,
    SLTIU,
    XORI,
    ORI,
    ANDI,
    SLLI,
    SRLI,
    SRAI,
    ADD,
    SUB,
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND,
    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW,
    JAL,
    JALR,
    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,
    // CSR
    ECALL,
    EBREAK,
    SRET,
    MRET,
    WFI,
    CSRRW,
    CSRRS,
    CSRRC,
    CSRRWI,
    CSRRSI,
    CSRRCI,

    FENCE,
    FENCE_I,
    SFENCE_VMA,

    // M-extension
    MUL,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVU,
    REM,
    REMU,

    // A-extension
    LR_W,
    SC_W,
    AMOSWAP_W,
    AMOADD_W,
    AMOXOR_W,
    AMOAND_W,
    AMOOR_W,
    AMOMIN_W,
    AMOMAX_W,
    AMOMINU_W,
    AMOMAXU_W,

    //  C-extension (optional)
    /* ========================= */
    /* RV32C – Quadrant 0 (00)   */
    /* ========================= */
    C_ADDI4SPN, // must-have
    C_LW,       // must-have
    C_SW,       // must-have

    /* ========================= */
    /* RV32C – Quadrant 1 (01)   */
    /* ========================= */
    C_NOP,
    C_ADDI, // must-have
    C_JAL,  // rv32 only
    C_LI,
    C_ADDI16SP, // Linux-critical
    C_LUI,
    C_SRLI,
    C_SRAI,
    C_ANDI,
    C_SUB,
    C_XOR,
    C_OR,
    C_AND,
    C_J,    // must-have
    C_BEQZ, // must-have
    C_BNEZ, // must-have

    /* ========================= */
    /* RV32C – Quadrant 2 (10)   */
    /* ========================= */
    C_SLLI,
    C_LWSP, // Linux-critical
    C_JR,   // must-have
    C_MV,
    C_EBREAK,
    C_JALR, // must-have
    C_ADD,
    C_SWSP, // Linux-critical

    NOP,
    INVALID
};
std::string to_string(InstrType type)
{
    switch (type)
    {
        // Base RV32I
    case InstrType::LUI:
        return "LUI";
    case InstrType::AUIPC:
        return "AUIPC";
    case InstrType::ADDI:
        return "ADDI";
    case InstrType::SLTI:
        return "SLTI";
    case InstrType::SLTIU:
        return "SLTIU";
    case InstrType::XORI:
        return "XORI";
    case InstrType::ORI:
        return "ORI";
    case InstrType::ANDI:
        return "ANDI";
    case InstrType::SLLI:
        return "SLLI";
    case InstrType::SRLI:
        return "SRLI";
    case InstrType::SRAI:
        return "SRAI";
    case InstrType::ADD:
        return "ADD";
    case InstrType::SUB:
        return "SUB";
    case InstrType::SLL:
        return "SLL";
    case InstrType::SLT:
        return "SLT";
    case InstrType::SLTU:
        return "SLTU";
    case InstrType::XOR:
        return "XOR";
    case InstrType::SRL:
        return "SRL";
    case InstrType::SRA:
        return "SRA";
    case InstrType::OR:
        return "OR";
    case InstrType::AND:
        return "AND";
    case InstrType::LB:
        return "LB";
    case InstrType::LH:
        return "LH";
    case InstrType::LW:
        return "LW";
    case InstrType::LBU:
        return "LBU";
    case InstrType::LHU:
        return "LHU";
    case InstrType::SB:
        return "SB";
    case InstrType::SH:
        return "SH";
    case InstrType::SW:
        return "SW";
    case InstrType::JAL:
        return "JAL";
    case InstrType::JALR:
        return "JALR";
    case InstrType::BEQ:
        return "BEQ";
    case InstrType::BNE:
        return "BNE";
    case InstrType::BLT:
        return "BLT";
    case InstrType::BGE:
        return "BGE";
    case InstrType::BLTU:
        return "BLTU";
    case InstrType::BGEU:
        return "BGEU";
    case InstrType::ECALL:
        return "ECALL";
    case InstrType::EBREAK:
        return "EBREAK";
    case InstrType::SRET:
        return "SRET";
    case InstrType::MRET:
        return "MRET";
    case InstrType::WFI:
        return "WFI";
    case InstrType::CSRRW:
        return "CSRRW";
    case InstrType::CSRRS:
        return "CSRRS";
    case InstrType::CSRRC:
        return "CSRRC";
    case InstrType::CSRRWI:
        return "CSRRWI";
    case InstrType::CSRRSI:
        return "CSRRSI";
    case InstrType::CSRRCI:
        return "CSRRCI";
    case InstrType::FENCE:
        return "FENCE";
    case InstrType::FENCE_I:
        return "FENCE_I";
    case InstrType::SFENCE_VMA:
        return "SFENCE_VMA";
    case InstrType::MUL:
        return "MUL";
    case InstrType::MULH:
        return "MULH";
    case InstrType::MULHSU:
        return "MULHSU";
    case InstrType::MULHU:
        return "MULHU";
    case InstrType::DIV:
        return "DIV";
    case InstrType::DIVU:
        return "DIVU";
    case InstrType::REM:
        return "REM";
    case InstrType::REMU:
        return "REMU";
    case InstrType::LR_W:
        return "LR_W";
    case InstrType::SC_W:
        return "SC_W";
    case InstrType::AMOSWAP_W:
        return "AMOSWAP_W";
    case InstrType::AMOADD_W:
        return "AMOADD_W";
    case InstrType::AMOXOR_W:
        return "AMOXOR_W";
    case InstrType::AMOAND_W:
        return "AMOAND_W";
    case InstrType::AMOOR_W:
        return "AMOOR_W";
    case InstrType::AMOMIN_W:
        return "AMOMIN_W";
    case InstrType::AMOMAX_W:
        return "AMOMAX_W";
    case InstrType::AMOMINU_W:
        return "AMOMINU_W";
    case InstrType::AMOMAXU_W:
        return "AMOMAXU_W";
    case InstrType::NOP:
        return "NOP";
    case InstrType::C_ADDI4SPN:
        return "C_ADDI4SPN"; // must-have
    case InstrType::C_LW:
        return "C_LW"; // must-have
    case InstrType::C_SW:
        return "C_SW"; // must-have
    case InstrType::C_NOP:
        return "C_NOP";
    case InstrType::C_ADDI:
        return "C_ADDI"; // must-have
    case InstrType::C_JAL:
        return "C_JAL"; // rv32 only
    case InstrType::C_LI:
        return "C_LI";
    case InstrType::C_ADDI16SP:
        return "C_ADDI16SP"; // Linux-critical
    case InstrType::C_LUI:
        return "C_LUI";
    case InstrType::C_SRLI:
        return "C_SRLI";
    case InstrType::C_SRAI:
        return "C_SRAI";
    case InstrType::C_ANDI:
        return "C_ANDI";
    case InstrType::C_SUB:
        return "C_SUB";
    case InstrType::C_XOR:
        return "C_XOR";
    case InstrType::C_OR:
        return "C_OR";
    case InstrType::C_AND:
        return "C_AND";
    case InstrType::C_J:
        return "C_J"; // must-have
    case InstrType::C_BEQZ:
        return "C_BEQZ"; // must-have
    case InstrType::C_BNEZ:
        return "C_BNEZ"; // must-have
    case InstrType::C_SLLI:
        return "C_SLLI";
    case InstrType::C_LWSP:
        return "C_LWSP"; // Linux-critical
    case InstrType::C_JR:
        return "C_JR"; // must-have
    case InstrType::C_MV:
        return "C_MV";
    case InstrType::C_EBREAK:
        return "C_EBREAK";
    case InstrType::C_JALR:
        return "C_JALR"; // must-have
    case InstrType::C_ADD:
        return "C_ADD";
    case InstrType::C_SWSP:
        return "C_SWSP"; // Linux-critical
    case InstrType::INVALID:
        return "INVALID";
    default:
        return "INVALID";
    }
}
void sc_trace(sc_trace_file *tf, const InstrType &v, const std::string &NAME)
{
}
std::ostream &operator<<(std::ostream &os, const InstrType &val)
{ // streaming output, needed for printing
    os << to_string(val);
    return os;
}
