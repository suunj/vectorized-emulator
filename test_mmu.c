#include "vemu.h"

int main(void) {
    MMU mmu;
    mmu_init(&mmu);

    mmu_write8(&mmu, 0, 0x1000, 0xAA);
    mmu_write8(&mmu, 1, 0x1000, 0xBB);
    printf("VM0=0x%02X VM1=0x%02X\n", mmu_read8(&mmu, 0, 0x1000), mmu_read8(&mmu, 1, 0x1000));

    mmu_write32(&mmu, 0, 0x2000, 0xDEADBEEF);
    printf("32bit=0x%08X\n", mmu_read32(&mmu, 0, 0x2000));

    mmu_set_perm(&mmu, 0x3000, 16, PERM_W);
    mmu_write8(&mmu, 0, 0x3000, 0xFF);
    printf("no-read=0x%02X\n", mmu_read8(&mmu, 0, 0x3000));
    return 0;
}
