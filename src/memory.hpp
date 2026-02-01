#pragma once
#include <vector>
#include <unordered_map>
#include <systemc>
#include <algorithm>
#include <cstdint>
// #include "core/csr.hpp"
using namespace sc_core;
#define PLIC_BASE 0x30000000
#define PLIC_TOP 0x34000000

#define UART_BASE 0x10000000
#define UART_TOP 0x10000100

#define CLINT_BASE 0x20000000
#define CLINT_TOP 0x20010000
struct IMEM_IN
{
    bool imem_req_ack = false;
    uint32_t imem_addr = 0;

    bool operator==(const IMEM_IN &rhs)
    { // equality operator, needed for value_changed_event()
        return imem_req_ack == rhs.imem_req_ack &&
               imem_addr == rhs.imem_addr;
    }
};
void sc_trace(sc_trace_file *tf, const IMEM_IN &v, const std::string &NAME)
{
    sc_trace(tf, v.imem_req_ack, NAME + ".imem_req_ack");
    sc_trace(tf, v.imem_addr, NAME + ".imem_addr");
}
std::ostream &operator<<(std::ostream &os, const IMEM_IN &val)
{ // streaming output, needed for printing
    os << "ack " << val.imem_req_ack << "; iaddr = " << val.imem_addr;
    return os;
}
struct DMEM_IN
{
    bool dmem_req_ack = false;
    uint32_t dmem_addr = 0;
    uint32_t dmem_wdata = 0;
    uint8_t dmem_size = 4;
    bool dmem_is_load = false;
    bool dmem_is_store = false;
    bool dmem_is_amo = false;
    uint8_t dmem_amoop = 0;

    bool operator==(const DMEM_IN &rhs)
    { // equality operator, needed for value_changed_event()
        return dmem_req_ack == rhs.dmem_req_ack &&
               dmem_addr == rhs.dmem_addr &&
               dmem_wdata == rhs.dmem_wdata &&
               dmem_size == rhs.dmem_size &&
               dmem_is_load == rhs.dmem_is_load &&
               dmem_is_store == rhs.dmem_is_store &&
               dmem_is_amo == rhs.dmem_is_amo &&
               dmem_amoop == rhs.dmem_amoop;
    }
};
void sc_trace(sc_trace_file *tf, const DMEM_IN &v, const std::string &NAME)
{
    sc_trace(tf, v.dmem_req_ack, NAME + ".dmem_req_ack");
    sc_trace(tf, v.dmem_addr, NAME + ".dmem_addr");
    sc_trace(tf, v.dmem_wdata, NAME + ".dmem_wdata");
    sc_trace(tf, v.dmem_size, NAME + ".dmem_size");
    sc_trace(tf, v.dmem_is_load, NAME + ".dmem_is_load");
    sc_trace(tf, v.dmem_is_store, NAME + ".dmem_is_store");
    sc_trace(tf, v.dmem_is_amo, NAME + ".dmem_is_amo");
    sc_trace(tf, v.dmem_amoop, NAME + ".dmem_amoop");
}
std::ostream &operator<<(std::ostream &os, const DMEM_IN &val)
{ // streaming output, needed for printing
    os << "ack " << val.dmem_req_ack << "; daddr = " << val.dmem_addr << "; dmem_wdata = " << val.dmem_wdata << "; is_load = "
       << val.dmem_is_load << "; is_store = " << val.dmem_is_store;
    return os;
}
struct MEM_OUT
{
    bool mem_rsp_ack = false;
    uint32_t mem_rsp_rdata = 0;
    uint8_t mem_rsp_fault = false;
    bool operator==(const MEM_OUT &rhs)
    { // equality operator, needed for value_changed_event()
        return mem_rsp_ack == rhs.mem_rsp_ack &&
               mem_rsp_rdata == rhs.mem_rsp_rdata &&
               mem_rsp_fault == rhs.mem_rsp_fault;
    }
};
void sc_trace(sc_trace_file *tf, const MEM_OUT &v, const std::string &NAME)
{
    sc_trace(tf, v.mem_rsp_ack, NAME + ".mem_rsp_ack");
    sc_trace(tf, v.mem_rsp_rdata, NAME + ".mem_rsp_rdata");
    sc_trace(tf, v.mem_rsp_fault, NAME + ".mem_rsp_fault");
}
std::ostream &operator<<(std::ostream &os, const MEM_OUT &val)
{ // streaming output, needed for printing
    os << "ack " << val.mem_rsp_ack << "; rdata = " << val.mem_rsp_rdata << "; mem_rsp_fault= " << (uint32_t)val.mem_rsp_fault;
    return os;
}

