#include "vemu.h"

static MMU mmu;
static CPU cpus[NUM_LANES];

int main(void) {
    int i;
    mmu_init(&mmu);

    uint32_t prog[] = { 0x02A00513, 0x00000073 };
    for (i = 0; i < 2; i++) {
        uint8_t b[4] = { prog[i]&0xFF, (prog[i]>>8)&0xFF,
                         (prog[i]>>16)&0xFF, (prog[i]>>24)&0xFF };
        mmu_load_code(&mmu, 0x100 + i*4, b, 4);
    }

    for (i = 0; i < NUM_LANES; i++) {
        memset(&cpus[i], 0, sizeof(CPU));
        cpus[i].pc = 0x100;
        cpus[i].active = true;
    }

    exec_scalar(&mmu, cpus, 0);
    printf("after ADDI: x10=%d pc=0x%04X\n", cpus[0].x[10], cpus[0].pc);

    exec_scalar(&mmu, cpus, 0);
    printf("after ECALL: exit=%d active=%d\n", cpus[0].exit, cpus[0].active);

    if (cpus[0].x[10] == 42 && cpus[0].exit == 1)
        printf("OK\n");
    else
        printf("FAILED\n");

    return 0;
}
