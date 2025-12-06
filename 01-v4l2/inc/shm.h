#ifndef __SHM_H
#define __SHM_H

#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// 共享内存和信号量名称
#define SHM_NAME "/shm_video_data"
#define MAX_TEXT_SIZE 1024

// 简单的共享数据结构
struct shared_data {
    char text[MAX_TEXT_SIZE];  // 存储字符串
    int is_data_ready;         // 数据就绪标志：0=无数据，1=有数据
};

void Shm_Create();

#endif