struct MMU_RSP
{
    bool mmu_rsp_ack = false;
    uint32_t mmu_rsp_rdata = 0;
    uint32_t mmu_rsp_tval = 0;
    uint8_t mmu_rsp_fault = false;
    bool operator==(const MMU_RSP &rhs)
    { // equality operator, needed for value_changed_event()
        return mmu_rsp_ack == rhs.mmu_rsp_ack &&
               mmu_rsp_rdata == rhs.mmu_rsp_rdata &&
               mmu_rsp_tval == rhs.mmu_rsp_tval &&
               mmu_rsp_fault == rhs.mmu_rsp_fault;
    }
};
void sc_trace(sc_trace_file *tf, const MMU_RSP &v, const std::string &NAME)
{
    sc_trace(tf, v.mmu_rsp_ack, NAME + ".mmu_rsp_ack");
    sc_trace(tf, v.mmu_rsp_rdata, NAME + ".mmu_rsp_rdata");
    sc_trace(tf, v.mmu_rsp_tval, NAME + ".mmu_rsp_tval");
    sc_trace(tf, v.mmu_rsp_fault, NAME + ".mmu_rsp_fault");
}
std::ostream &operator<<(std::ostream &os, const MMU_RSP &val)
{ // streaming output, needed for printing
    os << "ack " << val.mmu_rsp_ack << "; rdata = " << val.mmu_rsp_rdata << "; mmu_rsp_fault= " << (uint32_t)val.mmu_rsp_fault;
    return os;
}
uint32_t amo_op(uint8_t amoop, uint32_t a, uint32_t b)
{
    switch (amoop)
    {
    case 2:
        return a;
    case 3:
        return a + b;
    case 4:
        return a ^ b;
    case 5:
        return a & b;
    case 6:
        return a | b;
    case 7:
        return std::min((int32_t)a, (int32_t)b);
    case 8:
        return std::max((int32_t)a, (int32_t)b);
    case 9:
        return std::min(a, b);
    case 10:
        return std::max(a, b);
    default:
        break;
    }
    return 0;
}
struct Device
{
    uint32_t base, top;
    virtual uint8_t read(uint32_t addr) { return 0; }
    virtual void write(uint32_t addr, uint8_t val) {}
};

