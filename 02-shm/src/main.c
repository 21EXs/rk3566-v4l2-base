#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "main.h"

static struct buffer *buffers = NULL;
static struct shared_memory *shm_ptr = NULL;
static int fd = -1;
int shm_fd;

int NV21_To_BGRA(unsigned char* input, unsigned char* output, int width, int height) 
{
    if (width < 1 || height < 1 || input == NULL || output == NULL)
        return 0;
    
    int frame_size = width * height;
    int uv_offset = frame_size;
    
    // 使用字节指针
    uint8_t* bgra = (uint8_t*)output;
    
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
            g = (g> 255) ? 255 : (g < 0) ? 0 : g;
            b = (b > 255) ? 255 : (b < 0) ? 0 : b;
            
            int pixel_index = y_index * 4;
            
            // 直接输出 DRM 需要的 BGRA 格式
            bgra[pixel_index] = b;         // Blue
            bgra[pixel_index + 1] = g;     // Green
            bgra[pixel_index + 2] = r;     // Red
            bgra[pixel_index + 3] = 0xFF;  // Alpha (不透明)
        }
    }
    
    return 1;
}
struct shared_memory* Shm_Shm()
{
	if (shm_ptr != NULL) 
	{
        return shm_ptr;
    }

	size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
	size_t total_size = sizeof(struct shared_memory) + nv21_data_size + argb_data_size;

	shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) 
    {
        perror("shm_open失败");
        return NULL;
    }
	
	shm_ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) 
    {
        perror("mmap失败");
        close(shm_fd);
        return NULL;
    }
	
	// close(shm_fd);

	printf("成功映射共享内存，大小: %zu 字节\n", total_size);
    printf("NV21数据偏移: %u\n", shm_ptr->nv21.data_offset);
    printf("ARGB数据偏移: %u\n", shm_ptr->argb.data_offset);

	 return shm_ptr;
}

void Take_ARGB_Shm(struct shared_memory* shm)
{
    uint8_t* nv21_data = Get_NV21_Data(shm);
    uint8_t* argb888_data = Get_ARGB_Data(shm);
    NV21_To_BGRA(nv21_data, argb888_data, WIDTH, HEIGHT);
    // printf("已完成nv21转化为argb888格式 \n");
    msleep(1000);
}

int main() 
{
    shm_ptr = Shm_Shm();
    if (!shm_ptr) 
    {
        fprintf(stderr, "错误：共享内存映射失败，共享内存（图像处理）进程无法继续\n");
        return 0;  
    }
    while(1)
    {
        sem_wait(&shm_ptr->sem.capture_done);
        Take_ARGB_Shm(shm_ptr);
        sem_post(&shm_ptr->sem.convert_done);
    }
    return 0;  

}