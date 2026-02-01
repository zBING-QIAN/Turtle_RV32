// elf_loader32.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <elf.h> // use system elf.h (POSIX)
#include <stdexcept>
#include <iomanip>

struct SegmentInfo
{
    uint32_t vaddr; // ELF virtual base address
    uint32_t memsz; // in-memory size (including bss)
    uint32_t flags; // permissions (PF_X/PF_W/PF_R)
    std::vector<uint8_t> mem;
};

struct LoadedELF
{
    uint32_t entry;
    std::vector<SegmentInfo> segments; //  record both virtual and loaded locations
};

static void print_segment_map(const LoadedELF &elf)
{
    std::cout << "=== ELF Segment Map ===\n";
    std::cout << "Idx |  VirtAddr  |   MemSz   | Flags\n";
    std::cout << "----+------------+------------+----------+----------+--------\n";
    for (size_t i = 0; i < elf.segments.size(); ++i)
    {
        const auto &s = elf.segments[i];
        std::cout << std::dec << std::setw(3) << i << " | "
                  << std::hex << std::setfill('0')
                  << "0x" << std::setw(8) << s.vaddr << " | "
                  << "0x" << std::setw(6) << s.memsz << " | "
                  << ((s.flags & PF_R) ? 'R' : '-')
                  << ((s.flags & PF_W) ? 'W' : '-')
                  << ((s.flags & PF_X) ? 'X' : '-')
                  << "\n";
    }
    std::cout << "Loaded ELF: base_vaddr=0x"
              << " entry=0x" << elf.entry << std::dec << "\n\n";
}

static inline void read_file_to_vector(const std::string &path, std::vector<uint8_t> &out)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        throw std::runtime_error("failed to open ELF file");
    std::streamsize sz = f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize((size_t)sz);
    if (!f.read((char *)out.data(), sz))
        throw std::runtime_error("failed to read ELF file contents");
}

LoadedELF load_elf32(const std::string &path)
{
    std::vector<uint8_t> filedata;
    read_file_to_vector(path, filedata);

    if (filedata.size() < sizeof(Elf32_Ehdr))
        throw std::runtime_error("file too small for ELF header");

    const Elf32_Ehdr *eh = reinterpret_cast<const Elf32_Ehdr *>(filedata.data());

    // Basic validation
    if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3)
        throw std::runtime_error("not an ELF file");

    if (eh->e_ident[EI_CLASS] != ELFCLASS32)
        throw std::runtime_error("not ELF32");
    if (eh->e_ident[EI_DATA] != ELFDATA2LSB)
        throw std::runtime_error("not little-endian");
    if (eh->e_machine != EM_RISCV)
    {
        std::cerr << "warning: ELF e_machine = " << eh->e_machine << " (expected EM_RISCV=" << EM_RISCV << ")\n";
        // allow for testing different toolchains, but you may want to require EM_RISCV later
    }

    // Read program headers
    if (eh->e_phoff == 0 || eh->e_phnum == 0)
        throw std::runtime_error("no program headers");

    const uint8_t *base = filedata.data();
    const Elf32_Phdr *phdrs = reinterpret_cast<const Elf32_Phdr *>(base + eh->e_phoff);

    LoadedELF out;
    // Load segments
    for (int i = 0; i < eh->e_phnum; ++i)
    {
        const Elf32_Phdr &ph = phdrs[i];
        if (ph.p_type != PT_LOAD)
            continue;
        std::vector<uint8_t> mem((uint32_t)ph.p_memsz, 0);
        if (ph.p_offset + ph.p_filesz > filedata.size())
        {
            throw std::runtime_error("segment file size / offset out of range");
        }

        // copy file-backed bytes
        std::copy_n(base + ph.p_offset, ph.p_filesz, mem.begin());
        SegmentInfo info{
            ph.p_vaddr,
            ph.p_memsz,
            ph.p_flags,
            mem};
        out.segments.push_back(info);
    }

    out.entry = eh->e_entry;

    print_segment_map(out);

    return out;
}
