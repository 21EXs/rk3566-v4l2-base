#include<stdio.h>
#include "v4l2_app.h"

#define DEVICE_NAME "/dev/video0"
#define BUFFER_COUNT 8
#define VIDEO_MAX_PLANES 8 

struct buffer{
	struct v4l2_plane *planes;
	int n_planes;
	void **start;
	size_t *length;
};

struct buffer *buffers = NULL;
struct shared_memory *shm_ptr = NULL;
static int fd = -1;
int shm_fd;

void handle_error(const char* operation) 
{
    fprintf(stderr, "%s 失败: %s (错误码: %d)\n", 
            operation, strerror(errno), errno);
    exit(1);
}

int xioctl(int fh,int request,void *arg)
{
	int r;
	do{
		r = ioctl(fh,request,arg);
	}while(r == -1 && errno == EINVAL);
	return r;
}

void request_buffers()
{
	struct v4l2_requestbuffers req = {0};

	printf("1. 请求视频缓冲区...\n");

	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1)
	{
		fprintf(stderr, "缓冲区数量不足: %d\n", req.count);
        exit(1);
	}

    printf("   驱动实际分配了 %d 个缓冲区\n", req.count);

    // 分配缓冲区管理结构
    buffers = calloc(req.count, sizeof(struct buffer));
    if (!buffers)
	{
        handle_error("分配缓冲区管理结构");
    }
}

void query_and_map_buffers()
{
    printf("2. 查询并映射缓冲区...\n");
    
    for (int i = 0; i < BUFFER_COUNT; ++i) 
	{
        struct v4l2_buffer buf = {0};
        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {0};
        
        // 设置多平面缓冲区
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = VIDEO_MAX_PLANES;  // 最大平面数
        buf.m.planes = planes;         // 平面数组
        
        // 查询缓冲区信息
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) 
		{
            handle_error("VIDIOC_QUERYBUF");
        }
        
        printf("   缓冲区 %d: %d个平面\n", i, buf.length);
        
        // 为这个缓冲区分配管理结构
        buffers[i].n_planes = buf.length;
        buffers[i].planes = malloc(buf.length * sizeof(struct v4l2_plane));
        buffers[i].start = malloc(buf.length * sizeof(void*));
        buffers[i].length = malloc(buf.length * sizeof(size_t));
        
        if (!buffers[i].planes || !buffers[i].start || !buffers[i].length) 
		{
            handle_error("分配平面管理结构");
        }
        
        // 映射每个平面
        for (int j = 0; j < buf.length; ++j) 
		{
            buffers[i].planes[j] = planes[j];  // 保存平面信息
            
            printf("     平面 %d: 长度=%u, 偏移=%u\n", 
                   j, planes[j].length, planes[j].m.mem_offset);
            
            // 内存映射
            buffers[i].start[j] = mmap(NULL, planes[j].length,
                                     PROT_READ | PROT_WRITE, MAP_SHARED,
                                     fd, planes[j].m.mem_offset);

            if (buffers[i].start[j] == MAP_FAILED) 
			{
                handle_error("mmap");
            }
            
            buffers[i].length[j] = planes[j].length;
            
            printf("       映射到地址: %p\n", buffers[i].start[j]);
        }
    }
    printf("   所有缓冲区映射完成！\n");
}

void queue_all_buffers()
{
	printf("3. 将缓冲区加入采集队列...\n");
	for(int i = 0; i < BUFFER_COUNT; ++i)
	{
		struct v4l2_buffer buf = {0};
		struct v4l2_plane planes[VIDEO_MAX_PLANES] = {0};
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.length = 1;
		buf.m.planes = planes;

		if(xioctl(fd,VIDIOC_QBUF,&buf))
		{
			handle_error("VIDIOC_QBUF");
		}
		printf("   缓冲区 %d 已加入队列\n", i);
	}
	 printf(" 所有缓冲区已加入采集队列\n");
}

void start_streaming()
{
	printf("4. 启动视频流...\n");
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if(xioctl(fd,VIDIOC_STREAMON,&type) == -1)
	{
		handle_error("VIDIOC_STREAMON");
	}
	printf("视频流已启动，开始采集图像...\n");

	usleep(500000); // 等待500ms
}