struct UART_DEV : Device
{
    uint32_t iir, ier, lcr;
    uint32_t UART_BUF_SIZE = 128;
    uint32_t uart_buf_begin = 0, uart_buf_end = 0;
    char uart_buf[128];
    bool thri = 0;
    uint32_t dlm, dll;
    UART_DEV()
    {
        thri = 0;
        dlm = 0, dll = 0;
        ier = 0;
        iir = 1;
        lcr = 0;
        base = UART_BASE;
        top = UART_TOP;
    }
    void keyboard_input(const char *s, int len)
    {
        // update uart_buf and begin/end
        for (int i = 0; i < len; i++)
        {
            int nxt_end = (uart_buf_end + 1 != UART_BUF_SIZE) ? uart_buf_end + 1 : 0;
            if (nxt_end == uart_buf_begin)
                break;
            uart_buf[uart_buf_end] = s[i];
            uart_buf_end = nxt_end;
        }
        std::cerr << "[DEBUG] UART buffer : " << uart_buf << ", input string : " << s << "\n";
        std::cerr << "[DEBUG] UART buffer_begin : " << uart_buf_begin << ", UART buffer_end : " << uart_buf_end << "\n";
        std::cerr << "[DEBUG] UART IIR : " << iir << ", UART IER: " << ier << ", UART LCR: " << lcr << "\n";
    }
    bool uart_irq()
    {
        if ((uart_buf_end != uart_buf_begin) && (ier & 0x01))
        {
            iir = 0x04;
            return 1;
        }

        if ((ier & 0x02) && thri) //((lsr & 0x20) && (ier & 0x02))
        {
            iir = 0x02;
            return 1;
        }
        iir = 1;
        return 0;
    }
    uint8_t read(uint32_t addr) override
    {
        uint32_t offset = addr - UART_BASE;
        bool dlab = (lcr >> 7) & 1; // You need to store 'lcr' in your struct

        switch (offset)
        {
        case 0: // RBR or DLL
            if (dlab)
                return dll;

            // RBR: Read from buffer
            if (uart_buf_end != uart_buf_begin)
            {
                uint8_t val = uart_buf[uart_buf_begin++];
                if (uart_buf_begin >= UART_BUF_SIZE)
                    uart_buf_begin = 0;

                // CRITICAL: After reading a byte, the "Data Ready" interrupt
                // condition might be gone. Update status!
                uart_irq();
                return val;
            }
            return 0;

        case 1: // IER or DLM
            if (dlab)
                return dlm;
            return ier;

        case 2:         // IIR
            uart_irq(); // Update IIR based on current state

            thri = 0;
            // std::cerr << "[DEBUG] UART IIR : " << iir << ", UART IER: " << ier << ", UART LCR: " << lcr << "\n";
            return iir;

        case 3: // LCR
            // std::cerr << "[DEBUG] UART IIR : " << iir << ", UART IER: " << ier << ", UART LCR: " << lcr << "\n";
            return lcr;

        case 5: // LSR
            // Bit 0: Data Ready
            // Bit 5: THR Empty
            // Bit 6: Transmitter Empty
            return 0x60 | (uart_buf_end != uart_buf_begin);
        }

        return 0;
    }
    void write(uint32_t addr, uint8_t val) override
    {
        uint32_t offset = addr - UART_BASE;
        bool dlab = (lcr >> 7) & 1;

        switch (offset)
        {
        case 0:
            if (dlab)
                dll = val;
            else
            {
                thri = 1;
                uart_irq();
                std::cerr << val;
            }
            break;
        case 1:
            if (dlab)
                dlm = val;
            else
            {
                ier = val;
                uart_irq();
                // std::cerr << "[DEBUG] UART IIR : " << iir << ", UART IER: " << ier << ", UART LCR: " << lcr << "\n";
            } // Only update IER if DLAB is 0!
            break;
        case 3:
            lcr = val; // This is where DLAB lives
            break;
            // ... other cases
        }
    }
};

struct PLIC_DEV : Device // UART only
{
    // context of hart 0 m/s mode
    uint32_t CNTXT_0_CLM_CMPLT_THRESHOLD_ADDR = PLIC_BASE + 0x200000;
    uint32_t CNTXT_1_CLM_CMPLT_THRESHOLD_ADDR = PLIC_BASE + 0x201000;
    uint32_t CNTXT_0_CLM_CMPLT_ADDR = PLIC_BASE + 0x200004;
    uint32_t CNTXT_1_CLM_CMPLT_ADDR = PLIC_BASE + 0x201004;

    // uart is 10th interrupt source
    uint32_t UART_PRIORITY_ADDR = PLIC_BASE + 0x28;
    uint32_t SOURCE_PENDING_ADDR = PLIC_BASE + 0x1000;
    // cntxt 0/1
    uint32_t CNTXT_0_ENABLE_ADDR = PLIC_BASE + 0x002000;
    uint32_t CNTXT_1_ENABLE_ADDR = PLIC_BASE + 0x002080;

