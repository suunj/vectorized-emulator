#include "vemu.h"
#include <stdlib.h>

int load_flat_binary(MMU *m, const char *path, uint32_t base) {
    FILE *f;
    long flen;
    int lane;
    uint8_t *buf;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (flen > MEM_SIZE) {
        fprintf(stderr, "error: binary too large (%ld > %d)\n", flen, MEM_SIZE);
        fclose(f);
        return -1;
    }

    buf = malloc(flen);
    if (!buf) { fclose(f); return -1; }

    if (fread(buf, 1, flen, f) != (size_t)flen) {
        fprintf(stderr, "error: read failed\n");
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);

    uint32_t addr = base & (MEM_SIZE - 1);
    for (lane = 0; lane < NUM_LANES; lane++)
        memcpy(m->data[lane] + addr, buf, flen);

    printf("  loaded %ld bytes at 0x%04x\n", flen, base);
    free(buf);
    return 0;
}

int save_snapshot_files(const char *dir, CPU *cpus, MMU *m) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), "%s/regs.bin", dir);
    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(&cpus[0], sizeof(CPU), 1, f);
    fclose(f);

    snprintf(path, sizeof(path), "%s/mem.bin", dir);
    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(m->data[0], 1, MEM_SIZE, f);
    fclose(f);

    printf("  snapshot saved to %s/\n", dir);
    return 0;
}

int load_snapshot_files(const char *dir, CPU *cpus, MMU *m) {
    char path[256];
    FILE *f;
    int lane;
    CPU loaded_cpu;
    uint8_t *buf;
    size_t nread;

    snprintf(path, sizeof(path), "%s/regs.bin", dir);
    f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "error: cannot open '%s'\n", path); return -1; }
    if (fread(&loaded_cpu, sizeof(CPU), 1, f) != 1) { fclose(f); return -1; }
    fclose(f);

    for (lane = 0; lane < NUM_LANES; lane++) {
        memcpy(&cpus[lane], &loaded_cpu, sizeof(CPU));
        cpus[lane].active = true;
        cpus[lane].exit = 0;
    }

    snprintf(path, sizeof(path), "%s/mem.bin", dir);
    f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "error: cannot open '%s'\n", path); return -1; }
    buf = malloc(MEM_SIZE);
    if (!buf) { fclose(f); return -1; }
    nread = fread(buf, 1, MEM_SIZE, f);
    fclose(f);

    for (lane = 0; lane < NUM_LANES; lane++)
        memcpy(m->data[lane], buf, nread);

    printf("  snapshot loaded from %s/ (%zu bytes mem)\n", dir, nread);
    free(buf);
    return 0;
}
