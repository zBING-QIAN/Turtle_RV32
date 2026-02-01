#pragma once
#include <cstdint>
#include <unordered_map>
#include <stdexcept>
// Privilage Mode
#define RV_PRIV_U_MODE 0
#define RV_PRIV_S_MODE 1
#define RV_PRIV_M_MODE 3
// Mstatus Helper
#define MSTATUS_SIE_SHIFT 1
#define MSTATUS_MIE_SHIFT 3
#define MSTATUS_SPIE_SHIFT 5
#define MSTATUS_UBE_SHIFT 6
#define MSTATUS_MPIE_SHIFT 7
#define MSTATUS_SPP_SHIFT 8
#define MSTATUS_MPP_SHIFT 11
#define MSTATUS_MPRV_SHIFT 17
#define MSTATUS_SUM_SHIFT 18
#define MSTATUS_MXR_SHIFT 19
#define MSTATUS_TVM_SHIFT 20
#define MSTATUS_TW_SHIFT 21
#define MSTATUS_TSR_SHIFT 22
#define MSTATUS_SIE (1 << MSTATUS_SIE_SHIFT)
#define MSTATUS_MIE (1 << MSTATUS_MIE_SHIFT)
#define MSTATUS_SPIE (1 << MSTATUS_SPIE_SHIFT)
#define MSTATUS_UBE (1 << MSTATUS_UBE_SHIFT)
#define MSTATUS_MPIE (1 << MSTATUS_MPIE_SHIFT)
#define MSTATUS_SPP (1 << MSTATUS_SPP_SHIFT)
#define MSTATUS_MPP (3 << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPRV (1 << MSTATUS_MPRV_SHIFT)
#define MSTATUS_SUM (1 << MSTATUS_SUM_SHIFT)
#define MSTATUS_MXR (1 << MSTATUS_MXR_SHIFT)
#define MSTATUS_TVM (1 << MSTATUS_TVM_SHIFT)
#define MSTATUS_TW (1 << MSTATUS_TW_SHIFT)
#define MSTATUS_TSR (1 << MSTATUS_TSR_SHIFT)

#define MSTATUS_FS (3 << 13)
// VS (Vector Status): bits 9-10
#define MSTATUS_VS (3 << 9)
// XS (Additional Extensions Status): bits 15-16
#define MSTATUS_XS (3 << 15)
// SD (Status Dirty) bit: Bit 31 (RV32) or Bit 63 (RV64)
#define MSTATUS_SD (1 << 31) // For RV32
// Combined mask for all extension status bits
#define MSTATUS_MASK ~(MSTATUS_FS | MSTATUS_VS | MSTATUS_XS | MSTATUS_SD)
// Sstatus Helper
#define SSTATUS_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP | MSTATUS_SUM | MSTATUS_MXR)

// MIE Helper
#define MIE_SSIE (1 << 1)
#define MIE_MSIE (1 << 3)
#define MIE_STIE (1 << 5)
#define MIE_MTIE (1 << 7)
#define MIE_SEIE (1 << 9)
#define MIE_MEIE (1 << 11)
// read/write mask
#define MIE_MASK (MIE_SSIE | MIE_MSIE | MIE_STIE | MIE_MTIE | MIE_SEIE | MIE_MEIE)
// MIP Helper
#define MIP_SSIP (1 << 1)
#define MIP_MSIP (1 << 3)
#define MIP_STIP (1 << 5)
#define MIP_MTIP (1 << 7)
#define MIP_SEIP (1 << 9)
#define MIP_MEIP (1 << 11)
// write mask
#define MIP_WMASK (MIP_SSIP | MIP_SEIP) // with Sstc ext, this bit is read only in m-mode
// #define MIP_MASK (MIP_SSIP | MIP_STIP | MIP_SEIP)
#define SIP_WMASK MIP_SSIP
// read mask
#define MIP_RMASK MIE_MASK

// MEDELEG
#define MEDELEG_MASK 0xB7FF // ECALL_M can not delegate to S-mode

// Exception
enum ExceptionCause : uint32_t
{
    INSTR_ADDR_MISALIGNED = 0,
    INSTR_ACCESS_FAULT = 1,
    ILLEGAL_INSTR = 2,
    BREAKPOINT = 3,
    LOAD_ADDR_MISALIGNED = 4,
    LOAD_ACCESS_FAULT = 5,
    STORE_ADDR_MISALIGNED = 6,
    STORE_ACCESS_FAULT = 7,
    ECALL_U = 8,
    ECALL_S = 9,
    ECALL_M = 11,
    INSTR_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_PAGE_FAULT = 15,
};

// Common CSR addresses
enum CSRAddr : uint16_t
{
    // Machine
    CSR_MSTATUS = 0x300,
    CSR_MISA = 0x301,
    CSR_MIE = 0x304,
    CSR_MTVEC = 0x305,
    CSR_MCOUNTEREN = 0x306,
    CSR_MSCRATCH = 0x340,
    CSR_MEPC = 0x341,
    CSR_MCAUSE = 0x342,
    CSR_MTVAL = 0x343,
    CSR_MIP = 0x344,
    CSR_MEDELEG = 0x302,
    CSR_MIDELEG = 0x303,
    CSR_MHARTID = 0xF14,

    CSR_TSELECT = 0x7A0,
    CSR_TDATA1 = 0x7A1,
    CSR_TDATA2 = 0x7A2,
    CSR_TDATA3 = 0x7A3,
    CSR_MCONTEXT = 0x7A8,
    // Machine performance-monitor
    CSR_MCYCLE = 0xB00,
    CSR_MINSTRET = 0xB02,
    CSR_MHPMCOUNTER3 = 0xB03,
    CSR_MCYCLEH = 0xB80,
    CSR_MINSTRETH = 0xB82,
    CSR_MHPMCOUNTER3H = 0xB83,
    CSR_MCOUNTERINHIBIT = 0x320,
    // Pmpconfig
    PMPCONFIG0 = 0x3A0,
    PMPADDR0 = 0x3B0,

    // Supervisor
    CSR_SSTATUS = 0x100,
    CSR_SIE = 0x104,
    CSR_STVEC = 0x105,
    CSR_SSCRATCH = 0x140,
    CSR_SEPC = 0x141,
    CSR_SCAUSE = 0x142,
    CSR_STVAL = 0x143,
    CSR_SIP = 0x144,
    CSR_SATP = 0x180,
    CSR_STIMECMP = 0x14D,
    CSR_STIMECMPH = 0x15D,

    CSR_TIME = 0xC01,
    CSR_TIMEH = 0xC81,
    CSR_CYCLE = 0xC00,
    CSR_CYCLEH = 0xC80,
};

inline uint32_t get_bits(uint32_t inst, int hi, int lo)
{
    return (inst >> lo) & ((1u << (hi - lo + 1)) - 1);
}

// Sign-extend a value with n bits
inline int32_t sign_extend(uint32_t val, int bits)
{
    int32_t mask = 1 << (bits - 1);
    return (val ^ mask) - mask;
}