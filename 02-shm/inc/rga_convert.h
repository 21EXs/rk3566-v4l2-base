#ifndef RGA_CONVERT_H
#define RGA_CONVERT_H

#include <stdint.h>

/*
 * 将 NV21 转换为 BGRA8888（Rockchip RGA 硬件加速实现）
 * 成功返回 0，失败返回 -1
 */
int RGA_NV21_To_BGRA(unsigned char* src_nv21, unsigned char* dst_bgra, int width, int height);

/*
 * 将 NV21 转换为 BGRA8888 并顺时针旋转 90 度（Rockchip RGA 硬件加速实现）
 * 旋转后输出宽高互换：输出宽度 = height，输出高度 = width
 * 成功返回 0，失败返回 -1
 */
int RGA_NV21_To_BGRA_Rotate90(unsigned char* src_nv21, unsigned char* dst_bgra, int width, int height);

#endif /* RGA_CONVERT_H */