    uint32_t uart_priority = 0;
    uint32_t source_pending = 0; // source 0~31
    uint32_t core0_m_threshold = 0;
    uint32_t core0_s_threshold = 0;
    uint32_t core0_m_clm = 0;
    uint32_t core0_s_clm = 0;
    uint32_t core0_m_irq = 0;
    uint32_t core0_s_irq = 0;
    uint32_t core0_m_cmplt = 0;
    uint32_t core0_s_cmplt = 0;
    uint32_t core0_m_enable = 0;
    uint32_t core0_s_enable = 0;

    uint32_t claim_mask = 0;
    UART_DEV *uart;
    PLIC_DEV()
    {

        base = PLIC_BASE;
        top = PLIC_TOP;
    }
    void update()
    {
        // Simple logic: if any pending IRQ is enabled and has priority > 0
        if (core0_m_cmplt == core0_m_clm)
        {
            // notify device the interrupt is done
            core0_m_clm = 0;
            core0_m_irq = 0;
        }
        if (core0_s_cmplt == core0_s_clm)
        {
            // notify device the interrupt is done
            core0_s_clm = 0;
            core0_s_irq = 0;
        }

        source_pending = (uint32_t(uart->uart_irq()) << 10) & (~claim_mask);
        // if (source_pending != 0)
        //     std::cerr << "plic pending " << source_pending << "plic claim_mask " << claim_mask << " core0_m_enable " << core0_m_enable << " core0_s_enable " << core0_s_enable << " uart_priority " << uart_priority << " core0_m_threshold " << core0_m_threshold << " core0_s_threshold " << core0_s_threshold << "\n";
        if ((source_pending & (1 << 10)) && (core0_m_enable & (1 << 10)) && uart_priority > core0_m_threshold)
        {
            core0_m_irq = 10;
            // std::cerr << "uart m irq pending\n";
        }
        else
            core0_m_irq = core0_m_clm;
        if ((source_pending & (1 << 10)) && (core0_s_enable & (1 << 10)) && uart_priority > core0_s_threshold)
        {
            core0_s_irq = 10;
        }
        else
            core0_s_irq = core0_s_clm;

        core0_s_cmplt = core0_m_cmplt = 0;
    }

    uint8_t read(uint32_t addr)
    {
        uint32_t off = (addr & 3) * 8;
        addr &= ~3;
        uint32_t mask = 0xff << off;
        // std::cerr << "Access PLIC : " << addr + (off >> 3) << "\n";
        if (addr == CNTXT_0_CLM_CMPLT_THRESHOLD_ADDR)
        {
            return (core0_m_threshold >> off);
        }
        else if (addr == CNTXT_1_CLM_CMPLT_THRESHOLD_ADDR)
        {
            return (core0_s_threshold >> off);
        }
        else if (addr == CNTXT_0_CLM_CMPLT_ADDR)
        {
            // std::cerr << "[DEBUG] PLIC m claim " << core0_m_clm << "\n";
            claim_mask |= (1 << core0_m_clm);
            core0_m_clm = core0_m_irq;
            // std::cerr << "plic pending " << source_pending << "plic claim_mask " << claim_mask << " core0_m_enable " << core0_m_enable << " core0_s_enable " << core0_s_enable << " uart_priority " << uart_priority << " core0_m_threshold " << core0_m_threshold << " core0_s_threshold " << core0_s_threshold << "\n";
            return (core0_m_clm >> off);
        }
        else if (addr == CNTXT_1_CLM_CMPLT_ADDR)
        {
            // std::cerr << "[DEBUG] PLIC s claim " << core0_s_clm << "\n";
            claim_mask |= (1 << core0_s_clm);
            core0_s_clm = core0_s_irq;
            // std::cerr << "plic pending " << source_pending << "plic claim_mask " << claim_mask << " core0_m_enable " << core0_m_enable << " core0_s_enable " << core0_s_enable << " uart_priority " << uart_priority << " core0_m_threshold " << core0_m_threshold << " core0_s_threshold " << core0_s_threshold << "\n";
            return (core0_s_clm >> off);
        }
        else if (addr == UART_PRIORITY_ADDR)
        {
            return (uart_priority >> off);
        }
        else if (addr == SOURCE_PENDING_ADDR)
        {
            return (source_pending >> off);
        }
        else if (addr == CNTXT_0_ENABLE_ADDR)
        {
            // std::cerr << "[DEBUG] PLIC m enable " << core0_m_enable << "\n";
            return (core0_m_enable >> off);
        }
        else if (addr == CNTXT_1_ENABLE_ADDR)
        {
            // std::cerr << "[DEBUG] PLIC s enable " << core0_s_enable << "\n";
            return (core0_s_enable >> off);
        }

        return 0;
    }

