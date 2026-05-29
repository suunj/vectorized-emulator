#ifndef VEMU_H
#define VEMU_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <emmintrin.h>

#define NUM_LANES   2
#define MEM_SIZE    (64 * 1024)
#define NUM_REGS    32
#define MAX_INSN    200000

#define PERM_R   0x01
#define PERM_W   0x02
#define PERM_X   0x04
#define PERM_RWX 0x07

typedef struct {
    uint8_t data[NUM_LANES][MEM_SIZE];
    uint8_t perm[MEM_SIZE];
} MMU;

/* ═══ CPU ═══ */
typedef struct {
    uint32_t x[NUM_REGS];
    uint32_t pc;
    uint32_t mstatus;
    uint32_t cyclel, cycleh;
    uint32_t timerl, timerh;
    uint32_t timermatchl, timermatchh;
    uint32_t mscratch;
    uint32_t mtvec;
    uint32_t mie;
    uint32_t mip;
    uint32_t mepc;
    uint32_t mtval;
    uint32_t mcause;
    uint32_t extraflags;
    bool     active;
    int      exit;
} CPU;

/* ═══ Snapshot ═══ */
typedef struct {
    CPU     cpus[NUM_LANES];
    uint8_t data[NUM_LANES][MEM_SIZE];
} Snapshot;

void     mmu_init(MMU *m);
uint8_t  mmu_read8(MMU *m, int lane, uint32_t addr);
void     mmu_write8(MMU *m, int lane, uint32_t addr, uint8_t val);
uint16_t mmu_read16(MMU *m, int lane, uint32_t addr);
void     mmu_write16(MMU *m, int lane, uint32_t addr, uint16_t val);
uint32_t mmu_read32(MMU *m, int lane, uint32_t addr);
void     mmu_write32(MMU *m, int lane, uint32_t addr, uint32_t val);
uint32_t mmu_fetch32(MMU *m, int lane, uint32_t addr);
void     mmu_load_code(MMU *m, uint32_t addr, const uint8_t *code, int len);
void     mmu_set_perm(MMU *m, uint32_t start, uint32_t len, uint8_t perm);

void exec_scalar(MMU *m, CPU *cpus, int lane);
bool exec_simd(MMU *m, CPU *cpus);
void run(MMU *m, CPU *cpus);

void snap_save(Snapshot *s, CPU *cpus, MMU *m);
void snap_restore(Snapshot *s, CPU *cpus, MMU *m);

#endif
