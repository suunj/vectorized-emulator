#include "vemu.h"

static MMU mmu;
static CPU cpus[NUM_LANES];

int main(void) {
    int i;
    mmu_init(&mmu);

    uint32_t prog[] = { 0x00100513, 0x00150513, 0x00150513, 0x00000073 };
    for (i = 0; i < 4; i++) {
        uint8_t b[4] = { prog[i]&0xFF, (prog[i]>>8)&0xFF,
                         (prog[i]>>16)&0xFF, (prog[i]>>24)&0xFF };
        mmu_load_code(&mmu, 0x100 + i*4, b, 4);
    }

    for (i = 0; i < NUM_LANES; i++) {
        memset(&cpus[i], 0, sizeof(CPU));
        cpus[i].pc = 0x100;
        cpus[i].active = true;
    }

    run(&mmu, cpus);
    printf("VM0: x10=%d  VM1: x10=%d\n", cpus[0].x[10], cpus[1].x[10]);

    if (cpus[0].x[10] == 3 && cpus[1].x[10] == 3)
        printf("OK\n");
    else
        printf("FAILED\n");

    return 0;
}