    void write(uint32_t addr, uint8_t val)
    {
        uint32_t off = (addr & 3) * 8;
        addr &= ~3;
        uint32_t mask = 0xff << off;

        // std::cerr << "Write PLIC : " << addr + (off >> 3) << ", val = " << (int)(val) << "\n";
        if (addr == CNTXT_0_CLM_CMPLT_THRESHOLD_ADDR)
        {
            core0_m_threshold = (core0_m_threshold & ~mask) | (val << off);
        }
        else if (addr == CNTXT_1_CLM_CMPLT_THRESHOLD_ADDR)
        {
            core0_s_threshold = (core0_s_threshold & ~mask) | (val << off);
            // std::cerr << "[DEBUG] PLIC core0_s_threshold " << core0_s_threshold << "\n";
        }
        else if (addr == CNTXT_0_CLM_CMPLT_ADDR)
        {
            claim_mask &= ~(1 << core0_m_cmplt);
            core0_m_cmplt = (core0_m_cmplt & ~mask) | (val << off);
            // std::cerr << "plic pending " << source_pending << "plic claim_mask " << claim_mask << " core0_m_enable " << core0_m_enable << " core0_s_enable " << core0_s_enable << " uart_priority " << uart_priority << " core0_m_threshold " << core0_m_threshold << " core0_s_threshold " << core0_s_threshold << "\n";
        }
        else if (addr == CNTXT_1_CLM_CMPLT_ADDR)
        {
            claim_mask &= ~(1 << core0_s_cmplt);
            core0_s_cmplt = (core0_s_cmplt & ~mask) | (val << off);
            // std::cerr << "plic pending " << source_pending << "plic claim_mask " << claim_mask << " core0_m_enable " << core0_m_enable << " core0_s_enable " << core0_s_enable << " uart_priority " << uart_priority << " core0_m_threshold " << core0_m_threshold << " core0_s_threshold " << core0_s_threshold << "\n";
        }
        else if (addr == UART_PRIORITY_ADDR)
        {
            uart_priority = (uart_priority & ~mask) | (val << off);
            // std::cerr << "[DEBUG] PLIC UART PRIORITY " << uart_priority << "\n";
        }
        else if (addr == SOURCE_PENDING_ADDR)
        {
            // read-only
        }
        else if (addr == CNTXT_0_ENABLE_ADDR)
        {
            core0_m_enable = (core0_m_enable & ~mask) | (val << off);
        }
        else if (addr == CNTXT_1_ENABLE_ADDR)
        {
            core0_s_enable = (core0_s_enable & ~mask) | (val << off);
            // std::cerr << "[DEBUG] PLIC s enable " << core0_s_enable << "\n";
        }
    }
};

