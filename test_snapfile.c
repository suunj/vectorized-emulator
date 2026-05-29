#include "vemu.h"

static MMU mmu;
static CPU cpus[NUM_LANES];

int main(void) {
    int i;
    mmu_init(&mmu);

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
    mmu.data[0][0x1000] = 0x42;
    mmu.data[1][0x1000] = 0x42;

    /* 저장 */
    printf("1) save\n");
    save_snapshot_files(".", cpus, &mmu);

    /* 파괴 */
    memset(&mmu, 0, sizeof(mmu));
    memset(cpus, 0, sizeof(cpus));
    printf("2) destroyed: pc=0x%04X mem=0x%02X\n", cpus[0].pc, mmu.data[0][0x1000]);

    /* 복원 */
    printf("3) load\n");
    mmu_init(&mmu);
    load_snapshot_files(".", cpus, &mmu);
    printf("4) restored: pc=0x%04X mem=0x%02X\n", cpus[0].pc, mmu.data[0][0x1000]);

    /* 실행 */
    run(&mmu, cpus);
    printf("5) VM0: x10=0x%02X exit=%d\n", cpus[0].x[10], cpus[0].exit);

    if (cpus[0].x[10] == 0x42 && cpus[0].exit == 1)
        printf("OK\n");
    else
        printf("FAILED\n");

    remove("regs.bin");
    remove("mem.bin");
    return 0;
}
