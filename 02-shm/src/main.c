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
    
    // 使用 RGA 硬件加速：NV21 → BGRA8888
    if (RGA_NV21_To_BGRA(nv21_data, bgra_data, WIDTH, HEIGHT) != 0) 
    {
        fprintf(stderr, "[RGA] 转换失败！请检查 /dev/rga 设备\n");
    }
    
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
