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

#define SHM_NAME "/shm_video_data"
#define MAX_TEXT_SIZE 1024

#define WIDTH  640 
#define HEIGHT 480

#define NV21_TYPE 0
#define BGRA_TYPE 1
#define CONVERT_TYPE 2
#define FRAME_NUM 3

#define NV21_SIZE(WIDTH, HEIGHT) ((WIDTH) * (HEIGHT) * 3 / 2)
#define ARGB_SIZE(WIDTH, HEIGHT) ((WIDTH) * (HEIGHT) * 4)

// 定义嵌套结构体
struct image_metadata {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint8_t is_valid;
};

// 定义完整的主结构体
struct shared_memory {
    // NV21部分
    struct {
        struct image_metadata meta;
        uint32_t data_offset[FRAME_NUM];  // 从共享内存开始的偏移
    } nv21;
    
    // ARGB部分
    struct {
        struct image_metadata meta;
        uint32_t data_offset[FRAME_NUM];
    } argb;

    struct {
        // 采集 → 转换
        sem_t capture_done;    // 信号量A: 采集完成
        // 转换 → 显示
        sem_t convert_done;    // 信号量B: 转换完成
        // 显示 → 采集
        sem_t display_done;    // 信号量C: 显示完成

        uint8_t NV21_Avail_Buf;
        uint8_t Convert_Avail_Buf;
        uint8_t BGRA_Avail_Buf;


    } sem;
    
    // 控制信息
    uint32_t ref_count;
    uint8_t is_initialized;
    
    // 柔性数组
    uint8_t data_pool[0];
};

static inline uint8_t* Get_NV21_Data(struct shared_memory* shm) 
{
    if (!shm) 
        return NULL;
    return (uint8_t*)shm + shm->nv21.data_offset[0];
}

static inline uint8_t* Get_ARGB_Data(struct shared_memory* shm) 
{
    if (!shm) 
        return NULL;
    return (uint8_t*)shm + shm->argb.data_offset[0];
}

static inline uint8_t* Get_Frame_Data_Offset(struct shared_memory* shm,uint8_t Frame_Type ,uint8_t Frame_ID) 
{
    size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
    if (!shm) 
        return NULL;
    
    if(Frame_Type == NV21_TYPE)
    {
        return (uint8_t*)shm + sizeof(struct shared_memory) + nv21_data_size * (Frame_ID) + argb_data_size * (Frame_ID);
    }
    else if(Frame_Type == BGRA_TYPE)
    {
        return (uint8_t*)shm + sizeof(struct shared_memory) + nv21_data_size * (Frame_ID + 1) + argb_data_size * (Frame_ID );
    }
    else;
}

void Shm_Create();
struct shared_memory* Shm_Open();  
void UpdatePollID(uint8_t PollType);
#endif


/*
共享内存布局（总大小 = 元数据 + NV21数据 + ARGB数据）：
+--------------------------------+ ← 基地址 (base_ptr)
| 元数据 (struct metadata)        |
| - width, height                |
| - offsets, sizes               |
| - 状态标志                      |
| - 信号量                      | 
+--------------------------------+ ← base_ptr + sizeof(struct metadata)
| NV21 数据区 (460800字节)        |
| Y分量: 640×480 = 307200字节     |
| UV分量: 640×240 = 153600字节    |
+--------------------------------+ ← base_ptr + sizeof(metadata) + 460800
| ARGB 数据区 (1228800字节)       |
| 每个像素4字节: 640×480×4       |
+--------------------------------+ ← base_ptr + sizeof(struct metadata)
| NV21 数据区 (460800字节)        |
| Y分量: 640×480 = 307200字节     |
| UV分量: 640×240 = 153600字节    |
+--------------------------------+ ← base_ptr + sizeof(metadata) + 460800
| ARGB 数据区 (1228800字节)       |
| 每个像素4字节: 640×480×4       |
+--------------------------------+ ← base_ptr + sizeof(struct metadata)
| NV21 数据区 (460800字节)        |
| Y分量: 640×480 = 307200字节     |
| UV分量: 640×240 = 153600字节    |
+--------------------------------+ ← base_ptr + sizeof(metadata) + 460800
| ARGB 数据区 (1228800字节)       |
| 每个像素4字节: 640×480×4       |
+--------------------------------+
*/ 