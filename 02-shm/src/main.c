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

int NV21_To_ARGB888(unsigned char* input, unsigned char* output, int width, int height) 
{
    if (width < 1 || height < 1 || input == NULL || output == NULL)
        return 0;
    // bit depth
    int depth = 3;
    int nvOff = width * height;
    int i, j, yIndex = 0;
    int y, u, v;
    int r, g, b, nvIndex = 0;
    unsigned char* yuvData = input;
    unsigned char* rgbData = output;
    for (i = 0; i < height; i++) 
    {
        for (j = 0; j < width; j++, ++yIndex) 
        {
            nvIndex = (i / 2) * width + j - j % 2;
            y = yuvData[yIndex] & 0xff;
            v = yuvData[nvOff + nvIndex] & 0xff;
            u = yuvData[nvOff + nvIndex + 1] & 0xff;
 
            // yuv to rgb
            r = y + ((351 * (v - 128)) >> 8);  //r
            g = y - ((179 * (v - 128) + 86 * (u - 128)) >> 8); //g
            b = y + ((443 * (u - 128)) >> 8); //b
 
            r = ((r > 255) ? 255 : (r < 0) ? 0 : r);
            g = ((g > 255) ? 255 : (g < 0) ? 0 : g);
            b = ((b > 255) ? 255 : (b < 0) ? 0 : b);
            // RGB格式的图像存储的顺序，并非像字面的顺序，而是以：B、G、R的顺序进行存储。
            *(rgbData + yIndex * depth + 0) = b;
            *(rgbData + yIndex * depth + 1) = g;
            *(rgbData + yIndex * depth + 2) = r;
 
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
    NV21_To_ARGB888(nv21_data, argb888_data, WIDTH, HEIGHT);
    printf("已完成nv21转化为argb888格式 \n");
    sleep(2);
}

int main() 
{
    shm_ptr = Shm_Shm();
    if (!shm_ptr) 
    {
        fprintf(stderr, "错误：共享内存映射失败，共享内存（图像处理）进程无法继续\n");
        return;  
    }
    // while(1)
    Take_ARGB_Shm(shm_ptr);

}