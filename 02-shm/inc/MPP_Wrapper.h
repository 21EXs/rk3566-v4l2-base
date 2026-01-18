#ifndef __MPP_WRAPPER_H
#define __MPP_WRAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "main.h"
#include <shm.h>
#include <pthread.h>

#include <rockchip/rk_type.h>
#include <rockchip/mpp_err.h>
#include <rockchip/mpp_log.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_buffer.h>
#include <rockchip/mpp_meta.h>
#include <rockchip/mpp_task.h>
#include <rockchip/rk_mpi.h>

typedef struct {
    MppCtx ctx;     //上下文句柄
    MppApi *mpi;    //API函数表

    MppBuffer g_frm_buf;    // 输入帧缓冲区
    MppBuffer g_pkt_buf;    // 输出包缓冲区

    int fps;
    int gop;
    int bitrate;

    // 时间戳管理
    int64_t frame_count;
    int64_t start_time_ns;

    // 文件输出
    int output_fd;

    // 状态标志
    int initialized;
    int got_sps_pps;
    int encoding;

    // 统计
    int frames_encoded;
    int total_bytes;
    int64_t total_time_ns;
}H264Encoder;


H264Encoder* H264Encoder_Init(int width, int height, const char* output_path);
int H264Encoder_EncodeFrame(H264Encoder *enc, uint8_t *nv21_data);
void H264Encoder_Destroy(H264Encoder *enc);

#endif