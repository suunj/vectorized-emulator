#include "vemu.h"

void exec_scalar(MMU *m, CPU *cpus, int lane) {
    CPU *c = &cpus[lane];
    uint32_t ir, rval, pc, rdid, rs1v, rs2v;
    uint32_t imm, imm_se, ofs;
    uint32_t trap = 0;

    if (!c->active) return;

    pc = c->pc;
    ir = mmu_fetch32(m, lane, pc);
    rdid = (ir >> 7) & 0x1f;
    rval = 0;

    switch (ir & 0x7f) {

    case 0x37: /* LUI */
        rval = (ir & 0xfffff000);
        break;

    case 0x17: /* AUIPC */
        rval = pc + (ir & 0xfffff000);
        break;

    case 0x6F: /* JAL */
    {
        int32_t reladdy = ((ir & 0x80000000)>>11) | ((ir & 0x7fe00000)>>20)
                        | ((ir & 0x00100000)>>9) | (ir & 0x000ff000);
        if (reladdy & 0x00100000) reladdy |= 0xffe00000;
        rval = pc + 4;
        pc = pc + reladdy - 4;
        break;
    }

    case 0x67: /* JALR */
    {
        imm = ir >> 20;
        imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);
        rval = pc + 4;
        pc = ((c->x[(ir >> 15) & 0x1f] + imm_se) & ~1u) - 4;
        break;
    }

    case 0x63: /* BRANCH */
    {
        uint32_t immm4 = ((ir & 0xf00)>>7) | ((ir & 0x7e000000)>>20)
                       | ((ir & 0x80) << 4) | ((ir >> 31)<<12);
        if (immm4 & 0x1000) immm4 |= 0xffffe000;
        int32_t brs1 = (int32_t)c->x[(ir >> 15) & 0x1f];
        int32_t brs2 = (int32_t)c->x[(ir >> 20) & 0x1f];
        immm4 = pc + immm4 - 4;
        rdid = 0;
        switch ((ir >> 12) & 0x7) {
            case 0: if (brs1 == brs2) pc = immm4; break;
            case 1: if (brs1 != brs2) pc = immm4; break;
            case 4: if (brs1 < brs2) pc = immm4; break;
            case 5: if (brs1 >= brs2) pc = immm4; break;
            case 6: if ((uint32_t)brs1 < (uint32_t)brs2) pc = immm4; break;
            case 7: if ((uint32_t)brs1 >= (uint32_t)brs2) pc = immm4; break;
            default: trap = 3; break;
        }
        break;
    }

    case 0x03: /* LOAD */
    {
        rs1v = c->x[(ir >> 15) & 0x1f];
        imm = ir >> 20;
        imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);
        ofs = rs1v + imm_se;
        switch ((ir >> 12) & 0x7) {
            case 0: rval = (uint32_t)(int32_t)(int8_t)mmu_read8(m, lane, ofs); break;
            case 1: rval = (uint32_t)(int32_t)(int16_t)mmu_read16(m, lane, ofs); break;
            case 2: rval = mmu_read32(m, lane, ofs); break;
            case 4: rval = mmu_read8(m, lane, ofs); break;
            case 5: rval = mmu_read16(m, lane, ofs); break;
            default: trap = 3; break;
        }
        break;
    }

    case 0x23: /* STORE */
    {
        rs1v = c->x[(ir >> 15) & 0x1f];
        rs2v = c->x[(ir >> 20) & 0x1f];
        uint32_t addy = ((ir >> 7) & 0x1f) | ((ir & 0xfe000000) >> 20);
        if (addy & 0x800) addy |= 0xfffff000;
        addy += rs1v;
        rdid = 0;
        switch ((ir >> 12) & 0x7) {
            case 0: mmu_write8(m, lane, addy, rs2v & 0xFF); break;
            case 1: mmu_write16(m, lane, addy, rs2v & 0xFFFF); break;
            case 2: mmu_write32(m, lane, addy, rs2v); break;
            default: trap = 3; break;
        }
        break;
    }

    case 0x13: /* OP-IMM */
    case 0x33: /* OP */
    {
        imm = ir >> 20;
        imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);
        rs1v = c->x[(ir >> 15) & 0x1f];
        uint32_t is_reg = !!(ir & 0x20);
        rs2v = is_reg ? c->x[imm & 0x1f] : imm_se;

        if (is_reg && (ir & 0x02000000)) {
            switch ((ir >> 12) & 7) {
                case 0: rval = rs1v * rs2v; break;
                case 1: rval = (uint32_t)(((int64_t)(int32_t)rs1v * (int64_t)(int32_t)rs2v) >> 32); break;
                case 2: rval = (uint32_t)(((int64_t)(int32_t)rs1v * (uint64_t)rs2v) >> 32); break;
                case 3: rval = (uint32_t)(((uint64_t)rs1v * (uint64_t)rs2v) >> 32); break;
                case 4: if (rs2v == 0) rval = (uint32_t)-1;
                         else rval = ((int32_t)rs1v == INT32_MIN && (int32_t)rs2v == -1) ? rs1v : (uint32_t)((int32_t)rs1v / (int32_t)rs2v);
                         break;
                case 5: rval = rs2v == 0 ? 0xffffffff : rs1v / rs2v; break;
                case 6: if (rs2v == 0) rval = rs1v;
                         else rval = ((int32_t)rs1v == INT32_MIN && (int32_t)rs2v == -1) ? 0 : (uint32_t)((int32_t)rs1v % (int32_t)rs2v);
                         break;
                case 7: rval = rs2v == 0 ? rs1v : rs1v % rs2v; break;
            }
        } else {
            switch ((ir >> 12) & 7) {
                case 0: rval = (is_reg && (ir & 0x40000000)) ? (rs1v - rs2v) : (rs1v + rs2v); break;
                case 1: rval = rs1v << (rs2v & 0x1F); break;
                case 2: rval = (int32_t)rs1v < (int32_t)rs2v; break;
                case 3: rval = rs1v < rs2v; break;
                case 4: rval = rs1v ^ rs2v; break;
                case 5: rval = (ir & 0x40000000) ? (uint32_t)((int32_t)rs1v >> (rs2v & 0x1F)) : (rs1v >> (rs2v & 0x1F)); break;
                case 6: rval = rs1v | rs2v; break;
                case 7: rval = rs1v & rs2v; break;
            }
        }
        break;
    }

    case 0x0f: /* FENCE */
        rdid = 0;
        break;

    case 0x73: /* ECALL/EBREAK */
    {
        uint32_t csrno = ir >> 20;
        uint32_t microop = (ir >> 12) & 0x7;
        if (microop == 0) {
            rdid = 0;
            switch (csrno) {
                case 0:
                    c->active = false;
                    c->exit = 1;
                    c->pc = pc + 4;
                    c->x[0] = 0;
                    return;
                case 1:
                    c->active = false;
                    c->exit = 2;
                    c->pc = pc + 4;
                    c->x[0] = 0;
                    return;
                default: trap = 3; break;
            }
        } else {
            trap = 3;
        }
        break;
    }

    default:
        trap = 3;
        break;
    }

    if (trap) {
        c->active = false;
        c->exit = 2;
        c->pc = pc;
        c->x[0] = 0;
        return;
    }

    if (rdid) c->x[rdid] = rval;
    c->x[0] = 0;
    c->pc = pc + 4;
    c->cyclel++;
}
