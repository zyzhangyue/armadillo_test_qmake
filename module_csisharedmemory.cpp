#include "module_csisharedmemory.h"
#include <sys/ipc.h>
#include <errno.h>
#include <strings.h>
#include <sys/shm.h>
#include <stdio.h>

module_csiSharedMemory::module_csiSharedMemory()
{
    slots = 2;
    type = -1;
    index = 0;
    shmid_sem = -1;
    shmid_csi = -1;
    sem_array = (sem_t*)-1;
    csi_array = (csi_packet*)-1;
}

module_csiSharedMemory::~module_csiSharedMemory()
{
    if (sem_array != (void*)-1) {
        destroy_sems(slots);
        shmdt(sem_array);
    }
    if (csi_array != (void*)-1) {
        shmdt(csi_array);
    }
    if (type == SHM_PUT) {
        if (shmid_sem > 0) {
            shmctl(shmid_sem, IPC_RMID, nullptr);
        }
        if (shmid_csi > 0) {
            shmctl(shmid_csi, IPC_RMID, nullptr);
        }
    }
}

bool module_csiSharedMemory::init(int mtype)
{
    if (mtype == SHM_PUT) {
        type = SHM_PUT;
        return init_shm_put();
    }
    else if (mtype == SHM_GET){
        type = SHM_GET;
        return init_shm_get();
    }
    else {
        fprintf(stderr, "Error initing: 'type' should be either SHM_PUT or SHM_GET\n");
        return false;
    }
}

bool module_csiSharedMemory::init_shm_get()
{
    key_t key_sem = ftok(SHM_SEM_TOKEN, 0);
    key_t key_csi = ftok(SHM_CSI_TOKEN, 0);
    if (key_sem == -1 || key_csi == -1) {
        fprintf(stderr, "Error converting shm keys: %s\n", strerror(errno));
        return false;
    }

    if ((shmid_sem = shmget(key_sem, 0, SHM_MODE)) < 0) {
        fprintf(stderr, "Error getting shared memory segments: %s\n", strerror(errno));
        return false;
    }
    sem_array = (sem_t*)shmat(shmid_sem, nullptr, 0);
    if (sem_array == (void*)-1) {
        fprintf(stderr, "Error getting to shared meory segments: %s\n", strerror(errno));
        return false;
    }

    if ((shmid_csi = shmget(key_csi, 0, SHM_MODE)) < 0) {
        fprintf(stderr, "Error allocating shared memory segments: %s\n", strerror(errno));
        return false;
    }
    csi_array = (csi_packet*)shmat(shmid_csi, nullptr, 0);
    if (csi_array == (void*)-1) {
        fprintf(stderr, "Error attaching to shared meory segments: %s\n", strerror(errno));
        return false;
    }
    return true;
}

bool module_csiSharedMemory::init_shm_put()
{
    key_t key_sem = ftok(SHM_SEM_TOKEN, 0);
    key_t key_csi = ftok(SHM_CSI_TOKEN, 0);
    // convert a global token to shm keys
    if (key_sem == -1 || key_csi == -1) {
        fprintf(stderr, "Error converting shm keys: %s\n", strerror(errno));
        return false;
    }
    // get shm segment for semaphores
    if ((shmid_sem = shmget(key_sem, sizeof(sem_t)*slots, SHM_MODE|IPC_CREAT|IPC_EXCL)) < 0) {
        fprintf(stderr, "Error allocating shared memory segments: %s\n", strerror(errno));
        return false;
    }
    sem_array = (sem_t*)shmat(shmid_sem, nullptr, 0);
    if (sem_array == (void*)-1) {
        fprintf(stderr, "Error attaching to shared meory segments: %s\n", strerror(errno));
        return false;
    }
    // get shm segment for csi buffer
    if ((shmid_csi = shmget(key_csi, sizeof(csi_packet)*slots, SHM_MODE|IPC_CREAT|IPC_EXCL)) < 0) {
        fprintf(stderr, "Error allocating shared memory segments: %s\n", strerror(errno));
        return false;
    }
    csi_array = (csi_packet*)shmat(shmid_csi, nullptr, 0);
    if (csi_array == (void*)-1) {
        fprintf(stderr, "Error attaching to shared meory segments: %s\n", strerror(errno));
        return false;
    }

    // initialize semaphore array
    for (int i = 0; i < slots; i++) {
        if (sem_init(sem_array+i, 1, 0) < 0) {
            fprintf(stderr, "Error initilizing semaphores: %s\n", strerror(errno));
            return false;
        }
    }
    return true;
}

bool module_csiSharedMemory::put_csi_to_shm(const csi_packet *packet)
{
    if (type == -1) {
        fprintf(stderr, "Use after initialization\n");
        return false;
    }
    if (type != SHM_PUT) {
        fprintf(stderr, "No right to put csi packets\n");
        return false;
    }
    if (packet == nullptr) {
        fprintf(stderr, "Null pointer: No where to store packet\n");
        return false;
    }
    int value;
    while (true) {
        if (sem_getvalue(sem_array+index, &value) < 0) {
            fprintf(stderr, "Error calling sem_getvalue: %s\n", strerror(errno));
            return false;
        }
        if (value == 0)
            break;
    }
    memcpy(csi_array+index, packet, sizeof(csi_packet));
    while (true) {
        if (sem_post(sem_array+index) < 0) {
            if (errno == EINTR)
                continue;
            else
                return false;
        }
        break;
    }
    index = (index+1) % slots;
    return true;
}

bool module_csiSharedMemory::get_full_csi(csi_packet *packet)
{
    int value;
    while (true) {
        if (sem_getvalue(sem_array+index, &value) < 0) {
            fprintf(stderr, "Error calling sem_getvalue: %s\n", strerror(errno));
            return false;
        }
        if (value != 0)
            break;
    }
    memcpy(packet, csi_array+index, sizeof(csi_packet));
    while (true) {
        if (sem_wait(sem_array+index) < 0) {
            if (errno == EINTR)
                continue;
            else
                return false;
        }
        break;
    }
    index = (index+1) % slots;
    return true;
}

bool module_csiSharedMemory::get_csi_from_shm(csi_packet *packet, int downsampling)
{
    if (type == -1) {
        fprintf(stderr, "Use after initialization\n");
        return false;
    }
    if (type != SHM_GET) {
        fprintf(stderr, "No right to get csi packets\n");
        return false;
    }
    if (packet == nullptr) {
        fprintf(stderr, "Null pointer: No where to store packet\n");
        return false;
    }
    for (int i = 0; i < downsampling; i++)
        if (!get_full_csi(packet))
            return false;
    return true;
}

void module_csiSharedMemory::destroy_sems(int index)
{
    for (int i = 0; i < index; i++)
        sem_destroy(sem_array+i);
}
