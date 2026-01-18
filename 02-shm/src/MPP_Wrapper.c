#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "main.h"
#include <shm.h>
#include <pthread.h>
#include <sys/time.h>  // 需要这个头文件才能用gettimeofday

#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_buffer.h>
#include "MPP_Wrapper.h"

size_t nv21_size = WIDTH * HEIGHT * 3 / 2;

int encoder_cfg_nv21(H264Encoder *enc)
{
    if (!enc->ctx || !enc->mpi)
        return -1;
    
    MppEncCfg cfg = NULL;
    mpp_enc_cfg_init(&cfg);
    
    mpp_enc_cfg_set_s32(cfg, "prep:width",       WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:height",      HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",  WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",  HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:format",      MPP_FMT_YUV420SP_VU);
    mpp_enc_cfg_set_s32(cfg, "rc:mode",          MPP_ENC_RC_MODE_CBR);
    mpp_enc_cfg_set_s32(cfg, "rc:bps",           WIDTH * HEIGHT * 7.5);
    mpp_enc_cfg_set_s32(cfg, "rc:fps",           enc->fps);  // 改为30fps
    mpp_enc_cfg_set_s32(cfg, "rc:gop",           enc->gop);  // 改为30
    
    MPP_RET ret = enc->mpi->control(enc->ctx, MPP_ENC_SET_CFG, cfg);
    mpp_enc_cfg_deinit(cfg);
    return (ret == MPP_OK) ? 0 : -1;
}

