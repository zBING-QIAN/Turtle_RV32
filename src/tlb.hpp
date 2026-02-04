#pragma once
#include <vector>
#include <cstdint>
struct TLB
{
    struct TLBEntry
    {
        uint32_t vpn : 20;
        uint32_t asid : 10;
        uint32_t pte : 32; // Full original PTE
        bool valid : 1;
        bool is_megapage : 1;
        TLBEntry()
        {
            valid = 0;
        }
    };
    int size = 32;
    std::vector<TLBEntry> table;
    TLB() { table.resize(size, TLBEntry()); }
    void flush(uint32_t asid, uint32_t vpn)
    {
        for (int i = size - 1; i >= 0; i--)
        {
            if ((table[i].asid == asid || asid == 0) && (vpn == table[i].vpn || vpn == 0))
            {
                table[i].valid = 0;
            }
        }
    }
    TLBEntry find(uint32_t asid, uint32_t vpn)
    {
        TLBEntry tlbe;
        for (int i = 0; i < size; i++)
        {
            if (table[i].valid &&                                                  // check valid
                ((table[i].pte & 0x20) == 0x20 || asid == table[i].asid) &&        // asid match
                ((table[i].vpn >> 10) == (vpn >> 10)) &&                           // first part ppn match
                (table[i].is_megapage || (table[i].vpn & 0x3ff) == (vpn & 0x3ff))) // megapage or full match
            {
                tlbe = table[i];
                if (i)
                {
                    TLBEntry tmp = table[i];
                    table[i] = table[0];
                    table[0] = tmp;
                }
                break;
            }
        }
        // std::cout << "tlb find " << asid << " " << vpn << " " << tlbe.pte << std::endl;
        return tlbe;
    }
    void store(uint32_t asid, uint32_t vpn, uint32_t pte, bool is_megapage)
    {
        TLBEntry tmp;
        tmp.valid = 1;
        tmp.pte = pte;
        tmp.is_megapage = is_megapage;

        if (tmp.is_megapage)
        {
            vpn &= (0xfffffc00);
        }
        tmp.vpn = vpn;
        tmp.asid = asid;
        for (int i = 0; i < size; i++)
        {
            if ((table[i].asid == asid | table[i].vpn == vpn) ||
                !table[i].valid)
            {
                table[i] = tmp;
                break;
            }
            else
            {
                TLBEntry tmp_swap = table[i];
                table[i] = tmp;
                tmp = tmp_swap;
            }
        }
    }
};