struct CLINT_DEV : Device
{
    uint64_t mtime, mtimecmp;
    CLINT_DEV()
    {
        base = CLINT_BASE;
        top = CLINT_TOP;
        mtime = 0;
        mtimecmp = 0;
    }
    uint8_t read(uint32_t addr) override
    {

        uint32_t off = addr - CLINT_BASE;
        if (off >= 0xBFF8 && off < 0xC000)
        {
            // std::cout << "read mtime :" << mtime << " mtimecmp :" << mtimecmp << "\n";

            uint8_t shift = (off - 0xBFF8) * 8;
            return (mtime >> shift) & 0xff;
        }

        return 0;
    }

    void write(uint32_t addr, uint8_t val) override
    {

        uint32_t off = addr - CLINT_BASE;
        if (off >= 0x4000 && off < 0x4008)
        {
            // std::cout << "read mtime :" << mtime << " mtimecmp :" << mtimecmp << "\n";

            uint8_t shift = (off - 0x4000) * 8;
            uint64_t tmp = ((uint64_t)val) << shift, mask = 0xffULL << shift;
            mtimecmp = (mtimecmp & ~mask) | (tmp & mask);
            // std::cout << "write mtime :" << mtime << " mtimecmp :" << mtimecmp << "\n";
        }
    }
};

struct RAM : Device
{
    uint32_t dseg_base, iseg_base, SIZE;
    std::vector<uint8_t> *imem, *dmem;
    std::vector<bool> *dreserve;
    std::unordered_map<uint32_t, std::vector<uint8_t>> seg_table;
    std::unordered_map<uint32_t, std::vector<bool>> reservation;
    RAM(uint32_t SIZE) : SIZE(SIZE)
    {
        base = 0;
        top = -1;
        dseg_base = 0x80000000;
        iseg_base = 0x80000000;
        build_seg(0x80000000);
        dmem = &seg_table[dseg_base];
        imem = &seg_table[iseg_base];
        dreserve = &reservation[dseg_base];
    }
    void build_seg(uint32_t base)
    {
        if (seg_table.find(base) == seg_table.end())
        {
            seg_table[base].resize(SIZE, 0);
            reservation[base].resize(SIZE, 0);
        }
    }
    inline uint32_t find_seg(uint32_t addr)
    {
        return addr & ~(SIZE - 1);
    }
    uint8_t read(uint32_t addr) override
    {
        uint32_t base = find_seg(addr);
        if (dseg_base != base)
        {
            build_seg(base);
            dseg_base = base;
            dmem = &seg_table[dseg_base];
            dreserve = &reservation[dseg_base];
        }
        return (*dmem)[addr - dseg_base];
    }
    uint8_t amo_lr(uint32_t addr)
    {
        uint32_t base = find_seg(addr);
        if (dseg_base != base)
        {
            build_seg(base);
            dseg_base = base;
            dmem = &seg_table[dseg_base];
            dreserve = &reservation[dseg_base];
        }
        (*dreserve)[addr - dseg_base] = 1;
        return (*dmem)[addr - dseg_base];
    }
    void write(uint32_t addr, uint8_t val) override
    {
        uint32_t base = find_seg(addr);
        if (dseg_base != base)
        {
            dseg_base = base;
            build_seg(base);
            dmem = &seg_table[dseg_base];
            dreserve = &reservation[dseg_base];
        }
        (*dmem)[addr - dseg_base] = val;
        (*dreserve)[addr - dseg_base] = 0;
    }
    uint32_t fetch(uint32_t addr)
    {
        uint32_t base = find_seg(addr);
        if (iseg_base != base)
        {
            build_seg(base);
            iseg_base = base;
            imem = &seg_table[iseg_base];
        }

        uint32_t out = 0;
        for (int i = 3; i >= 0; i--)
        {
            out <<= 8;
            out |= (*imem)[addr + i - iseg_base];
        }
        return out;
    }
    bool amo_sc(uint32_t addr, uint8_t size)
    {
        uint32_t base = find_seg(addr);
        if (dseg_base != base)
        {
            build_seg(base);
            dseg_base = base;
            dmem = &seg_table[dseg_base];
            dreserve = &reservation[dseg_base];
        }
        bool out = (*dreserve)[addr - dseg_base] == 0;
        for (int i = 0; i < size; i++)
        {
            (*dreserve)[addr - dseg_base] = 0;
        }
        return out;
    }
};

