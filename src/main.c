#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define DEVICE_NAME "/dev/video0"

// 错误处理函数
void handle_error(const char* operation) {
    fprintf(stderr, "%s 失败: %s (错误码: %d)\n", 
            operation, strerror(errno), errno);
    exit(1);
}

int main()
{
	int fd;
	struct v4l2_capability cap;

	fd = open(DEVICE_NAME,O_RDWR);
	if(fd == -1)
	{
		handle_error("打开设备");
	}
	printf("   设备打开成功！文件描述符: %d\n", fd);

	if(ioctl(fd,VIDIOC_QUERYCAP,&cap) == -1)
	{
		handle_error("VIDIOC_QUERYCAP");
	}
	printf("   设备能力查询成功！\n");

	    printf("驱动版本: %d.%d.%d\n",
           (cap.version >> 16) & 0xFF,
           (cap.version >> 8) & 0xFF,
           cap.version & 0xFF);
    
    // 设备能力标志
    printf("\n=== 设备能力 ===\n");
    printf("视频采集支持: %s\n", 
           (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? "是" : "否");
    printf("流式IO支持: %s\n", 
           (cap.capabilities & V4L2_CAP_STREAMING) ? "是" : "否");
    printf("读/写IO支持: %s\n",
           (cap.capabilities & V4L2_CAP_READWRITE) ? "是" : "否");

	printf("\n3. 关闭设备...\n");
    close(fd);
    printf("   设备关闭完成！\n");

    return 0;
}
