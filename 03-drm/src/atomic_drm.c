#include "atomic_drm.h"

extern struct shared_memory *shm_ptr;



int create_test_pattern(uint32_t *buffer, int width, int height)
{
  for(uint16_t y = 0;y < height;y++)
  {
    for (uint16_t x = 0; x < width; x++) 
    {
      uint8_t r = (x * 255 / width) & 0xFF;
      uint8_t g = (y * 255 / height) & 0xFF;
      uint8_t b = 128;
      buffer[y * width + x] = (r << 16) | (g << 8) | b;
    }
  }
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

int Drm_Show(struct shared_memory* shm)
{
    struct drm_device my_dev = {0};
    
    // 1. 强制停止 Weston 并防止重启
    printf("强制停止Weston显示服务...\n");
    system("killall -9 weston 2>/dev/null");
    system("systemctl stop weston 2>/dev/null");
    system("systemctl mask weston 2>/dev/null");  // 防止自动启动
    system("pkill -9 weston 2>/dev/null");
    sleep(3);
    
    // 2. 检查是否还有进程占用DRM
    printf("检查占用DRM的进程...\n");
    system("lsof /dev/dri/card0 2>/dev/null | head -20");
    system("fuser -v /dev/dri/card0 2>/dev/null");
    
    // 2. 初始化DRM设备
    if (init_drm_device(&my_dev) < 0) 
    {
        printf("DRM设备初始化失败\n");
        return -1;
    }
    
    // 4. 先禁用所有CRTC
    printf("禁用所有CRTC...\n");
    for (int i = 0; i < my_dev.res->count_crtcs; i++) 
    {
        uint32_t crtc_id = my_dev.res->crtcs[i];
        int ret = drmModeSetCrtc(my_dev.fd, crtc_id, 0, 0, 0, NULL, 0, NULL);
        if (ret < 0) 
        {
            printf("  禁用CRTC %u 失败: %s\n", crtc_id, strerror(errno));
        } 
        else 
        {
            printf("  已禁用CRTC %u\n", crtc_id);
        }
    }
    sleep(1);
    
    // 5. 清理所有未使用的framebuffer
    printf("清理未使用的framebuffer...\n");
    drmModeFBPtr fb = NULL;
    for (uint32_t fb_id = 1; fb_id < 200; fb_id++) 
    {  // 假设fb_id小于200
        if (drmModeRmFB(my_dev.fd, fb_id) == 0) 
        {
            printf("  已删除framebuffer %u\n", fb_id);
        }
    }
    

    // 3. 创建framebuffer
    if (create_framebuffer(&my_dev, WIDTH, HEIGHT) < 0) 
    {
        printf("帧缓冲创建失败\n");
        return -1;
    }
    
    // 4. 从共享内存获取数据并复制到framebuffer
    uint8_t* argb888_data = Get_ARGB_Data(shm);
    if (!argb888_data) {
        printf("获取ARGB数据失败\n");
        return -1;
    }
    
    uint8_t *dst = (uint8_t*)my_dev.fb_data;
    uint8_t *src = argb888_data;
    
    for (int y = 0; y < HEIGHT; y++) 
    {        
        memcpy(dst, src, WIDTH * 4);
        dst += my_dev.fb_pitch;
        src += WIDTH * 4;
    }
    
    // 5. 设置CRTC显示
    if (drmModeSetCrtc(my_dev.fd, my_dev.crtc_id, my_dev.fb_id, 
                       0, 0, &my_dev.conn_id, 1, &my_dev.mode) < 0) 
    {
        printf("显示启动失败: %s\n", strerror(errno));
        return -1;
    }
    
    printf("显示成功！按Ctrl+C退出\n");
    
    // 6. 保持显示
    sleep(10);
    
    // 7. 清理资源
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
