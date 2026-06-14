#ifndef _ATOMIC_DRM_H_
#define _ATOMIC_DRM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "disp_manager.h"
#include "drm_fourcc.h"
#include "shm.h"

// RGA 旋转控制宏
// 1 = 在 DRM 显示时用 RGA 做旋转90度（适配竖屏）
// 0 = 不旋转，直接显示
#define RGA_ROTATE_ENABLE  1

struct drm_device {
    int fd;                          // 设备文件描述符
    drmModeRes *res;                // DRM资源信息
    drmModeConnector *connector;    // 连接器信息
    uint32_t crtc_id;               // CRTC ID
    uint32_t conn_id;               // 连接器 ID
    drmModeModeInfo mode;           // 显示模式
    
    uint32_t fb_id;                 // 帧缓冲ID
    uint32_t fb_handle;             // 缓冲句柄
    uint32_t fb_pitch;              // 行间距
    uint64_t fb_size;               // 缓冲大小
    void *fb_data;                  // 映射的内存地址
};
int create_test_pattern(uint32_t *buffer, int width, int height);
int init_drm_device(struct drm_device *dev);
int create_framebuffer(struct drm_device *dev, int width, int height, uint32_t format);
int find_overlay_plane(int fd);
int drm_start();
#endif