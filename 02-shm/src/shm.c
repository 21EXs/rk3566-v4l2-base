#include "../inc/shm.h"

void Shm_Create()
{
    size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
    size_t total_size = sizeof(struct shared_memory) + nv21_data_size + argb_data_size;
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, total_size);

    struct shared_memory *shm = mmap(NULL, total_size, PROT_READ|PROT_WRITE, 
                                    MAP_SHARED, shm_fd, 0);

    shm->nv21.meta.width = WIDTH;
    shm->nv21.meta.height = HEIGHT;
    shm->nv21.meta.format = 0;  // NV21
    shm->nv21.data_offset = sizeof(struct shared_memory);  // NV21数据在数据池开头

    shm->argb.meta.width = WIDTH;
    shm->argb.meta.height = HEIGHT;  
    shm->argb.meta.format = 1;  // ARGB8888
    shm->argb.data_offset = sizeof(struct shared_memory) + nv21_data_size;  // ARGB数据在NV21后面

    sem_init(&shm->nv21.semaphore, 1, 1);
    sem_init(&shm->argb.semaphore, 1, 1);

    printf("共享内存创建、初始化成功\n");
}
