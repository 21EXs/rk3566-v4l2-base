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

    printf("=== 共享内存布局 ===\n");
    printf("结构体基地址: %p\n", shm);
    printf("结构体大小: %zu 字节\n", sizeof(struct shared_memory));
    printf("NV21数据大小: %zu 字节\n", nv21_data_size);
    printf("ARGB数据大小: %zu 字节\n", argb_data_size);
    printf("总大小: %zu 字节\n", total_size);
    printf("NV21偏移量: %u 字节\n", shm->nv21.data_offset[0]);
    printf("NV21绝对地址: %p\n", (uint8_t*)shm + shm->nv21.data_offset[0]);
    printf("ARGB偏移量: %u 字节\n", shm->argb.data_offset[0]);
    printf("ARGB绝对地址: %p\n", (uint8_t*)shm + shm->argb.data_offset[0]);
    printf("==================\n");

    shm->nv21.meta.width = WIDTH;
    shm->nv21.meta.height = HEIGHT;
    shm->nv21.meta.format = 0;  // NV21
    shm->nv21.data_offset[0] = sizeof(struct shared_memory);  // NV21数据在数据池开头

    shm->argb.meta.width = WIDTH;
    shm->argb.meta.height = HEIGHT;  
    shm->argb.meta.format = 1;  // ARGB8888
    shm->argb.data_offset[0] = sizeof(struct shared_memory) + nv21_data_size;  // ARGB数据在NV21后面

    for(uint8_t Frame_Index = 0; Frame_Index < FRAME_NUM ; Frame_Index++)
    {
        shm->nv21.data_offset[Frame_Index] = Get_Frame_Data_Offset(&shm,NV21_TYPE ,Frame_Index);
        shm->argb.data_offset[Frame_Index] = Get_Frame_Data_Offset(&shm,BGRA_TYPE ,Frame_Index);
    }

    sem_init(&shm->sem.capture_done, 1, 0);
    sem_init(&shm->sem.convert_done, 1, 0);
    sem_init(&shm->sem.display_done, 1, FRAME_NUM);//表示有FRAME_NUM个空闲帧

    printf("共享内存创建、初始化成功\n");
}

uint8_t* GetAvailPollAddr(uint8_t PollType)
{
    struct shared_memory *shm_ptr = NULL;
    uint8_t* AvailNV21PollAddr = NULL;
    uint8_t* AvailBGRAPollAddr = NULL;
    sem_getvalue(&shm_ptr->sem.display_done,shm_ptr->sem.NV21_Avail_Buf);//获取信号量以计算哪些缓冲区可用
    sem_getvalue(&shm_ptr->sem.capture_done,shm_ptr->sem.Convert_Avail_Buf);//获取信号量以计算哪些缓冲区可用
    sem_getvalue(&shm_ptr->sem.convert_done,shm_ptr->sem.Display_Avail_Buf);//获取信号量以计算哪些缓冲区可用
    AvailNV21PollAddr = Get_Frame_Data_Offset(shm_ptr,NV21_TYPE ,FRAME_NUM - shm_ptr->sem.NV21_Avail_Buf);
    AvailBGRAPollAddr = Get_Frame_Data_Offset(shm_ptr,BGRA_TYPE ,shm_ptr->sem.Display_Avail_Buf);
    switch(PollType)
    {
        case NV21_TYPE:
            return AvailNV21PollAddr; break;
        case BGRA_TYPE:
            return AvailBGRAPollAddr; break;
        break;
    }
    
}