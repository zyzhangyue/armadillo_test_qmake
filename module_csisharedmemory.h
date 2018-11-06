#ifndef MODULE_CSISHAREDMEMORY_H
#define MODULE_CSISHAREDMEMORY_H

#include "csi_packet.h"
#include <semaphore.h>

#define SHM_PUT 0
#define SHM_GET 1
/*
 * user R/W
 * group R/W
 * others R/W
 */
#define SHM_MODE 0666
#define SHM_SEM_TOKEN "/home/wifi/sem"
#define SHM_CSI_TOKEN "/home/wifi/csi"

class module_csiSharedMemory
{
private:
    int slots, type, index;
    int shmid_sem, shmid_csi;
    sem_t *sem_array;
    csi_packet *csi_array;

private:
    bool init_shm_put();
    bool init_shm_get();
    void destroy_sems(int);
    bool get_full_csi(csi_packet*);

public:
    bool init(int);
    module_csiSharedMemory();
    ~module_csiSharedMemory();
    bool put_csi_to_shm(const csi_packet*);
    bool get_csi_from_shm(csi_packet*, int);
};

#endif // MODULE_CSISHAREDMEMORY_H
