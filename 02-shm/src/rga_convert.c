#include "rga_convert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "im2d.h"
#include "rga.h"

/*
 * 使用 Rockchip RGA 硬件加速将 NV21 转换为 BGRA8888
 * 输入: src_nv21 - NV21 格式的 YUV 数据
 * 输出: dst_bgra - BGRA8888 格式的 RGB 数据
 * 成功返回 0，失败返回 -1
 */
int RGA_NV21_To_BGRA(unsigned char* src_nv21, unsigned char* dst_bgra, int width, int height)
{
    if (width < 1 || height < 1 || src_nv21 == NULL || dst_bgra == NULL)
        return -1;

    // 使用 im2d API 封装源和目标 buffer
    // wrapbuffer_virtualaddr(vir_addr, width, height, format, wstride, hstride)
    // 如果不传 wstride/hstride，则默认等于 width/height
    rga_buffer_t src = wrapbuffer_virtualaddr(src_nv21, width, height, RK_FORMAT_YCrCb_420_SP);
    rga_buffer_t dst = wrapbuffer_virtualaddr(dst_bgra, width, height, RK_FORMAT_RGBA_8888);

    // 调用 imcvtcolor 进行格式转换（硬件加速）
    // 最后一个参数 1 表示同步模式（等待操作完成）
    IM_STATUS ret = imcvtcolor(src, dst, RK_FORMAT_YCrCb_420_SP, RK_FORMAT_RGBA_8888, IM_COLOR_SPACE_DEFAULT, 1);
    if (ret <= IM_STATUS_FAILED) 
    {
        fprintf(stderr, "[RGA] imcvtcolor 失败: %s (ret=%d)\n", imStrError(ret), ret);
        return -1;
    }

    return 0;
}

/*
 * 使用 Rockchip RGA 硬件加速将 NV21 转换为 BGRA8888 并顺时针旋转 90 度
 * 旋转后输出宽高互换：输出宽度 = height，输出高度 = width
 * 成功返回 0，失败返回 -1
 */
int RGA_NV21_To_BGRA_Rotate90(unsigned char* src_nv21, unsigned char* dst_bgra, int width, int height)
{
    if (width < 1 || height < 1 || src_nv21 == NULL || dst_bgra == NULL)
        return -1;

    // 源 buffer（原始宽高，NV21 格式）
    rga_buffer_t src = wrapbuffer_virtualaddr(src_nv21, width, height, RK_FORMAT_YCrCb_420_SP);

    // 目标 buffer（旋转 90 度后宽高互换，RGBA 格式）
    rga_buffer_t dst = wrapbuffer_virtualaddr(dst_bgra, height, width, RK_FORMAT_RGBA_8888);

    // 定义源和目标矩形区域
    im_rect src_rect = {0, 0, width, height};
    im_rect dst_rect = {0, 0, height, width};

    // pat 为空
    rga_buffer_t pat;
    memset(&pat, 0, sizeof(pat));

    im_rect pat_rect;
    memset(&pat_rect, 0, sizeof(pat_rect));

    // 使用 improcess 一步完成格式转换 + 旋转 90 度
    // improcess 会根据 src/dst 的 format 自动进行颜色格式转换
    IM_STATUS ret = improcess(src, dst, pat, src_rect, dst_rect, pat_rect,IM_HAL_TRANSFORM_ROT_90);
    if (ret <= IM_STATUS_FAILED) 
    {
        fprintf(stderr, "[RGA] improcess (旋转90) 失败: %s (ret=%d)\n", imStrError(ret), ret);
        return -1;
    }

    return 0;
}