H264Encoder* H264Encoder_Init(int width, int height, const char* output_path)
{
    H264Encoder *enc = (H264Encoder*)calloc(1, sizeof(H264Encoder));
    if (!enc) 
    {
        printf("错误：无法分配编码器内存\n");
        return NULL;
    }

    enc->ctx = NULL;//上下文句柄
    enc->mpi = NULL;//API函数表

    enc->g_frm_buf = NULL;
    enc->g_pkt_buf = NULL;

    enc->fps = 30;//帧率
    enc->gop = 30;//关键帧间隔
    enc->bitrate = width * height * 7.5;//码率

    enc->frame_count = 0;//已编码帧数
    enc->start_time_ns = 0;//开始时间

    enc->output_fd = -1;

    enc->initialized = 0;//初始化状态位，0表示未初始化
    enc->got_sps_pps = 0;//SPS/PPS头，0表示未获取
    enc->encoding = 0;//编码状态，0表示没有在编码

    enc->frames_encoded = 0;//已经成功编码的帧数
    enc->total_bytes = 0;//输出总字节数
    enc->total_time_ns = 0;//编码总耗时

    if(output_path)
    {
        enc->output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(enc->output_fd < 0)
        {
            printf("错误：无法打开输出文件 %s\n", output_path);
            free(enc);
            return NULL;
        }
    }
    else 
    {
        enc->output_fd = -1;  // 无文件输出
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    enc->start_time_ns = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;

    printf("结构体初始化成功: %dx%d@%dfps, GOP=%d, 码率=%d\n", 
           width, height, enc->fps, enc->gop, enc->bitrate);
    
    MPP_RET Ret_Creat = mpp_create(&enc->ctx, &enc->mpi);
    if(Ret_Creat != MPP_OK)
    {
        printf("MPP创建失败\n");
        return NULL;
    }

    MPP_RET Ret_Init = mpp_init(enc->ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
    if(Ret_Init != MPP_OK)
    {
        printf("MPP初始化失败\n");
        return NULL;
    }
    printf("MPP初始化成功\n");

    if (encoder_cfg_nv21(enc) != 0) 
    {
        printf("MPP配置失败\n");
        return NULL;
    }
    printf("MPP配置成功\n");

    /*硬件加速 */
    MPP_RET ret = mpp_buffer_get(NULL, &enc->g_frm_buf, nv21_size);   
    if (ret != MPP_OK) 
    {
        printf("错误：无法分配帧缓冲区\n");
        return NULL;
    }   
    printf("帧缓冲区分配成功\n");
    ret = mpp_buffer_get(NULL, &enc->g_pkt_buf, nv21_size * 2);//包缓冲区要大一些（编码后数据可能更大）
    if (ret != MPP_OK) 
    {
        printf("错误：无法分配包帧缓冲区\n");
        return NULL;
    }  
    printf("包缓冲区分配成功\n");

    MppPacket header_packet = NULL;//准备一个空的H.264参数集包
    ret = enc->mpi->control(enc->ctx, MPP_ENC_GET_EXTRA_INFO, &header_packet);//MPP_ENC_GET_EXTRA_INFO获取H.264/H.265编码器的SPS、PPS、VPS等参数集
    if (ret == MPP_OK && header_packet) 
    {
        void *ptr = mpp_packet_get_pos(header_packet);//获取获取包里数据的起始地址
        size_t len = mpp_packet_get_length(header_packet);//获取数据有多长
        
        if (enc->output_fd >= 0) 
        {
            ssize_t written = write(enc->output_fd, ptr, len);
            if (written == (ssize_t)len) 
            {
                enc->got_sps_pps = 1;
                enc->total_bytes += len;
                printf("SPS/PPS头写入成功: %zu字节\n", len);
            }
            else
            {
                printf("SPS/PPS头写入不完整: %zd/%zu\n", written, len);
            }
        }
        mpp_packet_deinit(&header_packet);
    }

    enc->initialized = 1;
    printf("H.264编码器初始化完成\n");

    return enc;
}

int H264Encoder_EncodeFrame(H264Encoder *enc, uint8_t *nv21_data)
{
    if (!enc || !enc->initialized || !nv21_data) 
    {
        printf("编码器未初始化或参数错误\n");
        return -1;
    }
    size_t nv21_size = WIDTH * HEIGHT * 3 / 2; 
    uint8_t *dst = (uint8_t*)mpp_buffer_get_ptr(enc->g_frm_buf);
    memcpy(dst, nv21_data, nv21_size);  

    MppFrame frame = NULL;
    mpp_frame_init(&frame);
    mpp_frame_set_buffer(frame, enc->g_frm_buf);
    mpp_frame_set_width(frame, WIDTH);
    mpp_frame_set_height(frame, HEIGHT);
    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP_VU);

    mpp_frame_set_hor_stride(frame, WIDTH);
    mpp_frame_set_ver_stride(frame, HEIGHT);

    int64_t pts = enc->frame_count * 3000;  // 90kHz时基，30fps=3000
    mpp_frame_set_pts(frame, pts);

    // 创建MppPacket
    MppPacket packet = NULL;
    mpp_packet_init_with_buffer(&packet, enc->g_pkt_buf);

    MPP_RET ret = enc->mpi->encode_put_frame(enc->ctx, frame);//送入帧
    if (ret != MPP_OK) 
    {
        printf("送入帧失败: %d\n", ret);
        goto cleanup;
    }
    ret = enc->mpi->encode_get_packet(enc->ctx, &packet);//取出包
    if (ret != MPP_OK) 
    {
        printf("取出包失败: %d\n", ret);
        goto cleanup;
    }

    //获取编码后的数据
    void *encoded_data = mpp_packet_get_pos(packet);
    size_t encoded_size = mpp_packet_get_length(packet);

    if (enc->output_fd >= 0 && encoded_size > 0) 
    {
        ssize_t written = write(enc->output_fd, encoded_data, encoded_size);
        if (written > 0) 
        {
            enc->total_bytes += written;
            enc->frames_encoded++;
        }
    }

    enc->frame_count++;//完成一帧编码，计数加一

    cleanup:
    // 清理资源
    if (frame) mpp_frame_deinit(&frame);
    if (packet) mpp_packet_deinit(&packet);
    
    return (ret == MPP_OK) ? 0 : -1;
    
}

void H264Encoder_Destroy(H264Encoder *enc)
{
    if (!enc) return;
    
    printf("清理编码器...\n");
    
    // 1. 关闭文件
    if (enc->output_fd >= 0) 
    {
        close(enc->output_fd);
    }
    
    // 2. 释放缓冲区
    if (enc->g_frm_buf) 
    {
        mpp_buffer_put(enc->g_frm_buf);
    }
    if (enc->g_pkt_buf) 
    {
        mpp_buffer_put(enc->g_pkt_buf);
    }
    
    // 3. 销毁MPP上下文
    if (enc->ctx) 
    {
        mpp_destroy(enc->ctx);
    }
    
    // 4. 打印统计信息
    printf("编码统计: %d帧, %lld字节\n", 
           enc->frames_encoded, enc->total_bytes);
    
    // 5. 释放结构体
    free(enc);
    
    printf("编码器清理完成\n");
}

// int MPP_Encode_One_Frame(void)
// {
//     MppFrame  frame  = NULL;
//     MppPacket packet = NULL;
    
//     static int64_t frame_count = 0;  // 帧计数器
//     static int first_call = 1;
//     static int64_t start_time_us = 0;
//     static int64_t first_pts = 0;
    
//     /* 1. 把已注册好的输入 buffer 包成 MppFrame */
//     mpp_frame_init(&frame);
//     mpp_frame_set_width(frame, WIDTH);
//     mpp_frame_set_height(frame, HEIGHT);
//     mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP_VU);
//     mpp_frame_set_buffer(frame, g_frm_buf);
//     mpp_frame_set_hor_stride(frame, WIDTH);
//     mpp_frame_set_ver_stride(frame, HEIGHT);
    
//     // 计算PTS（基于30fps）
//     // 90kHz时基下，30fps时每帧间隔是 90000/30 = 3000
//     int64_t pts = frame_count * 3000;
//     mpp_frame_set_pts(frame, pts);
    
//     frame_count++;
    
//     /* 2. 把输出 buffer 包成 MppPacket */
//     mpp_packet_init(&packet, NULL, 0);
//     mpp_packet_set_buffer(packet, g_pkt_buf);
    
//     /* 3. 送帧-取包 */
//     MPP_RET ret;
//     ret = g_mpi->encode_put_frame(g_ctx, frame);
//     if (ret != MPP_OK) goto DONE;
    
//     ret = g_mpi->encode_get_packet(g_ctx, &packet);
//     if (ret != MPP_OK) goto DONE;
    
//     /* 4. 写文件 */
//     void   *ptr = mpp_packet_get_pos(packet);
//     size_t  len = mpp_packet_get_length(packet);
//     write(g_fd_h264, ptr, len);
    
// DONE:
//     mpp_frame_deinit(&frame);
//     mpp_packet_deinit(&packet);
//     return (ret == MPP_OK) ? len : -1;
// }
// /* 让 ctx/mpi 在本文件内全局可见，供后续 encode 使用 */
// static MppCtx  g_ctx = NULL;//上下文句柄
// static MppApi *g_mpi = NULL;//API函数表

// static MppBuffer g_frm_buf = NULL;
// static MppBuffer g_pkt_buf = NULL;

// extern int g_fd_h264; 
// extern struct shared_memory *shm_ptr;

// int64_t get_relative_pts(int timebase_num, int timebase_den) 
// {
//     static struct timespec start_ts = {0, 0};
//     static int initialized = 0;
//     struct timespec now_ts;
    
//     clock_gettime(CLOCK_MONOTONIC, &now_ts);
    
//     if (!initialized) 
//     {
//         start_ts = now_ts;
//         initialized = 1;
//         return 0;
//     }
    
//     // 计算经过的纳秒
//     uint64_t elapsed_ns = (uint64_t)(now_ts.tv_sec - start_ts.tv_sec) * 1000000000ULL
//                          + (now_ts.tv_nsec - start_ts.tv_nsec);
    
//     // 转换为时基单位
//     // elapsed_ns * timebase_den / (1e9 * timebase_num)
//     int64_t pts = (int64_t)(elapsed_ns * timebase_den) / (1000000000LL * timebase_num);
    
//     return pts;
// }


// /* 配置函数：只负责写寄存器，不返回任何指针 */
// int encoder_cfg_nv21(void)
// {
//     if (!g_ctx || !g_mpi)
//         return -1;
    
//     MppEncCfg cfg = NULL;
//     mpp_enc_cfg_init(&cfg);
    
//     mpp_enc_cfg_set_s32(cfg, "prep:width",       WIDTH);
//     mpp_enc_cfg_set_s32(cfg, "prep:height",      HEIGHT);
//     mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",  WIDTH);
//     mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",  HEIGHT);
//     mpp_enc_cfg_set_s32(cfg, "prep:format",      MPP_FMT_YUV420SP_VU);
//     mpp_enc_cfg_set_s32(cfg, "rc:mode",          MPP_ENC_RC_MODE_CBR);
//     mpp_enc_cfg_set_s32(cfg, "rc:bps",           WIDTH * HEIGHT * 7.5);
//     mpp_enc_cfg_set_s32(cfg, "rc:fps",           30);  // 改为30fps
//     mpp_enc_cfg_set_s32(cfg, "rc:gop",           30);  // 改为30
    
//     MPP_RET ret = g_mpi->control(g_ctx, MPP_ENC_SET_CFG, cfg);
//     mpp_enc_cfg_deinit(cfg);
//     return (ret == MPP_OK) ? 0 : -1;
// }

// int MPP_Encode_One_Frame(void)
// {
//     MppFrame  frame  = NULL;
//     MppPacket packet = NULL;
    
//     static int64_t frame_count = 0;  // 帧计数器
//     static int first_call = 1;
//     static int64_t start_time_us = 0;
//     static int64_t first_pts = 0;
    
//     /* 1. 把已注册好的输入 buffer 包成 MppFrame */
//     mpp_frame_init(&frame);
//     mpp_frame_set_width(frame, WIDTH);
//     mpp_frame_set_height(frame, HEIGHT);
//     mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP_VU);
//     mpp_frame_set_buffer(frame, g_frm_buf);
//     mpp_frame_set_hor_stride(frame, WIDTH);
//     mpp_frame_set_ver_stride(frame, HEIGHT);
    
//     // 计算PTS（基于30fps）
//     // 90kHz时基下，30fps时每帧间隔是 90000/30 = 3000
//     int64_t pts = frame_count * 3000;
//     mpp_frame_set_pts(frame, pts);
    
//     frame_count++;
    
//     /* 2. 把输出 buffer 包成 MppPacket */
//     mpp_packet_init(&packet, NULL, 0);
//     mpp_packet_set_buffer(packet, g_pkt_buf);
    
//     /* 3. 送帧-取包 */
//     MPP_RET ret;
//     ret = g_mpi->encode_put_frame(g_ctx, frame);
//     if (ret != MPP_OK) goto DONE;
    
//     ret = g_mpi->encode_get_packet(g_ctx, &packet);
//     if (ret != MPP_OK) goto DONE;
    
//     /* 4. 写文件 */
//     void   *ptr = mpp_packet_get_pos(packet);
//     size_t  len = mpp_packet_get_length(packet);
//     write(g_fd_h264, ptr, len);
    
// DONE:
//     mpp_frame_deinit(&frame);
//     mpp_packet_deinit(&packet);
//     return (ret == MPP_OK) ? len : -1;
// }

// void MPP_Enc_Wrapper_Init(size_t nv21_size)
// {
//     MPP_RET Ret_Creat = mpp_create(&g_ctx, &g_mpi);
//     if(Ret_Creat != MPP_OK)
//     {
//         printf("MPP创建失败\n");
//         return;
//     }
//     printf("MPP创建成功\n");
    
//     MPP_RET Ret_Init = mpp_init(g_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
//     if(Ret_Init != MPP_OK)
//     {
//         printf("MPP初始化失败\n");
//         return;
//     }
//     printf("MPP初始化成功\n");

//     // 配置
//     if (encoder_cfg_nv21() != 0) 
//     {
//         printf("MPP配置失败\n");
//         return;
//     }
//     printf("MPP配置成功\n");

//     g_frm_buf = NULL;                       // 全局变量清空
//     mpp_buffer_get(NULL, &g_frm_buf, nv21_size); // 重新 alloc
//     g_pkt_buf = NULL;
//     mpp_buffer_get(NULL, &g_pkt_buf, nv21_size * 2);
// }

// void MPP_Enc_Wrapper_Loop(size_t nv21_size)
// {
//     static uint8_t Encode_Avail_Buf = 0;
//     Encode_Avail_Buf = shm_ptr->sem.BGRA_Avail_Buf;
//     // printf("Encode_Avail_Buf:%d\n",Encode_Avail_Buf);
//     uint8_t* nv21_data_ptr = Get_Frame_Data_Offset(shm_ptr,NV21_TYPE ,Encode_Avail_Buf);
//     // if(shm_ptr->sem.BGRA_Avail_Buf  == 0)
//     //     Encode_Avail_Buf = 2;
//     // else 
//     //     Encode_Avail_Buf = shm_ptr->sem.BGRA_Avail_Buf -1;

//     uint8_t *dst = (uint8_t *)mpp_buffer_get_ptr(g_frm_buf);
//     memcpy(dst, nv21_data_ptr, nv21_size);  // 这里只是拷贝了数据
        
//     // 编码
//     int ret = MPP_Encode_One_Frame();
//     if (ret > 0) 
//     {
//         // printf("MPP编码成功，编码大小：%d\n", ret);
//     } 
//     else 
//     {
//         printf("MPP编码失败\n");
//     }
// }