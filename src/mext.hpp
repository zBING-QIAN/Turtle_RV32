#pragma once
#include "instr_cfg.hpp"
#include <systemc>
#include "decode.hpp"
using namespace sc_core;

SC_MODULE(MEXT)
{
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_in<bool> mext_req_ack;
    sc_in<ID_EX> id_ex_port;
    sc_out<bool> mext_rsp_ack;
    sc_out<uint32_t> mext_result;

    SC_HAS_PROCESS(MEXT);
    MEXT(sc_module_name name) : sc_module(name)
    {

        SC_METHOD(mext_seq);
        sensitive << clk.pos();
    }
    void mext_seq()
    {
        if (mext_req_ack)
        {
            switch (id_ex_port.read().type)
            {

            case InstrType::MUL:
                mext_result.write((int32_t)id_ex_port.read().rs1_val * (int32_t)id_ex_port.read().rs2_val);
                mext_rsp_ack.write(1);
                break;
            case InstrType::MULH:
                mext_result.write(((int64_t)(int32_t)id_ex_port.read().rs1_val * (int64_t)(int32_t)id_ex_port.read().rs2_val) >> 32);
                mext_rsp_ack.write(1);
                break;
            case InstrType::MULHSU:
                mext_result.write(((int64_t)(int32_t)id_ex_port.read().rs1_val * (uint64_t)id_ex_port.read().rs2_val) >> 32);
                mext_rsp_ack.write(1);
                break;
            case InstrType::MULHU:
                mext_result.write(((uint64_t)id_ex_port.read().rs1_val * (uint64_t)id_ex_port.read().rs2_val) >> 32);
                mext_rsp_ack.write(1);
                break;
            case InstrType::DIV:
                if (id_ex_port.read().rs2_val == 0)
                    mext_result.write(-1);
                else if ((int32_t)id_ex_port.read().rs1_val == INT32_MIN && (int32_t)id_ex_port.read().rs2_val == -1)
                    mext_result.write(INT32_MIN);
                else
                    mext_result.write((int32_t)id_ex_port.read().rs1_val / (int32_t)id_ex_port.read().rs2_val);
                mext_rsp_ack.write(1);
                break;
            case InstrType::DIVU:
                mext_result.write((id_ex_port.read().rs2_val == 0) ? 0xFFFFFFFF : ((uint32_t)id_ex_port.read().rs1_val / (uint32_t)id_ex_port.read().rs2_val));
                mext_rsp_ack.write(1);
                break;
            case InstrType::REM:
                if (id_ex_port.read().rs2_val == 0)
                    mext_result.write(id_ex_port.read().rs1_val);
                else if ((int32_t)id_ex_port.read().rs1_val == INT32_MIN && (int32_t)id_ex_port.read().rs2_val == -1)
                    mext_result.write(0);
                else
                    mext_result.write((int32_t)id_ex_port.read().rs1_val % (int32_t)id_ex_port.read().rs2_val);
                mext_rsp_ack.write(1);
                break;
            case InstrType::REMU:
                mext_result.write((id_ex_port.read().rs2_val == 0) ? id_ex_port.read().rs1_val : ((uint32_t)id_ex_port.read().rs1_val % (uint32_t)id_ex_port.read().rs2_val));
                mext_rsp_ack.write(1);
                break;
            default:
                mext_result.write(0);
                mext_rsp_ack.write(0);
                break;
            }
        }
        else
        {
            mext_result.write(0);
            mext_rsp_ack.write(0);
        }
    }
};