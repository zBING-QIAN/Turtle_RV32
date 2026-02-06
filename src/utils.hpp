#pragma once
#include "elf_loader32.hpp"
#include <cstdint>
#include <getopt.h>
#include "tb.hpp"
void load_to_ram(const char *filename, Testbench *tb, uint32_t offset)
{
    std::cerr << "Binary file " << filename << " Loading" << std::endl;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open: " << filename << std::endl;
        return;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size, 0);
    if (!file.read((char *)buf.data(), size))
        throw std::runtime_error("failed to read binary file contents");
    uint32_t addr = offset;
    uint32_t base = tb->memory.ram.find_seg(addr);
    tb->memory.ram.build_seg(base);
    std::vector<uint8_t> *mem = &tb->memory.ram.seg_table[base];

    std::cerr << "Write to RAM" << std::endl;
    for (uint32_t i = 0; i < size; i++, addr++)
    {
        uint32_t base_tmp = tb->memory.ram.find_seg(addr);
        if (base != base_tmp)
        {
            base = base_tmp;
            tb->memory.ram.build_seg(base);
            mem = &tb->memory.ram.seg_table[base];
        }
        (*mem)[addr - base] = buf[i];
    }
    std::cerr << "Binary file " << filename << " Loaded" << std::endl;
    file.close();
}

void run_ui_console(SharedState *share_state)
{
    std::string s;
    // PROCESS 0: Keyboard Reader
    while (true)
    {
        std::getline(std::cin, s);
        int len = s.size();
        if (len)
        {
            if (s[0] == ':')
            {
                std::vector<std::string> commands;
                std::string tmp;
                for (int i = 1; i < len; i++)
                {
                    if (s[i] == ' ' || s[i] == '\n')
                    {
                        commands.push_back(tmp);
                        tmp = "";
                    }
                    else
                        tmp += s[i];
                }
                if (tmp.size() > 0)
                {
                    commands.push_back(tmp);
                    // std::cerr << "tmp is " << tmp << " len = " << len << "\n";
                }
                if (commands.size() > 0)
                {
                    unsigned long long arg2 = 0;
                    if (commands.size() > 1)
                    {
                        // std::cerr << commands[1] << "  second args\n";
                        try
                        {
                            arg2 = std::stoull(commands[1]);
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cerr << "Second input should be number\n";
                            arg2 = 0;
                        }
                        // std::cerr << arg2 << "\n";
                    }
                    if (commands[0] == "b" && arg2 > 0)
                    { // brk (only one breakpoint)
                        share_state->breakpoint = 1;
                        share_state->brk_pc = arg2;
                    }
                    else if (commands[0] == "d") // delete breakpoint
                    {                            // continue
                        share_state->breakpoint = 0;
                    }
                    else if (commands[0] == "c")
                    { // continue
                        share_state->force_run = 1;
                    }
                    else if (commands[0] == "p" && arg2 > 0)
                    { // print
                        share_state->new_print_cycles = arg2;
                    }
                    else if (commands[0] == "stop")
                    {
                        share_state->force_stop = 1;
                    }
                    else if (commands[0] == "step" && arg2 > 0)
                    {
                        share_state->new_step_cycles = arg2;
                    }
                    else if (commands[0] == "r")
                    {
                        share_state->force_run = 1;
                        share_state->start = 1;
                        // std::cerr << "start simulate\n";
                    }
                    else if (commands[0] == "timeirq")
                    {
                        share_state->timeirq = 1;
                        // std::cerr << "trigger timer interrupt\n";
                    }
                }
            }
            else if (share_state->uart_buf[0] == 0)
            {
                for (int i = 0; i < len && i < 128; i++)
                    share_state->uart_buf[i] = s[i];
                if (len >= 128)
                    share_state->uart_buf[127] = '\n';
                else
                    share_state->uart_buf[len] = '\n';
            }
        }
    }
}

// Unified configuration structure
struct Config
{
    std::string mode = "linux"; // default
    std::string elf_path;
    std::string linux_path;
    std::string dtb_path;
    std::string sbi_path;
    std::string sig_path;
    int max_time = 0;
    uint32_t sig_start = 0;
    uint32_t sig_end = 0;
};
Config parse_args(int argc, char **argv)
{
    Config cfg;
    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"linux", required_argument, 0, 'l'},
        {"dtb", required_argument, 0, 'd'},
        {"sbi", required_argument, 0, 's'},
        {"elf", required_argument, 0, 'e'},
        {"sig", required_argument, 0, 'g'},
        {"max_time", required_argument, 0, 't'},
        {"start", required_argument, 0, 'b'}, // 'b' for begin
        {"end", required_argument, 0, 'f'},   // 'f' for finish
        {0, 0, 0, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "m:l:d:s:e:g:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'm':
            cfg.mode = optarg;
            break;
        case 'l':
            cfg.linux_path = optarg;
            break;
        case 'd':
            cfg.dtb_path = optarg;
            break;
        case 's':
            cfg.sbi_path = optarg;
            break;
        case 'e':
            cfg.elf_path = optarg;
            break;
        case 'g':
            cfg.sig_path = optarg;
            break;
        case 't':
            cfg.max_time = std::stoi(optarg);
            break;
        case 'b':
            cfg.sig_start = std::stoul(optarg, nullptr, 16);
            break;
        case 'f':
            cfg.sig_end = std::stoul(optarg, nullptr, 16);
            break;
        }
    }
    return cfg;
}
// Abstract base for different test types
class EmulatorMode
{
public:
    Config cfg;
    virtual ~EmulatorMode() = default;
    virtual void init() = 0;
    virtual void execute()
    {
        if (cfg.max_time <= 0)
        {
            sc_start();
        }
        else
        {
            sc_start(cfg.max_time, SC_NS);
        }
    }
};
