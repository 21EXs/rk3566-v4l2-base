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

void* thread1_function(void* arg);

static struct buffer *buffers = NULL;
struct shared_memory *shm_ptr = NULL;
static int fd = -1;
int shm_fd;
int g_fd_h264; 

int NV21_To_BGRA(unsigned char* input, unsigned char* output, int width, int height) 
{
    if (width < 1 || height < 1 || input == NULL || output == NULL)
        return 0;
    
    int frame_size = width * height;
    int uv_offset = frame_size;
    
    // 使用字节指针
    uint8_t* rgba = (uint8_t*)output;
    
    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            int y_index = y * width + x;
            
            // NV21 UV索引计算
            int uv_index = (y/2) * width + (x/2) * 2;
            
            int y_val = input[y_index];
            int v_val = input[uv_offset + uv_index];      // V分量
            int u_val = input[uv_offset + uv_index + 1];  // U分量
            
            // YUV to RGB转换
            int c = y_val - 16;
            int d = u_val - 128;
            int e = v_val - 128;
            
            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;
            
            r = (r > 255) ? 255 : (r < 0) ? 0 : r;
            g = (g > 255) ? 255 : (g < 0) ? 0 : g;
            b = (b > 255) ? 255 : (b < 0) ? 0 : b;
            
            int pixel_index = y_index * 4;
            
            // 修正：改为RGBA格式（红色在前，蓝色在后）
            rgba[pixel_index] = r;         // Red
            rgba[pixel_index + 1] = g;     // Green
            rgba[pixel_index + 2] = b;     // Blue
            rgba[pixel_index + 3] = 0xFF;  // Alpha (不透明)
        }
    }
    
    return 1;
}

// int NV21_To_BGRA(unsigned char* input, unsigned char* output, int width, int height) 
// {
//     if (width < 1 || height < 1 || input == NULL || output == NULL)
//         return 0;
    
//     int frame_size = width * height;
//     int uv_offset = frame_size;
    
//     uint8_t* bgra = (uint8_t*)output;
    
//     for (int y = 0; y < height; y++) 
//     {
//         for (int x = 0; x < width; x++) 
//         {
//             int y_index = y * width + x;
            
//             int uv_index = (y/2) * width + (x/2) * 2;
            
//             int y_val = input[y_index];
//             int v_val = input[uv_offset + uv_index];      // V分量
//             int u_val = input[uv_offset + uv_index + 1];  // U分量
            
//             int c = y_val - 16;
//             int d = u_val - 128;
//             int e = v_val - 128;
            
//             int r = (298 * c + 409 * e + 128) >> 8;
//             int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
//             int b = (298 * c + 516 * d + 128) >> 8;
            
//             r = (r > 255) ? 255 : (r < 0) ? 0 : r;
//             g = (g> 255) ? 255 : (g < 0) ? 0 : g;
//             b = (b > 255) ? 255 : (b < 0) ? 0 : b;
            
//             int pixel_index = y_index * 4;
            
//             bgra[pixel_index] = b;         // Blue
//             bgra[pixel_index + 1] = g;     // Green
//             bgra[pixel_index + 2] = r;     // Red
//             bgra[pixel_index + 3] = 0xFF;  // Alpha (不透明)
//         }
//     }
    
//     return 1;
// }

void Take_ARGB_Shm(struct shared_memory* shm)
{
    sem_wait(&shm_ptr->sem.capture_done);
    
    uint8_t* nv21_data = Get_Frame_Data_Offset(shm,NV21_TYPE ,shm->sem.Convert_Avail_Buf);
    uint8_t* bgra_data = Get_Frame_Data_Offset(shm,BGRA_TYPE ,shm->sem.Convert_Avail_Buf);
    //转化
    NV21_To_BGRA(nv21_data, bgra_data, WIDTH, HEIGHT);
    UpdatePollID(CONVERT_TYPE);

    sem_post(&shm->sem.convert_done); 
    // printf("已完成nv21转化为argb888格式 \n");
    // usleep(10000);
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