void save_frame_to_file(int buf_index, size_t data_size, int frame_count)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "frame_%d.yuv", frame_count);
    
    FILE *file = fopen(filename, "wb");
    if (!file)
	{
        perror("无法创建文件");
        return;
    }
    
    // 将图像数据写入文件
    size_t written = fwrite(buffers[buf_index].start[0], 1, data_size, file);
    fclose(file);
    
    printf("     已保存为: %s (%zu 字节)\n", filename, written);
}


struct shared_memory* Frame_Shm()//获取指向加上NV21偏移量的地址的指针，不是获取共享内存的指针！！！
{
    size_t nv21_data_size = NV21_SIZE(WIDTH, HEIGHT);
    size_t argb_data_size = ARGB_SIZE(WIDTH, HEIGHT);
    /* Shm_Create 为 FRAME_NUM 帧分配了空间，这里需要映射相同的总大小 */
    size_t total_size = sizeof(struct shared_memory) + (nv21_data_size + argb_data_size) * FRAME_NUM;

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
    printf("NV21数据偏移: %u\n", shm_ptr->nv21.data_offset[0]);
    printf("ARGB数据偏移: %u\n", shm_ptr->argb.data_offset[0]);

	return shm_ptr;
}

int Write_Frame_Shm(struct shared_memory* shm, uint8_t* v4l2_buffer, size_t size)
{
	if (!shm || !v4l2_buffer) 
	{
        printf("错误: 无效参数\n");
        return -1;
    }

	// 保护访问（获取信号量）
    sem_wait(&shm->sem.display_done);
	
    //  获取NV21数据指针
    uint8_t* nv21_data = Get_Frame_Data_Offset(shm,NV21_TYPE ,shm->sem.NV21_Avail_Buf);
    UpdatePollID(NV21_TYPE);
    if (!nv21_data) 
	{
        // sem_post(&shm->sem.display_done);
        return -1;
    }

    memcpy(nv21_data, v4l2_buffer , size);
	// printf("     已写入共享内存: %zu 字节\n", size);

	// // 4. 更新元数据
    // shm->nv21.meta.is_valid = 1;

	// 操作数据量代表可用buff
    sem_post(&shm->sem.capture_done); 
    
	return 0;
}

void save_frame_to_shm(int buf_index, size_t data_size)
{
    // printf("帧 %d: 写入共享内存\n", frame_count);
	if (!shm_ptr) 
	{
        printf("错误：共享内存指针为空\n");
        return;
    }
    // 调用写入函数
    int ret = Write_Frame_Shm(shm_ptr, buffers[buf_index].start[0], data_size);
    if (ret != 0) 
	{
        printf("     写入共享内存失败\n");
    } 
	// else 
	// {
    //     printf("     已写入共享内存: %zu 字节\n", data_size);
    // }
}

void capture_frames()
{
	printf("5. 开始采集图像帧（采集5帧后停止）...\n");

    struct v4l2_buffer buf = {0};
    struct v4l2_plane planes[VIDEO_MAX_PLANES] = {0};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = 1;
    buf.m.planes = planes;
    while(1)
    {
        if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1)
        {
            handle_error("VIDIOC_DQBUF");
        }
        save_frame_to_shm(buf.index, buf.m.planes[0].bytesused);
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            handle_error("重新QBUF");
        }
    }
}

void v4l2_start()
{
    printf("=== V4L2 多平面缓冲区设置 ===\n");
	
	shm_ptr = Shm_Open();
    if (!shm_ptr) 
    {
        fprintf(stderr, "错误：共享内存映射失败，v4l2进程无法继续\n");
        return;  // 或者exit(1)
    }

	printf("打开设备 %s...\n", DEVICE_NAME);
	// fd = open(DEVICE_NAME,O_RDWR | O_NONBLOCK);
    fd = open(DEVICE_NAME,O_RDWR);
	if(fd == -1)
	{
		handle_error("打开设备");
	}
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = 640;
    fmt.fmt.pix_mp.height = 480;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21;
	
	if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
	{
        handle_error("设置格式");
    }

	printf("格式设置成功: 640x480 NV21\n");

	request_buffers();

	query_and_map_buffers();

	// 阶段三：启动采集
    queue_all_buffers();    // 将缓冲区加入队列
    start_streaming();      // 启动视频流
    capture_frames();       // 采集图像
    
    // 阶段四：停止采集并清理资源
//    stop_streaming();
//   cleanup_resources();
    
    printf("\n图像采集完成！\n");
}
