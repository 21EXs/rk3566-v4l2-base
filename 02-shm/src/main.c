#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "main.h"

int main() {
    int shm_fd;
    struct shared_data *shm_ptr;
    
    printf("=== 消费者进程 ===\n");
    printf("正在连接共享内存...\n");
    
    // 1. 打开已存在的共享内存对象
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) 
    {
        perror("shm_open失败");
        printf("请先运行生产者进程！\n");
        return 1;
    }
    
    // 2. 将共享内存映射到进程地址空间
    shm_ptr = mmap(NULL, sizeof(struct shared_data), 
                   PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) 
    {
        perror("mmap失败");
        close(shm_fd);
        return 1;
    }
    
    printf("成功连接到共享内存！\n");
    printf("等待生产者写入数据...\n");
    
    // 3. 等待生产者写入数据
    while (shm_ptr->is_data_ready == 0) 
    {
        printf("等待中...\n");
        sleep(1);  // 每秒检查一次
    }
    
    // 4. 读取数据
    printf("收到消息: %s\n", shm_ptr->text);
    
    // 5. 标记数据已读取
    shm_ptr->is_data_ready = 0;
    printf("已标记数据为已读取\n");
    
    // 6. 清理资源
    munmap(shm_ptr, sizeof(struct shared_data));
    close(shm_fd);
    
    printf("消费者退出。\n");
    return 0;
}