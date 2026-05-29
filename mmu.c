#include "vemu.h"

void mmu_init(MMU *m) {
    memset(m, 0, sizeof(*m));
    memset(m->perm, PERM_RWX, MEM_SIZE);
}

static inline uint32_t wrap(uint32_t addr) { return addr & (MEM_SIZE - 1); }

uint8_t mmu_read8(MMU *m, int lane, uint32_t addr) {
    addr = wrap(addr);
    if (!(m->perm[addr] & PERM_R)) return 0;
    return m->data[lane][addr];
}

void mmu_write8(MMU *m, int lane, uint32_t addr, uint8_t val) {
    addr = wrap(addr);
    if (!(m->perm[addr] & PERM_W)) return;
    m->data[lane][addr] = val;
}

uint16_t mmu_read16(MMU *m, int lane, uint32_t addr) {
    return mmu_read8(m, lane, addr) | ((uint16_t)mmu_read8(m, lane, addr+1) << 8);
}

void mmu_write16(MMU *m, int lane, uint32_t addr, uint16_t val) {
    mmu_write8(m, lane, addr, val & 0xFF);
    mmu_write8(m, lane, addr+1, (val >> 8) & 0xFF);
}

uint32_t mmu_read32(MMU *m, int lane, uint32_t addr) {
    return mmu_read16(m, lane, addr) | ((uint32_t)mmu_read16(m, lane, addr+2) << 16);
}

void mmu_write32(MMU *m, int lane, uint32_t addr, uint32_t val) {
    mmu_write16(m, lane, addr, val & 0xFFFF);
    mmu_write16(m, lane, addr+2, (val >> 16) & 0xFFFF);
}

uint32_t mmu_fetch32(MMU *m, int lane, uint32_t addr) {
    addr = wrap(addr);
    if (!(m->perm[addr] & PERM_X)) return 0x00100073;
    return mmu_read32(m, lane, addr);
}

void mmu_load_code(MMU *m, uint32_t addr, const uint8_t *code, int len) {
    int lane;
    for (lane = 0; lane < NUM_LANES; lane++)
        memcpy(m->data[lane] + wrap(addr), code, len);
}

void mmu_set_perm(MMU *m, uint32_t start, uint32_t len, uint8_t perm) {
    uint32_t i;
    for (i = 0; i < len; i++)
        m->perm[wrap(start + i)] = perm;
}
