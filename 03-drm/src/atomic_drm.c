#include "atomic_drm.h"

static struct shared_memory *shm_ptr = NULL;
int shm_fd;

int create_test_pattern(uint32_t *buffer, int width, int height)
{
  for(uint16_t y = 0;y < height;y++)
  {
    for (uint16_t x = 0; x < width; x++) 
    {
    //   uint8_t r = (x * 255 / width) & 0xFF;
    //   uint8_t g = (y * 255 / height) & 0xFF;
    //   uint8_t b = 128;
      uint8_t r = 0;
      uint8_t g = 100;
      uint8_t b = 255;
      buffer[y * width + x] = (r << 16) | (g << 8) | b;
    }
  }
}

struct shared_memory* Drm_Shm()
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

int init_drm_device(struct drm_device *dev)
{
    dev->fd = drmOpen("rockchip", NULL);
    if (dev->fd < 0) 
    {
        printf("无法打开DRM设备\n");
        return -1;
    }

    dev->res = drmModeGetResources(dev->fd);
    if (!dev->res) 
    {
        printf("无法获取DRM资源\n");
        return -1;
    }

    // 查找可用的connector
    for (int i = 0; i < dev->res->count_connectors; i++) 
    {
        dev->connector = drmModeGetConnector(dev->fd, dev->res->connectors[i]);
        if (dev->connector && dev->connector->connection == DRM_MODE_CONNECTED && 
            dev->connector->count_modes > 0) 
        {
            dev->conn_id = dev->connector->connector_id;
            // 使用第一个可用模式
            dev->mode = dev->connector->modes[0];
            break;
        }
        if (dev->connector) 
        {
            drmModeFreeConnector(dev->connector);
            dev->connector = NULL;
        }
    }

    if (!dev->connector) 
    {
        printf("未找到可用的显示连接器\n");
        return -1;
    }

    // 查找可用的CRTC
    for (int i = 0; i < dev->res->count_encoders; i++) 
    {
        drmModeEncoder *encoder = drmModeGetEncoder(dev->fd, dev->res->encoders[i]);
        if (encoder) 
        {
            if (encoder->crtc_id) 
            {
                dev->crtc_id = encoder->crtc_id;
                drmModeFreeEncoder(encoder);
                break;
            }
            drmModeFreeEncoder(encoder);
        }
    }

    if (!dev->crtc_id) 
    {
        printf("未找到可用的CRTC\n");
        // 尝试使用第一个可用的CRTC
        if (dev->res->count_crtcs > 0) 
        {
            dev->crtc_id = dev->res->crtcs[0];
        }
        else 
        {
            return -1;
        }
    }

    return 0;
}

int create_framebuffer(struct drm_device *dev, int width, int height)
{
  struct drm_mode_create_dumb create = {0};
  struct drm_mode_map_dumb map = {0};

  create.width = width;
  create.height = height;
  create.bpp = 32;  // 32位色深（ARGB8888）

  if (drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) 
  {
    printf("创建dumb buffer失败\n");
    return -1;
  }

  dev->fb_handle = create.handle;
  dev->fb_size = create.size;
  dev->fb_pitch = create.pitch;

  if (drmModeAddFB(dev->fd, width, height, 24, 32, dev->fb_pitch, 
                    dev->fb_handle, &dev->fb_id) < 0)
  {
      printf("创建帧缓冲失败\n");
      return -1;
  }
  
  map.handle = dev->fb_handle;
  if (drmIoctl(dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) 
  {
      printf("映射buffer失败\n");
      return -1;
  }
  
  dev->fb_data = mmap(0, dev->fb_size, PROT_READ | PROT_WRITE, 
                      MAP_SHARED, dev->fd, map.offset);
  if (dev->fb_data == MAP_FAILED) 
  {
      printf("内存映射失败\n");
      return -1;
  }
  
  return 0;
}

int drm_start()
{
    struct drm_device my_dev;
    memset(&my_dev, 0, sizeof(struct drm_device));
    
    system("killall weston 2>/dev/null");
    sleep(1);
    
    if (init_drm_device(&my_dev) < 0) 
    {
        printf("DRM设备初始化失败\n");
        return -1;
    }

    // 获取屏幕分辨率
    int screen_width = my_dev.mode.hdisplay;
    int screen_height = my_dev.mode.vdisplay;
    
    // 从共享内存获取图像尺寸
    struct shared_memory* shm = Drm_Shm();
    if (shm == NULL) 
    {
        fprintf(stderr, "无法获取共享内存\n");
        return -1;
    }
    
    // 从共享内存的元数据中获取图像尺寸
    int img_width = WIDTH;
    int img_height = HEIGHT;
    
    if (create_framebuffer(&my_dev, screen_width, screen_height) < 0) 
    {
        printf("帧缓冲创建失败\n");
        return -1;
    }

    // 获取ARGB数据的指针
    uint8_t* argb_data_ptr = Get_ARGB_Data(shm);
    
    if (argb_data_ptr == NULL) {
        fprintf(stderr, "无法获取ARGB数据指针\n");
        return -1;
    }
    
    // 清空屏幕为黑色
    memset(my_dev.fb_data, 0, my_dev.fb_size);
    
    // 计算居中位置
    int start_x = (screen_width - img_width) / 2;
    int start_y = (screen_height - img_height) / 2;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    printf("图像显示位置: (%d, %d)\n", start_x, start_y);
    
    // 直接将ARGB数据居中拷贝到帧缓冲
    uint8_t* src = argb_data_ptr;
    uint8_t* dst = (uint8_t*)my_dev.fb_data;
    for (int y = 0; y < img_height && (start_y + y) < screen_height; y++)
    {
        int src_index = y * img_width * 4;
        int dst_index = ((start_y + y) * screen_width + start_x) * 4;
        
        // 直接拷贝一行像素（BGRA格式，4字节/像素）
        memcpy(dst + dst_index, src + src_index, img_width * 4);
    }
    // 显示
    if (drmModeSetCrtc(my_dev.fd, my_dev.crtc_id, my_dev.fb_id, 
                       0, 0, &my_dev.conn_id, 1, &my_dev.mode) < 0) 
    {
        printf("显示启动失败: %s\n", strerror(errno));
        return -1;
    }
    
    printf("显示已启动，按Ctrl+C退出\n");
    
    // 保持显示直到被中断
    while (1) 
    {
        sleep(1);
    }
    
    // 清理资源
    if (my_dev.fb_data) 
    {
        munmap(my_dev.fb_data, my_dev.fb_size);
    }
    if (my_dev.fb_id) 
    {
        drmModeRmFB(my_dev.fd, my_dev.fb_id);
    }
    if (my_dev.res) 
    {
        drmModeFreeResources(my_dev.res);
    }
    if (my_dev.connector) 
    {
        drmModeFreeConnector(my_dev.connector);
    }
    if (my_dev.fd >= 0) 
    {
        close(my_dev.fd);
    }
    
    return 0;
}