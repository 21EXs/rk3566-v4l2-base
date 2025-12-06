#include "../inc/shm.h"

void Shm_Create()
{
    int shm_fd;
    struct shared_data *shm_ptr;
    
    printf("=== 生产者进程 ===\n");
    
    // 1. 创建共享内存对象
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) 
    {
        perror("shm_open失败");
        return 1;
    }
    
    // 2. 设置共享内存大小
    if (ftruncate(shm_fd, sizeof(struct shared_data)) == -1) 
    {
        perror("ftruncate失败");
        close(shm_fd);
        return 1;
    }
    
    // 3. 将共享内存映射到进程地址空间
    shm_ptr = mmap(NULL, sizeof(struct shared_data), 
                   PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) 
    {
        perror("mmap失败");
        close(shm_fd);
        return 1;
    }
    
    // 4. 写入固定字符串
    printf("正在写入数据到共享内存...\n");
    strncpy(shm_ptr->text, "Hello World!", MAX_TEXT_SIZE - 1);
    shm_ptr->text[MAX_TEXT_SIZE - 1] = '\0';  // 确保以null结尾
    shm_ptr->is_data_ready = 1;
    
    printf("已写入: %s\n", shm_ptr->text);
    printf("数据已就绪，等待消费者读取...\n");
    
    // 5. 等待消费者读取
    while (shm_ptr->is_data_ready == 1) 
    {
        sleep(1);  // 每秒检查一次
    }
    
    printf("消费者已读取数据，生产者退出。\n");
    
    // 6. 清理资源
    munmap(shm_ptr, sizeof(struct shared_data));
    close(shm_fd);
    
    return 0;
}