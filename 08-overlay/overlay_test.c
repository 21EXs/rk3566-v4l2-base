/**
 * 08-overlay: DRM Overlay Plane 最小实现示例
 *
 * 功能：
 * 1. 打开 DRM 设备，获取 connector/crtc/plane 信息
 * 2. 在 primary plane 显示纯色背景
 * 3. 在 overlay plane 显示一个半透明小方块（模拟 UI 按钮）
 * 4. 硬件自动叠加两层
 *
 * 编译: make
 * 运行: ./overlay_test
 * 退出: Ctrl+C
 *
 * 学习要点：
 * - drmModeGetPlaneResources() 获取所有 plane
 * - drmModeGetPlane() 查询 plane 属性
 * - DRM_PLANE_TYPE_OVERLAY / DRM_PLANE_TYPE_PRIMARY
 * - drmModeSetPlane() 设置 overlay 显示
 * - z-order 和 alpha 混合
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl>
#include <xf86drm.h>
#include <xf86drmMode.h>

// ============================================================
// 配置参数（你可以修改这些值来观察效果）
// ============================================================

// Overlay 方块的位置和大小（相对于屏幕）
#define OVERLAY_X       100     // 方块左上角 X
#define OVERLAY_Y       100     // 方块左上角 Y
#define OVERLAY_W       200     // 方块宽度
#define OVERLAY_H       200     // 方块高度

// Overlay 颜色 (ARGB8888)
#define OVERLAY_COLOR   0x80FF0000  // 半透明红色 (A=0x80, R=0xFF, G=0x00, B=0x00)

// Primary 背景颜色
#define BG_COLOR        0xFF336699  // 蓝色背景

// ============================================================
// 全局变量
// ============================================================

static int drm_fd = -1;
static volatile int keep_running = 1;

// ============================================================
// 信号处理
// ============================================================

static void sigint_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

// ============================================================
// 工具函数：创建 dumb buffer 并映射
// ============================================================

struct buffer {
    uint32_t fb_id;
    uint32_t handle;
    uint32_t pitch;
    uint64_t size;
    void *map;
};

static int create_dumb_buffer(int fd, int width, int height, int bpp,
                              struct buffer *buf)
{
    struct drm_mode_create_dumb create = {0};
    struct drm_mode_map_dumb map = {0};

    create.width = width;
    create.height = height;
    create.bpp = bpp;

    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        perror("DRM_IOCTL_MODE_CREATE_DUMB");
        return -1;
    }

    buf->handle = create.handle;
    buf->size = create.size;
    buf->pitch = create.pitch;

    if (drmModeAddFB(fd, width, height, 24, 32, buf->pitch,
                     buf->handle, &buf->fb_id) < 0) {
        perror("drmModeAddFB");
        return -1;
    }

    map.handle = buf->handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        perror("DRM_IOCTL_MODE_MAP_DUMB");
        return -1;
    }

    buf->map = mmap(0, buf->size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, map.offset);
    if (buf->map == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    return 0;
}

static void destroy_dumb_buffer(int fd, struct buffer *buf)
{
    if (buf->map)
        munmap(buf->map, buf->size);
    if (buf->fb_id)
        drmModeRmFB(fd, buf->fb_id);
    if (buf->handle) {
        struct drm_mode_destroy_dumb destroy = {0};
        destroy.handle = buf->handle;
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
}

// ============================================================
// 填充像素数据
// ============================================================

static void fill_solid_color(struct buffer *buf, int width, int height,
                             uint32_t color)
{
    uint32_t *pixels = (uint32_t *)buf->map;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * width + x] = color;
        }
    }
}

static void fill_overlay_pattern(struct buffer *buf, int width, int height,
                                 uint32_t color)
{
    uint32_t *pixels = (uint32_t *)buf->map;

    // 画一个圆角方块（带透明通道）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 判断是否在圆角区域内
            int radius = 20;  // 圆角半径
            int in_corner = 0;

            // 四个角的圆角判断
            if (x < radius && y < radius) {
                // 左上角
                in_corner = ((x - radius) * (x - radius) +
                             (y - radius) * (y - radius) > radius * radius);
            } else if (x >= width - radius && y < radius) {
                // 右上角
                in_corner = ((x - (width - radius)) * (x - (width - radius)) +
                             (y - radius) * (y - radius) > radius * radius);
            } else if (x < radius && y >= height - radius) {
                // 左下角
                in_corner = ((x - radius) * (x - radius) +
                             (y - (height - radius)) * (y - (height - radius)) > radius * radius);
            } else if (x >= width - radius && y >= height - radius) {
                // 右下角
                in_corner = ((x - (width - radius)) * (x - (width - radius)) +
                             (y - (height - radius)) * (y - (height - radius)) > radius * radius);
            }

            if (in_corner) {
                pixels[y * width + x] = 0x00000000;  // 透明
            } else {
                pixels[y * width + x] = color;       // 半透明颜色
            }
        }
    }
}

// ============================================================
// 主函数
// ============================================================

int main(void)
{
    signal(SIGINT, sigint_handler);

    printf("=== DRM Overlay Plane 最小实现示例 ===\n\n");

    // --------------------------------------------------------
    // 1. 打开 DRM 设备
    // --------------------------------------------------------
    drm_fd = drmOpen("rockchip", NULL);
    if (drm_fd < 0) 
    {
        drm_fd = open("/dev/dri/card0", O_RDWR);
        if (drm_fd < 0)
        {
            perror("打开 DRM 设备失败");
            return -1;
        }
    }
    printf("[1] DRM 设备已打开, fd=%d\n", drm_fd);

    // --------------------------------------------------------
    // 2. 获取资源信息
    // --------------------------------------------------------
    drmModeRes *res = drmModeGetResources(drm_fd);
    if (!res) 
    {
        perror("drmModeGetResources");
        close(drm_fd);
        return -1;
    }
    printf("[2] 资源: %d connector(s), %d crtc(s), %d encoder(s)\n",
           res->count_connectors, res->count_crtcs, res->count_encoders);

    // --------------------------------------------------------
    // 3. 查找已连接的 connector
    // --------------------------------------------------------
    drmModeConnector *conn = NULL;
    for (int i = 0; i < res->count_connectors; i++) 
    {
        drmModeConnector *c = drmModeGetConnector(drm_fd, res->connectors[i]);
        if (c && c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) 
        {
            conn = c;
            printf("[3] 找到已连接 connector: id=%d, 模式: %dx%d\n",
                   conn->connector_id, conn->modes[0].hdisplay,
                   conn->modes[0].vdisplay);
            break;
        }
        if (c) drmModeFreeConnector(c);
    }
    if (!conn) 
    {
        printf("未找到已连接的 connector\n");
        drmModeFreeResources(res);
        close(drm_fd);
        return -1;
    }

    int screen_w = conn->modes[0].hdisplay;
    int screen_h = conn->modes[0].vdisplay;

    // --------------------------------------------------------
    // 4. 查找 CRTC
    // --------------------------------------------------------
    uint32_t crtc_id = 0;
    for (int i = 0; i < res->count_encoders; i++) 
    {
        drmModeEncoder *enc = drmModeGetEncoder(drm_fd, res->encoders[i]);
        if (enc) 
        {
            if (enc->crtc_id) 
            {
                crtc_id = enc->crtc_id;
                drmModeFreeEncoder(enc);
                break;
            }
            drmModeFreeEncoder(enc);
        }
    }
    if (!crtc_id && res->count_crtcs > 0) {
        crtc_id = res->crtcs[0];
    }
    printf("[4] 使用 CRTC: id=%d\n", crtc_id);

    // --------------------------------------------------------
    // 5. 查询所有 plane（重点学习内容）
    // --------------------------------------------------------
    drmModePlaneRes *plane_res = drmModeGetPlaneResources(drm_fd);
    if (!plane_res) 
    {
        perror("drmModeGetPlaneResources");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(drm_fd);
        return -1;
    }
    printf("[5] 系统共有 %d 个 plane:\n", plane_res->count_planes);

    // 遍历所有 plane，找出 primary 和 overlay
    uint32_t primary_plane_id = 0;
    uint32_t overlay_plane_id = 0;

    for (uint32_t i = 0; i < plane_res->count_planes; i++) 
    {
        drmModePlane *plane = drmModeGetPlane(drm_fd, plane_res->planes[i]);
        if (!plane) continue;

        // 获取 plane type 属性
        drmModeObjectProperties *props =
            drmModeObjectGetProperties(drm_fd, plane->plane_id,
                                       DRM_MODE_OBJECT_PLANE);
        uint32_t plane_type = 0;
        for (uint32_t j = 0; j < props->count_props; j++) 
        {
            drmModePropertyRes *prop =
                drmModeGetProperty(drm_fd, props->props[j]);
            if (prop && strcmp(prop->name, "type") == 0) 
            {
                plane_type = props->prop_values[j];
            }
            if (prop) drmModeFreeProperty(prop);
        }
        drmModeFreeObjectProperties(props);

        const char *type_str = "未知";
        if (plane_type == DRM_PLANE_TYPE_PRIMARY)
            type_str = "PRIMARY";
        else if (plane_type == DRM_PLANE_TYPE_OVERLAY)
            type_str = "OVERLAY";
        else if (plane_type == DRM_PLANE_TYPE_CURSOR)
            type_str = "CURSOR";

        printf("   Plane[%d]: id=%d, type=%s, 支持尺寸: %d~%d x %d~%d\n",
               i, plane->plane_id, type_str,
               plane->possible_crtcs,
               // 注意: possible_crtcs 是 bitmask，这里简化显示
               0, 0);

        if (plane_type == DRM_PLANE_TYPE_PRIMARY && !primary_plane_id)
            primary_plane_id = plane->plane_id;
        if (plane_type == DRM_PLANE_TYPE_OVERLAY && !overlay_plane_id)
            overlay_plane_id = plane->plane_id;

        drmModeFreePlane(plane);
    }

    if (!primary_plane_id) {
        printf("未找到 primary plane!\n");
        drmModeFreePlaneResources(plane_res);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(drm_fd);
        return -1;
    }
    printf("\n   → 使用 Primary Plane: id=%d\n", primary_plane_id);
    if (overlay_plane_id)
        printf("   → 使用 Overlay Plane: id=%d\n", overlay_plane_id);
    else
        printf("   → 未找到 Overlay Plane! (可能硬件不支持)\n");

    // --------------------------------------------------------
    // 6. 创建 primary framebuffer（全屏背景）
    // --------------------------------------------------------
    struct buffer primary_buf = {0};
    if (create_dumb_buffer(drm_fd, screen_w, screen_h, 32, &primary_buf) < 0) 
    {
        printf("创建 primary buffer 失败\n");
        drmModeFreePlaneResources(plane_res);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(drm_fd);
        return -1;
    }
    fill_solid_color(&primary_buf, screen_w, screen_h, BG_COLOR);
    printf("[6] Primary buffer 创建完成: %dx%d, fb_id=%d\n",
           screen_w, screen_h, primary_buf.fb_id);

    // --------------------------------------------------------
    // 7. 创建 overlay framebuffer（半透明方块）
    // --------------------------------------------------------
    struct buffer overlay_buf = {0};
    if (overlay_plane_id) 
    {
        if (create_dumb_buffer(drm_fd, OVERLAY_W, OVERLAY_H, 32,
                               &overlay_buf) < 0) 
        {
            printf("创建 overlay buffer 失败\n");
            overlay_plane_id = 0;
        } 
        else 
        {
            fill_overlay_pattern(&overlay_buf, OVERLAY_W, OVERLAY_H,
                                 OVERLAY_COLOR);
            printf("[7] Overlay buffer 创建完成: %dx%d, fb_id=%d\n",
                   OVERLAY_W, OVERLAY_H, overlay_buf.fb_id);
        }
    }

    // --------------------------------------------------------
    // 8. 设置 CRTC 显示 primary plane
    // --------------------------------------------------------
    if (drmModeSetCrtc(drm_fd, crtc_id, primary_buf.fb_id,
                       0, 0, &conn->connector_id, 1,
                       &conn->modes[0]) < 0) 
    {
        perror("drmModeSetCrtc");
        destroy_dumb_buffer(drm_fd, &primary_buf);
        destroy_dumb_buffer(drm_fd, &overlay_buf);
        drmModeFreePlaneResources(plane_res);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(drm_fd);
        return -1;
    }
    printf("[8] Primary plane 已显示 (蓝色背景)\n");

    // --------------------------------------------------------
    // 9. 设置 overlay plane（核心学习内容）
    // --------------------------------------------------------
    if (overlay_plane_id) 
    {
        /**
         * drmModeSetPlane() 参数说明：
         *
         * fd          - DRM 设备文件描述符
         * plane_id    - overlay plane 的 ID
         * crtc_id     - 绑定的 CRTC ID
         * fb_id       - 要显示的 framebuffer ID
         * flags       - 标志位 (0 或 DRM_MODE_PAGE_FLIP_EVENT)
         *
         * crtc_x, crtc_y     - 在屏幕上的显示位置（左上角坐标）
         * crtc_w, crtc_h     - 在屏幕上的显示尺寸（可以缩放）
         *
         * src_x, src_y       - framebuffer 中的裁剪起点（32.16 定点数）
         * src_w, src_h       - framebuffer 中的裁剪尺寸（32.16 定点数）
         *
         * 注意: src_* 参数是 16.16 定点数，需要左移 16 位
         *       例如: src_w = OVERLAY_W << 16
         */
        int ret = drmModeSetPlane(
            drm_fd,             // DRM 设备
            overlay_plane_id,   // overlay plane ID
            crtc_id,            // CRTC ID
            overlay_buf.fb_id,  // framebuffer ID
            0,                  // flags
            // 屏幕上的显示位置和大小
            OVERLAY_X,          // crtc_x: 屏幕上的 X 坐标
            OVERLAY_Y,          // crtc_y: 屏幕上的 Y 坐标
            OVERLAY_W,          // crtc_w: 屏幕上的显示宽度
            OVERLAY_H,          // crtc_h: 屏幕上的显示高度
            // framebuffer 中的裁剪区域 (16.16 定点数)
            0 << 16,            // src_x: 从 framebuffer 的 X 偏移
            0 << 16,            // src_y: 从 framebuffer 的 Y 偏移
            OVERLAY_W << 16,    // src_w: framebuffer 中裁剪宽度
            OVERLAY_H << 16     // src_h: framebuffer 中裁剪高度
        );

        if (ret < 0) 
        {
            perror("drmModeSetPlane (overlay)");
            printf("  提示: 可能该 overlay plane 不支持当前格式或尺寸\n");
            printf("  尝试调整 OVERLAY_W/OVERLAY_H 或检查 plane 支持的颜色格式\n");
        }
        else 
        {
            printf("[9] Overlay plane 已显示 (半透明红色方块在 (%d,%d))\n",
                   OVERLAY_X, OVERLAY_Y);
            printf("    → 硬件正在自动叠加两层: 背景 + 半透明按钮\n");
        }
    }

    // --------------------------------------------------------
    // 10. 保持运行，直到 Ctrl+C
    // --------------------------------------------------------
    printf("\n=== 叠加显示中，按 Ctrl+C 退出 ===\n");
    printf("提示: 修改 OVERLAY_X/Y/W/H 和 OVERLAY_COLOR 观察效果\n\n");

    while (keep_running) {
        usleep(100000);  // 100ms
    }

    // --------------------------------------------------------
    // 11. 清理
    // --------------------------------------------------------
    printf("\n清理资源...\n");

    // 关闭 overlay plane（解除绑定）
    if (overlay_plane_id) {
        drmModeSetPlane(drm_fd, overlay_plane_id, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0);
    }

    destroy_dumb_buffer(drm_fd, &overlay_buf);
    destroy_dumb_buffer(drm_fd, &primary_buf);
    drmModeFreePlaneResources(plane_res);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(drm_fd);

    printf("已退出\n");
    return 0;
}
