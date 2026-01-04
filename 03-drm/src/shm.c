#include "../inc/shm.h"

// 全局变量，避免重复 mmap
static struct shared_memory *g_shm_ptr = NULL;
static size_t g_total_size = 0;

void Shm_Create()
{
    size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);

    // 计算总大小
    g_total_size = sizeof(struct shared_memory) + (nv21_data_size + argb_data_size) * FRAME_NUM;
    
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open failed");
        return;
    }
    
    if (ftruncate(shm_fd, g_total_size) < 0) {
        perror("ftruncate failed");
        close(shm_fd);
        return;
    }

    g_shm_ptr = mmap(NULL, g_total_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);  // mmap 后可以关闭文件描述符
    
    if (g_shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        g_shm_ptr = NULL;
        return;
    }

    // 清零共享内存
    memset(g_shm_ptr, 0, sizeof(struct shared_memory));

    // 初始化所有缓冲区的偏移
    for (int i = 0; i < FRAME_NUM; i++) {
        g_shm_ptr->nv21.data_offset[i] = sizeof(struct shared_memory) + 
                                        (nv21_data_size + argb_data_size) * i;
        g_shm_ptr->argb.data_offset[i] = g_shm_ptr->nv21.data_offset[i] + nv21_data_size;
    }

    // 设置元数据
    g_shm_ptr->nv21.meta.width = WIDTH;
    g_shm_ptr->nv21.meta.height = HEIGHT;
    g_shm_ptr->nv21.meta.format = 0;  // NV21
    g_shm_ptr->nv21.meta.is_valid = 0;

    g_shm_ptr->argb.meta.width = WIDTH;
    g_shm_ptr->argb.meta.height = HEIGHT;  
    g_shm_ptr->argb.meta.format = 1;  // ARGB8888
    g_shm_ptr->argb.meta.is_valid = 0;

    // 初始化信号量（第二个参数1表示进程间共享）
    sem_init(&g_shm_ptr->sem.capture_done, 1, 0);
    sem_init(&g_shm_ptr->sem.convert_done, 1, 0);
    sem_init(&g_shm_ptr->sem.display_done, 1, FRAME_NUM);
    g_shm_ptr->sem.NV21_Avail_Buf = 0;
    g_shm_ptr->sem.Convert_Avail_Buf = 0;
    g_shm_ptr->sem.BGRA_Avail_Buf = 0;

    g_shm_ptr->is_initialized = 1;

    printf("=== 共享内存布局 ===\n");
    printf("结构体基地址: %p\n", g_shm_ptr);
    printf("结构体大小: %zu 字节\n", sizeof(struct shared_memory));
    printf("NV21数据大小: %zu 字节\n", nv21_data_size);
    printf("ARGB数据大小: %zu 字节\n", argb_data_size);
    printf("总大小: %zu 字节\n", g_total_size);
    
    for (int i = 0; i < FRAME_NUM; i++) {
        printf("缓冲区 %d: NV21偏移=0x%08X, BGRA偏移=0x%08X\n", 
               i, g_shm_ptr->nv21.data_offset[i], g_shm_ptr->argb.data_offset[i]);
    }
    printf("==================\n");

    printf("共享内存创建、初始化成功\n");
}

struct shared_memory* Shm_Open(void)
{
    if (g_shm_ptr != NULL) {
        return g_shm_ptr;  // 已经打开过了
    }
    
    size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
    g_total_size = sizeof(struct shared_memory) + 
                  (nv21_data_size + argb_data_size) * FRAME_NUM;
    
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) 
    {
        perror("shm_open failed");
        return NULL;
    }
    
    g_shm_ptr = mmap(NULL, g_total_size, PROT_READ|PROT_WRITE, 
                     MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (g_shm_ptr == MAP_FAILED) 
    {
        perror("mmap failed");
        g_shm_ptr = NULL;
        return NULL;
    }
    
    return g_shm_ptr;
}

void UpdatePollID(uint8_t PollType)
{
    // 使用全局指针，避免每次都调用 Shm_Open
    if (g_shm_ptr == NULL) {
        g_shm_ptr = Shm_Open();
    }
    
    struct shared_memory *shm_ptr = g_shm_ptr;
    if (!shm_ptr) {
        printf("错误: 无法获取共享内存指针\n");
        return NULL;
    }
    
    uint8_t* result = NULL;
    
    if (PollType == NV21_TYPE) 
    {
        g_shm_ptr->sem.NV21_Avail_Buf++;
        if(g_shm_ptr->sem.NV21_Avail_Buf > 2)
            g_shm_ptr->sem.NV21_Avail_Buf = 0;
    } 
    else if (PollType == BGRA_TYPE) 
    {
        g_shm_ptr->sem.BGRA_Avail_Buf++;
        if(g_shm_ptr->sem.BGRA_Avail_Buf > 2)
            g_shm_ptr->sem.BGRA_Avail_Buf = 0;
    } 
    else if(CONVERT_TYPE)
    {
        g_shm_ptr->sem.Convert_Avail_Buf++;
        if(g_shm_ptr->sem.Convert_Avail_Buf > 2)
            g_shm_ptr->sem.Convert_Avail_Buf = 0;
    }
    else 
    {
        printf("错误: 未知的PollType=%d\n", PollType);
    }
}