struct RVTEST_DEV : Device
{

    RVTEST_DEV(uint32_t tohost_addr)
    {
        base = tohost_addr;
        top = tohost_addr + 4;
    }
    uint8_t read(uint32_t addr) override
    {
        return 0;
    }

    void write(uint32_t addr, uint8_t val) override
    {
        if (addr == base)
        {
            if (val == 1)
            {
                std::cout << "[" << sc_time_stamp() << "]" << "[PASS] riscv-test completed successfully.\n";
                sc_stop();
            }
            else
            {
                std::cout << "[" << sc_time_stamp() << "]" << "[FAIL] riscv-test failed, tohost=0x"
                          << std::hex << (int)val << std::endl;
                sc_stop();
            }
        }
    }
};

template <uint32_t SIZE>
SC_MODULE(Memory)
{
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_in<DMEM_IN> din;
    sc_out<MEM_OUT> dout;
    sc_in<IMEM_IN> iin;
    sc_out<MEM_OUT> iout;

    RAM ram;
    // Devices
    std::vector<Device *> devices;

    SC_HAS_PROCESS(Memory);
    Memory(sc_module_name name) : sc_module(name), ram(SIZE)
    {

        SC_METHOD(inst_fetch);
        sensitive << iin;
        dont_initialize();
        // for non-blocking update memory
        SC_METHOD(mem_access);
        sensitive << din;
        dont_initialize();

        SC_THREAD(update_memory);
        sensitive << clk.pos();
        devices.push_back(&ram);
    }

    void inst_fetch()
    {
        IMEM_IN imem_in = iin.read();
        MEM_OUT imem_out;
        if (imem_in.imem_req_ack)
        {
            // if ((imem_in.imem_addr & 3) == 0)
            if ((imem_in.imem_addr & 1) == 0)
            {
                imem_out.mem_rsp_rdata = ram.fetch(imem_in.imem_addr);
                imem_out.mem_rsp_fault = false;
            }
            else
                imem_out.mem_rsp_fault = true;

            imem_out.mem_rsp_ack = true;
        }
        else
        {
            imem_out.mem_rsp_ack = false;
            imem_out.mem_rsp_rdata = false;
            imem_out.mem_rsp_fault = false;
        }

        iout.write(imem_out);
    }

    bool device_address(uint32_t addr, uint8_t size)
    {
        DMEM_IN dmem_in = din.read();
        return ((dmem_in.dmem_addr >= UART_BASE && dmem_in.dmem_addr < UART_TOP) ||
                (dmem_in.dmem_addr + dmem_in.dmem_size >= UART_BASE && dmem_in.dmem_addr + dmem_in.dmem_size < UART_TOP) ||
                (dmem_in.dmem_addr >= CLINT_BASE && dmem_in.dmem_addr < CLINT_TOP) ||
                (dmem_in.dmem_addr + dmem_in.dmem_size >= CLINT_BASE && dmem_in.dmem_addr + dmem_in.dmem_size < CLINT_TOP));
    }

    void mem_access()
    {
        DMEM_IN dmem_in = din.read();
        MEM_OUT dmem_out;
        if (dmem_in.dmem_req_ack)
        {

            if (dmem_in.dmem_is_amo && (dmem_in.dmem_addr & 3) == 0)
            {
                if (device_address(dmem_in.dmem_addr, dmem_in.dmem_size))
                {
                    std::cerr << "Unexpected AMO on Device\n";
                    dmem_out.mem_rsp_ack = 1;
                    dmem_out.mem_rsp_rdata = 0;
                    dmem_out.mem_rsp_fault = 1;
                    dout.write(dmem_out);
                }
                else
                {
                    switch (dmem_in.dmem_amoop)
                    {
                    case 0: // InstrType::LR_W:
                        dmem_out.mem_rsp_rdata = 0;
                        for (int i = dmem_in.dmem_size - 1; i >= 0; i--)
                        {
                            dmem_out.mem_rsp_rdata <<= 8;
                            dmem_out.mem_rsp_rdata |= ram.amo_lr(dmem_in.dmem_addr + i);
                        }
                        break;
                    case 1: // InstrType::SC_W:
                        dmem_out.mem_rsp_rdata = ram.amo_sc(dmem_in.dmem_addr, dmem_in.dmem_size);
                        break;
                    default:
                        dmem_out.mem_rsp_rdata = 0;
                        for (int i = dmem_in.dmem_size - 1; i >= 0; i--)
                        {
                            dmem_out.mem_rsp_rdata <<= 8;
                            dmem_out.mem_rsp_rdata |= ram.read(dmem_in.dmem_addr + i);
                        }
                        // lazy_wdata = amo_op(dmem_in.dmem_amoop, dmem_in.dmem_wdata, dmem_out.mem_rsp_rdata);
                        break;
                    }
                    dmem_out.mem_rsp_fault = 0;
                    dmem_out.mem_rsp_ack = true;
                }
            }
            else if (dmem_in.dmem_is_load)
            {
                dmem_out.mem_rsp_rdata = 0;
                for (int i = dmem_in.dmem_size - 1; i >= 0; i--)
                {
                    dmem_out.mem_rsp_rdata <<= 8;
                    for (auto dev : devices)
                    {
                        if (dev->base <= dmem_in.dmem_addr + i && dmem_in.dmem_addr + i <= dev->top)
                        {
                            dmem_out.mem_rsp_rdata |= dev->read(dmem_in.dmem_addr + i);
                            // break;
                        }
                    }
                }
                dmem_out.mem_rsp_fault = 0;
                dmem_out.mem_rsp_ack = true;
            }
            else if (dmem_in.dmem_is_store)
            {
                dmem_out.mem_rsp_fault = 0;
                dmem_out.mem_rsp_ack = true;
            }
            else
            {
                dmem_out.mem_rsp_fault = 1;
                dmem_out.mem_rsp_ack = true;
            }
        }
        dout.write(dmem_out);
    }

    void update_memory()
    {
        wait();
        while (1)
        {
            wait();
            DMEM_IN dmem_in = din.read();
            auto dmem_out = dout.read();
            if (dmem_out.mem_rsp_ack && dmem_out.mem_rsp_fault == 0)
            {
                if (dmem_in.dmem_is_amo)
                {

                    switch (dmem_in.dmem_amoop)
                    {
                    case 0: // InstrType::LR_W:
                        break;
                    case 1: // InstrType::SC_W:
                        if (dmem_out.mem_rsp_rdata == 0)
                        {
                            for (int i = 0; i < dmem_in.dmem_size; i++)
                            {
                                ram.write(dmem_in.dmem_addr + i, dmem_in.dmem_wdata);
                                dmem_in.dmem_wdata >>= 8;
                            }
                        }
                        break;
                    default:
                        uint32_t tmp = amo_op(dmem_in.dmem_amoop, dmem_in.dmem_wdata, dmem_out.mem_rsp_rdata);
                        for (int i = 0; i < dmem_in.dmem_size; i++)
                        {
                            ram.write(dmem_in.dmem_addr + i, tmp);
                            tmp >>= 8;
                        }
                        break;
                    }
                }
                else if (dmem_in.dmem_is_store)
                {

                    for (int i = 0; i < dmem_in.dmem_size; i++)
                    {
                        for (auto dev : devices)
                        {
                            if (dev->base <= dmem_in.dmem_addr + i && dmem_in.dmem_addr + i <= dev->top)
                            {
                                dev->write(dmem_in.dmem_addr + i, dmem_in.dmem_wdata);
                                break;
                            }
                        }
                        dmem_in.dmem_wdata >>= 8;
                    }
                }
            }
        }
    }
};
