// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/mman.h>
// #include "main.h"
// #include "atomic_drm.h"

// struct shared_memory *shm_ptr;
// int shm_fd;
// struct shared_memory* Drm_Shm()
// {
// 	if (shm_ptr != NULL) 
// 	{
//         return shm_ptr;
//     }

// 	size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
//     size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
// 	size_t total_size = sizeof(struct shared_memory) + nv21_data_size + argb_data_size;

// 	shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
//     if (shm_fd == -1) 
//     {
//         perror("shm_open失败");
//         return NULL;
//     }
	
// 	shm_ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
//     if (shm_ptr == MAP_FAILED) 
//     {
//         perror("mmap失败");
//         close(shm_fd);
//         return NULL;
//     }
	
// 	// close(shm_fd);

// 	printf("成功映射共享内存，大小: %zu 字节\n", total_size);
//     printf("NV21数据偏移: %u\n", shm_ptr->nv21.data_offset);
//     printf("ARGB数据偏移: %u\n", shm_ptr->argb.data_offset);

// 	 return shm_ptr;
// }

// int main() 
// {
//     shm_ptr = Drm_Shm();
//     if (!shm_ptr) 
//     {
//         printf("获取共享内存失败\n");
//         return -1;
//     }
//     printf("共享内存获取成功\n");

//     int ret = Drm_Show(shm_ptr);
//     if (ret < 0) 
//     {
//         printf("显示失败\n");
//         return -1;
//     }
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "disp_manager.h"
#include "drm_fourcc.h"

#include "atomic_drm.h"


int main()
{
    drm_start();
    while(1);
	// v4l2_start();
	return 0;
}
