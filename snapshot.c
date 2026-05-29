#include "vemu.h"

void snap_save(Snapshot *s, CPU *cpus, MMU *m) {
    memcpy(s->cpus, cpus, sizeof(CPU) * NUM_LANES);
    memcpy(s->data, m->data, sizeof(m->data));
}

void snap_restore(Snapshot *s, CPU *cpus, MMU *m) {
    memcpy(cpus, s->cpus, sizeof(CPU) * NUM_LANES);
    memcpy(m->data, s->data, sizeof(m->data));
    for (int i = 0; i < NUM_LANES; i++) {
        cpus[i].active = true;
        cpus[i].exit = 0;
    }
}
