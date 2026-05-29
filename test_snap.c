#include "vemu.h"

static MMU mmu;
static CPU cpus[NUM_LANES];
static Snapshot snap;

int main(void) {
    int i;
    mmu_init(&mmu);

    /* LUI x10,0x1000 → LBU x10,0(x10) → ECALL */
    uint32_t prog[] = { 0x00001537, 0x00054503, 0x00000073 };
    for (i = 0; i < 3; i++) {
        uint8_t b[4] = { prog[i]&0xFF, (prog[i]>>8)&0xFF,
                         (prog[i]>>16)&0xFF, (prog[i]>>24)&0xFF };
        mmu_load_code(&mmu, 0x100 + i*4, b, 4);
    }

    for (i = 0; i < NUM_LANES; i++) {
        memset(&cpus[i], 0, sizeof(CPU));
        cpus[i].pc = 0x100;
        cpus[i].active = true;
    }

    snap_save(&snap, cpus, &mmu);

    /* 1회차: 'A', 'B' */
    mmu.data[0][0x1000] = 'A';
    mmu.data[1][0x1000] = 'B';
    run(&mmu, cpus);
    printf("1st: VM0=%c VM1=%c\n", cpus[0].x[10], cpus[1].x[10]);

    /* 복원 후 2회차: 'X', 'Y' */
    snap_restore(&snap, cpus, &mmu);
    mmu.data[0][0x1000] = 'X';
    mmu.data[1][0x1000] = 'Y';
    run(&mmu, cpus);
    printf("2nd: VM0=%c VM1=%c\n", cpus[0].x[10], cpus[1].x[10]);

    if (cpus[0].x[10] == 'X' && cpus[1].x[10] == 'Y')
        printf("OK\n");
    else
        printf("FAILED\n");

    return 0;
}
