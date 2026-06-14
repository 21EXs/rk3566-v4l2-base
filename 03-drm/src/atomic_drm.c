#include "atomic_drm.h"
#include "rga_convert.h"
#include "im2d.h"
#include "rga.h"

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

int create_framebuffer(struct drm_device *dev, int width, int height, uint32_t format)
{
  struct drm_mode_create_dumb create = {0};
  struct drm_mode_map_dumb map = {0};

  create.width = width;
  create.height = height;
  create.bpp = 32;  // 32位色深

  if (drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) 
  {
    printf("创建dumb buffer失败\n");
    return -1;
  }

  dev->fb_handle = create.handle;
  dev->fb_size = create.size;
  dev->fb_pitch = create.pitch;

  // 使用 drmModeAddFB2 指定像素格式
  uint32_t handles[4] = {dev->fb_handle, 0, 0, 0};
  uint32_t pitches[4] = {dev->fb_pitch, 0, 0, 0};
  uint32_t offsets[4] = {0, 0, 0, 0};
  if (drmModeAddFB2(dev->fd, width, height, format,
                    handles, pitches, offsets, &dev->fb_id, 0) < 0)
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

int find_overlay_plane(int fd)
{
    drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
    if (!plane_res) 
    {
        printf("获取plane资源失败\n");
        return -1;
    }
    
    uint32_t overlay_id = 0;
    for (int i = 0; i < plane_res->count_planes; i++) {
        drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[i]);
        if (!plane) continue;
        
        drmModeObjectProperties *props = drmModeObjectGetProperties(fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
        if (!props) 
        {
            drmModeFreePlane(plane);
            continue;
        }
        
        for (int j = 0; j < props->count_props; j++) 
        {
            drmModePropertyRes *prop = drmModeGetProperty(fd, props->props[j]);
            if (prop && strcmp(prop->name, "type") == 0) {
                if (props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY) 
                {
                    overlay_id = plane->plane_id;
                    printf("找到 overlay plane, id=%d\n", overlay_id);
                }
            }
            if (prop) drmModeFreeProperty(prop);
            if (overlay_id) break;
        }
        drmModeFreeObjectProperties(props);
        drmModeFreePlane(plane);
        if (overlay_id) break;
    }
    
    drmModeFreePlaneResources(plane_res);
    return overlay_id;
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

    // 查找 overlay plane
    uint32_t overlay_plane_id = find_overlay_plane(my_dev.fd);
    if (overlay_plane_id == 0) 
    {
        printf("未找到 overlay plane\n");
        return -1;
    }

    // 获取屏幕分辨率
    int screen_width = my_dev.mode.hdisplay;
    int screen_height = my_dev.mode.vdisplay;
    
    shm_ptr = Shm_Open();
    if (shm_ptr == NULL) 
    {
        fprintf(stderr, "无法获取共享内存\n");
        return -1;
    }
    // 共享内存中的 BGRA 数据已经是旋转后的（由 shm_app 完成 NV21→BGRA+旋转90度）
    // 旋转后：img_width_rotated = HEIGHT = 720, img_height_rotated = WIDTH = 1280
    // 屏幕是竖屏 720×1280，所以旋转后的数据正好满屏显示
    int img_width = HEIGHT;   // 旋转后宽度 = 原高度 = 720
    int img_height = WIDTH;   // 旋转后高度 = 原宽度 = 1280
    
    // primary plane: 使用 XRGB8888（BGRA 数据，无 alpha 通道）
    if (create_framebuffer(&my_dev, screen_width, screen_height, DRM_FORMAT_XRGB8888) < 0) 
    {
        printf("帧缓冲创建失败\n");
        return -1;
    }

    struct drm_device overlay_dev;
    memset(&overlay_dev, 0, sizeof(struct drm_device));
    overlay_dev.fd = my_dev.fd;
    // overlay plane: 使用 ARGB8888（带 alpha 通道，支持透明混合）
    create_framebuffer(&overlay_dev, screen_width, screen_height, DRM_FORMAT_ARGB8888);

    drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(overlay_dev.fd, overlay_plane_id, DRM_MODE_OBJECT_PLANE);
    for (int i = 0; i < props->count_props; i++) {
        drmModePropertyPtr prop = drmModeGetProperty(overlay_dev.fd, props->props[i]);
        if (strcmp(prop->name, "zpos") == 0) 
        {
            // 设置较低的 zpos 值（0通常是最底层）
            drmModeObjectSetProperty(overlay_dev.fd, overlay_plane_id, DRM_MODE_OBJECT_PLANE,
                                    props->props[i], 2); // 或更小的值
        }
        drmModeFreeProperty(prop);
    }

    
    // 清空屏幕为黑色
    memset(my_dev.fb_data, 0, my_dev.fb_size);
    
    // 将 overlay buffer 全部初始化为 0（全透明）
    // 这样未绘制的地方不会遮挡 primary layer
    memset(overlay_dev.fb_data, 0, overlay_dev.fb_size);
    for (int y = screen_height/2 - 100; y < screen_height/2 + 100; y++) 
    {
        for (int x = screen_width/2 - 100; x < screen_width/2 + 100; x++) 
        {
            if (y >= 0 && y < screen_height && x >= 0 && x < screen_width) 
            {
                uint32_t *pixel = (uint32_t*)overlay_dev.fb_data + y * screen_width + x;
                *pixel = 0x80FF0000;  // 半透明红色 (A=0x80, R=0xFF)
            }
        }
    }
    printf("图像显示: %dx%d (已旋转), 屏幕: %dx%d\n", img_width, img_height, screen_width, screen_height);

    uint8_t* dst = 0;

    while (1) 
    {
        sem_wait(&shm_ptr->sem.convert_done);
        
        // 获取BGRA数据起始地址（已经是旋转后的 720×1280）
        uint8_t* bgra_data = Get_Frame_Data_Offset(shm_ptr, BGRA_TYPE, shm_ptr->sem.BGRA_Avail_Buf);

        dst = (uint8_t*)my_dev.fb_data;

        // 直接满屏拷贝（旋转后的数据尺寸 = 720×1280，与屏幕一致）
        for (int y = 0; y < img_height && y < screen_height; y++)
        {
            int src_index = y * img_width * 4;
            int dst_index = y * screen_width * 4;
            memcpy(dst + dst_index, bgra_data + src_index, img_width * 4);
        }
        
        // 设置 primary plane（摄像头图像）
        if (drmModeSetCrtc(my_dev.fd, my_dev.crtc_id, my_dev.fb_id, 
                        0, 0, &my_dev.conn_id, 1, &my_dev.mode) < 0) 
        {
            printf("显示启动失败: %s\n", strerror(errno));
            return -1;
        }
        
        // 设置 overlay plane（Qt 界面层，数据已在初始化时填充）
        if (drmModeSetPlane(my_dev.fd, overlay_plane_id, my_dev.crtc_id,
                        overlay_dev.fb_id, 0,
                        0, 0, screen_width, screen_height,
                        0, 0, screen_width << 16, screen_height << 16) < 0)
        {
            printf("overlay plane 设置失败: %s\n", strerror(errno));
        }
        
        UpdatePollID(BGRA_TYPE);
        sem_post(&shm_ptr->sem.display_done);
    }

    // 清理资源
    if (overlay_dev.fb_data) 
    {
        munmap(overlay_dev.fb_data, overlay_dev.fb_size);
    }
    if (overlay_dev.fb_id) 
    {
        drmModeRmFB(my_dev.fd, overlay_dev.fb_id);
    }
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