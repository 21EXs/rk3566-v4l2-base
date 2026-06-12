#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "main.h"
#include <shm.h>
#include <pthread.h>
#include "MPP_Wrapper.h"
#include "Socket_Sever.h"
#include "rga_convert.h"

void* thread1_function(void* arg);

static struct buffer *buffers = NULL;
struct shared_memory *shm_ptr = NULL;
static int fd = -1;
int shm_fd;
int g_fd_h264; 

void Take_ARGB_Shm(struct shared_memory* shm)
{
    sem_wait(&shm_ptr->sem.capture_done);
    
    uint8_t* nv21_data = Get_Frame_Data_Offset(shm,NV21_TYPE ,shm->sem.Convert_Avail_Buf);
    uint8_t* bgra_data = Get_Frame_Data_Offset(shm,BGRA_TYPE ,shm->sem.Convert_Avail_Buf);
    
    // malloc 临时 buffer（RGA 对堆内存的 MMU 映射通常没问题）
    // 注意：旋转90度后，BGRA 数据变为 HEIGHT × WIDTH
    unsigned char* tmp_nv21 = (unsigned char*)malloc(NV21_SIZE(WIDTH, HEIGHT));
    unsigned char* tmp_bgra = (unsigned char*)malloc(ARGB_SIZE(HEIGHT, WIDTH));  // 旋转后宽高互换
    if (!tmp_nv21 || !tmp_bgra) 
    {
        fprintf(stderr, "[RGA] malloc 临时 buffer 失败\n");
        if (tmp_nv21) free(tmp_nv21);
        if (tmp_bgra) free(tmp_bgra);
        UpdatePollID(CONVERT_TYPE);
        sem_post(&shm->sem.convert_done);
        return;
    }
    
    // 从共享内存拷到临时 buffer
    memcpy(tmp_nv21, nv21_data, NV21_SIZE(WIDTH, HEIGHT));
    
    // 使用 RGA 硬件加速：NV21 → BGRA8888 + 旋转90度（适配竖屏 720×1280）
    // 一步完成格式转换和旋转，输出为 HEIGHT × WIDTH = 720 × 1280
    if (RGA_NV21_To_BGRA_Rotate90(tmp_nv21, tmp_bgra, WIDTH, HEIGHT) != 0) 
    {
        fprintf(stderr, "[RGA] 转换+旋转失败，尝试不旋转仅转换...\n");
        // 回退：仅转换不旋转
        if (RGA_NV21_To_BGRA(tmp_nv21, tmp_bgra, WIDTH, HEIGHT) != 0)
        {
            fprintf(stderr, "[RGA] 转换也失败！直接拷贝原始数据\n");
            memcpy(bgra_data, tmp_nv21, NV21_SIZE(WIDTH, HEIGHT));
        }
        else
        {
            // 仅转换成功，但未旋转，数据是 WIDTH × HEIGHT
            memcpy(bgra_data, tmp_bgra, ARGB_SIZE(WIDTH, HEIGHT));
        }
    }
    else
    {
        // 转换+旋转成功，数据是 HEIGHT × WIDTH = 720 × 1280
        memcpy(bgra_data, tmp_bgra, ARGB_SIZE(HEIGHT, WIDTH));
    }
    
    free(tmp_nv21);
    free(tmp_bgra);
    
    UpdatePollID(CONVERT_TYPE);

    sem_post(&shm->sem.convert_done); 
}

int main() 
{
    pid_t encode_pid = fork();

    shm_ptr = Shm_Open();
    if (!shm_ptr) 
    {
        fprintf(stderr, "错误：共享内存映射失败，共享内存（图像处理）进程无法继续\n");
        return 0;  
    }

    if (encode_pid == 0) 
    {
        static uint8_t EncodeFlag = 0;
        int frame_count = 0;
        time_t last_print_time = time(NULL);
        H264Encoder *enc = H264Encoder_Init(WIDTH, HEIGHT, "/mnt/output.h264");
        
        while(1) 
        {
            if(EncodeFlag != shm_ptr->sem.BGRA_Avail_Buf)
            {
                uint8_t* nv21_data = Get_Frame_Data_Offset(shm_ptr,NV21_TYPE ,shm_ptr->sem.BGRA_Avail_Buf);
                int ret = H264Encoder_EncodeFrame(enc, nv21_data);
                if (ret != 0) 
                {
                    printf("编码失败: %d\n", ret);
                }

                frame_count++;
                EncodeFlag = shm_ptr->sem.BGRA_Avail_Buf;
            }
            else
            {
                usleep(10);
            }
        }
    }
    else if (encode_pid > 0)//子进程socket和父进程
    {
        // 将共享内存指针传递给 socket 模块（fork 后子进程继承 mmap，指针仍然有效）
        Socket_SetShmPtr(shm_ptr);

        // 父进程中再次fork创建socket监听子进程
        pid_t socket_pid = fork();
        
        if (socket_pid == 0) //子进程socket
        {
            // 这是socket监听子进程
            // socket服务器初始化代码（根据实际需求实现）
            Socket_Server_Init() ;
            while(1)
            {
                if (Socket_Server_Listen() == -1) 
                {
                    // 可以选择继续等待或退出
                    sleep(1);
                    continue;
                }
                Socket_Server_ProcessClient();
            }
            Socket_Server_CloseClient();
            Socket_Server_Shutdown();
            exit(0);
        }
        else if (socket_pid > 0)
        {
            // 父进程
            while(1)
            {
                Take_ARGB_Shm(shm_ptr);
            }
        }
        else
        {
            perror("socket子进程fork失败");
        }
    }
    else
    {
        perror("编码子进程fork失败");
    }
    
    return 0;  
}
