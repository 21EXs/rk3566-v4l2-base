#ifndef __DISP_MANAGER_H__
#define __DISP_MANAGER_H__

#include "drm_fourcc.h"
#include "xf86drm.h"
#include "xf86drmMode.h"

// 帧缓冲资源的结构体
typedef struct drm_buffer {
    uint32_t width;                        // 显示器宽度
    uint32_t height;                       // 显示器高度
    uint32_t pitch;                        // 每行像素占用的字节数
    uint32_t handle;                       // 缓冲区的句柄
    uint32_t size;                         // 帧缓冲总大小
    uint32_t *vaddr;                       // 帧缓冲映射到用户空间的首地址
    uint32_t fb_id;                        // 注册的帧缓冲ID
    uint32_t count_plane;                  // 平面数量计数器
    struct drm_mode_create_dumb create;    // 创建缓冲区的参数及返回值
    struct drm_mode_map_dumb map;          // 映射缓冲区到用户空间的参数
} drm_buf_t;

// CRTC相关属性ID的结构体
typedef struct property_crtc {
    uint32_t blob_id;             // 显示模式的 blob ID（用于CRTC模式设置）
    uint32_t property_crtc_id;    // "CRTC_ID"属性的ID（连接器关联CRTC时使用）
    uint32_t property_mode_id;    // "MODE_ID"属性的ID（CRTC设置显示模式时使用）
    uint32_t property_active;     // "ACTIVE"属性的ID（激活CRTC时使用）
} property_crtc_t;

// 平面相关属性ID的结构体
typedef struct property_planes {
    uint32_t plane_id;           // 平面ID
    uint32_t property_fb_id;     // "FB_ID"属性的ID
    uint32_t property_crtc_x;    // "CRTC_X"属性的ID
    uint32_t property_crtc_y;    // "CRTC_Y"属性的ID
    uint32_t property_crtc_w;    // "CRTC_W"属性的ID
    uint32_t property_crtc_h;    // "CRTC_H"属性的ID
    uint32_t property_src_x;     // "SRC_X"属性的ID
    uint32_t property_src_y;     // "SRC_Y"属性的ID
    uint32_t property_src_w;     // "SRC_W"属性的ID
    uint32_t property_src_h;     // "SRC_H"属性的ID
} property_planes_t;

// 平面配置参数结构体
typedef struct planes_setting {
    uint32_t crtc_id;     // 关联的CRTC ID
    uint32_t plane_id;    // 目标平面ID
    uint32_t fb_id;       // 要显示的帧缓冲ID
    uint32_t crtc_x;      // 平面在CRTC上的X坐标（显示位置）
    uint32_t crtc_y;      // 平面在CRTC上的Y坐标
    uint32_t crtc_w;      // 平面在CRTC上的显示宽度
    uint32_t crtc_h;      // 平面在CRTC上的显示高度
    uint32_t src_x;       // 从帧缓冲中取图的X坐标（源位置）
    uint32_t src_y;       // 从帧缓冲中取图的Y坐标
    uint32_t src_w;       // 从帧缓冲中取图的宽度
    uint32_t src_h;       // 从帧缓冲中取图的高度
} planes_setting_t;

// 颜色定义
#define RED 0XFF0000
#define GREEN 0X00FF00
#define BLUE 0X0000FF

int drm_init(uint32_t format);
void drm_exit(void);

#endif    // __DISP_MANAGER_H__