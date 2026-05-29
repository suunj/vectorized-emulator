
#include "vemu.h"

static MMU mmu;
static CPU cpus[NUM_LANES];

int main(void) {
    int i;

    /* 테스트용 .bin 파일 생성: ADDI x10,x0,99 → ECALL */
    uint32_t prog[] = { 0x06300513, 0x00000073 };
    FILE *f = fopen("_test.bin", "wb");
    for (i = 0; i < 2; i++) {
        uint8_t b[4] = { prog[i]&0xFF, (prog[i]>>8)&0xFF,
                         (prog[i]>>16)&0xFF, (prog[i]>>24)&0xFF };
        fwrite(b, 1, 4, f);
    }
    fclose(f);

    mmu_init(&mmu);
    if (load_flat_binary(&mmu, "_test.bin", 0x0000) < 0)
        return 1;

    for (i = 0; i < NUM_LANES; i++) {
        memset(&cpus[i], 0, sizeof(CPU));
        cpus[i].pc = 0x0000;
        cpus[i].active = true;
    }

    run(&mmu, cpus);
    printf("VM0: x10=%d exit=%d\n", cpus[0].x[10], cpus[0].exit);

    if (cpus[0].x[10] == 99 && cpus[0].exit == 1)
        printf("OK\n");
    else
        printf("FAILED\n");

    remove("_test.bin");
    return 0;
}
