#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define DEVICE_NAME "/dev/video0"
#define BUFFER_COUNT 4

struct buffer{
	struct v4l2_plane *planes;
	int n_planes;
	void **start;
	size_t *length;
};

static struct buffer *buffers = NULL;
static int fd = -1;

// 错误处理函数
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

void query_and_map_buffers() {
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
            
            printf("     平面 %d: 长度=%zu, 偏移=%u\n", 
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

int main()
{
	printf("=== V4L2 多平面缓冲区设置 ===\n");
	
	printf("打开设备 %s...\n", DEVICE_NAME);
	fd = open(DEVICE_NAME,O_RDWR | O_NONBLOCK);
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

	return 0;
}
