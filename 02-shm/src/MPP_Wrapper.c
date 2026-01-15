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
// #include <rockchip/rk_type.h>
// #include <rockchip/mpp_err.h>
// #include <rockchip/mpp_log.h>
// #include <rockchip/mpp_meta.h>
// #include <rockchip/mpp_task.h>

#include "MPP_Wrapper.h"

/* 让 ctx/mpi 在本文件内全局可见，供后续 encode 使用 */
static MppCtx  g_ctx = NULL;//上下文句柄
static MppApi *g_mpi = NULL;//API函数表

static MppBuffer g_frm_buf = NULL;
static MppBuffer g_pkt_buf = NULL;

extern int g_fd_h264; 
extern struct shared_memory *shm_ptr;

static uint64_t pts_counter = 0;

static uint64_t get_current_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}


/* 配置函数：只负责写寄存器，不返回任何指针 */
int encoder_cfg_nv21(void)
{
    if (!g_ctx || !g_mpi)
        return -1;

    MppEncCfg cfg = NULL;
    mpp_enc_cfg_init(&cfg);                       // 1. 分配默认配置

    mpp_enc_cfg_set_s32(cfg, "prep:width",       WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:height",      HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",  WIDTH);//内存对齐
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",  HEIGHT);//内存对齐
    mpp_enc_cfg_set_s32(cfg, "prep:format",      MPP_FMT_YUV420SP_VU);
    mpp_enc_cfg_set_s32(cfg, "rc:mode",          MPP_ENC_RC_MODE_CBR);// 恒定码率模式
    mpp_enc_cfg_set_s32(cfg, "rc:bps",           WIDTH * HEIGHT * 7.5);// 目标码率
    mpp_enc_cfg_set_s32(cfg, "rc:fps",           30);
    mpp_enc_cfg_set_s32(cfg, "rc:gop ",          30);

    MPP_RET ret = g_mpi->control(g_ctx, MPP_ENC_SET_CFG, cfg);
    mpp_enc_cfg_deinit(cfg);                      // 2. 立即释放
    return (ret == MPP_OK) ? 0 : -1;
}

int MPP_Encode_One_Frame(void)
{
    MppFrame  frame  = NULL;
    MppPacket packet = NULL;

    /* 1. 把已注册好的输入 buffer 包成 MppFrame */
    mpp_frame_init(&frame);
    mpp_frame_set_width(frame, WIDTH);
    mpp_frame_set_height(frame, HEIGHT);
    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP_VU);
    mpp_frame_set_buffer(frame, g_frm_buf);   // 零拷贝
    mpp_frame_set_hor_stride(frame, WIDTH);
    mpp_frame_set_ver_stride(frame, HEIGHT);
    mpp_frame_set_pts(frame, pts_counter);
    pts_counter += 33333;  // 1000000微秒/30帧 ≈ 33333
  

    /* 2. 把输出 buffer 包成 MppPacket */
    mpp_packet_init(&packet, NULL, 0);
    mpp_packet_set_buffer(packet, g_pkt_buf);

    /* 3. 送帧-取包 */
    MPP_RET ret;
    ret = g_mpi->encode_put_frame(g_ctx, frame);
    if (ret != MPP_OK) goto DONE;

    ret = g_mpi->encode_get_packet(g_ctx, &packet);
    if (ret != MPP_OK) goto DONE;

    /* 4. 写文件 */
    void   *ptr = mpp_packet_get_pos(packet);
    size_t  len = mpp_packet_get_length(packet);
    write(g_fd_h264, ptr, len);   // g_fd_h264 提前 open("out.h264",...)

DONE:
    mpp_frame_deinit(&frame);
    mpp_packet_deinit(&packet);
    return (ret == MPP_OK) ? len : -1;
}

void MPP_Enc_Wrapper_Init(size_t nv21_size)
{
    MPP_RET Ret_Creat = mpp_create(&g_ctx, &g_mpi);
    if(Ret_Creat != MPP_OK)
    {
        printf("MPP创建失败\n");
        return;
    }
    printf("MPP创建成功\n");
    
    MPP_RET Ret_Init = mpp_init(g_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
    if(Ret_Init != MPP_OK)
    {
        printf("MPP初始化失败\n");
        return;
    }
    printf("MPP初始化成功\n");

    // 配置
    if (encoder_cfg_nv21() != 0) 
    {
        printf("MPP配置失败\n");
        return;
    }
    printf("MPP配置成功\n");

    g_frm_buf = NULL;                       // 全局变量清空
    mpp_buffer_get(NULL, &g_frm_buf, nv21_size); // 重新 alloc
    g_pkt_buf = NULL;
    mpp_buffer_get(NULL, &g_pkt_buf, nv21_size * 2);
}

void MPP_Enc_Wrapper_Loop(size_t nv21_size)
{
    static uint8_t Encode_Avail_Buf = 0;
    // printf("Encode_Avail_Buf:%d\n",Encode_Avail_Buf);
    uint8_t* nv21_data_ptr = Get_Frame_Data_Offset(shm_ptr,NV21_TYPE ,Encode_Avail_Buf);
    if(shm_ptr->sem.BGRA_Avail_Buf  == 0)
        Encode_Avail_Buf = 2;
    else 
        Encode_Avail_Buf = shm_ptr->sem.BGRA_Avail_Buf -1;

    uint8_t *dst = (uint8_t *)mpp_buffer_get_ptr(g_frm_buf);
    memcpy(dst, nv21_data_ptr, nv21_size);  // 这里只是拷贝了数据
        
    // 编码
    int ret = MPP_Encode_One_Frame();
    if (ret > 0) 
    {
        // printf("MPP编码成功，编码大小：%d\n", ret);
    } 
    else 
    {
        printf("MPP编码失败\n");
    }
}