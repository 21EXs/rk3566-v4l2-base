#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "main.h"
#include <shm.h>
#include <pthread.h>

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
static MppCtx  g_ctx = NULL;
static MppApi *g_mpi = NULL;

static MppBuffer g_frm_buf = NULL;
static MppBuffer g_pkt_buf = NULL;

extern int g_fd_h264; 

/* 配置函数：只负责写寄存器，不返回任何指针 */
int encoder_cfg_nv21(void)
{
    if (!g_ctx || !g_mpi)
        return -1;

    MppEncCfg cfg = NULL;
    mpp_enc_cfg_init(&cfg);                       // 1. 分配默认配置

    mpp_enc_cfg_set_s32(cfg, "prep:width",       WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:height",      HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",  WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",  HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:format",      MPP_FMT_YUV420SP);
    mpp_enc_cfg_set_s32(cfg, "rc:mode",          MPP_ENC_RC_MODE_CBR);
    mpp_enc_cfg_set_s32(cfg, "rc:bps",           WIDTH * HEIGHT * 2);
    mpp_enc_cfg_set_s32(cfg, "rc:fps",           30);

    MPP_RET ret = g_mpi->control(g_ctx, MPP_ENC_SET_CFG, cfg);
    mpp_enc_cfg_deinit(cfg);                      // 2. 立即释放
    return (ret == MPP_OK) ? 0 : -1;
}

MppCtx  get_mpp_ctx(void)
{

    return g_ctx; 
}

MppApi* get_mpp_api(void)  
{
    return g_mpi; 
}

int mpp_wrap_nv21_segment(uint8_t *nv21_data_ptr, size_t nv21_size)
{
    MppBufferInfo info = {
        .type = MPP_BUFFER_TYPE_NORMAL,
        .size = nv21_size,
        .ptr  = nv21_data_ptr,
    };
    int ret = mpp_buffer_import(&info, &g_frm_buf);
    printf("mpp_buffer_import ret=%d  (0=ok)\n", ret);
    return ret;
}

int MPP_Encode_One_Frame(void)
{
    MppFrame  frame  = NULL;
    MppPacket packet = NULL;

    /* 1. 把已注册好的输入 buffer 包成 MppFrame */
    mpp_frame_init(&frame);
    mpp_frame_set_width(frame, 640);
    mpp_frame_set_height(frame, 480);
    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
    mpp_frame_set_buffer(frame, g_frm_buf);   // 零拷贝

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

void MPP_Enc_Wrapper(uint8_t *nv21_data_ptr, size_t nv21_size)
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
    uint8_t *dst = (uint8_t *)mpp_buffer_get_ptr(g_frm_buf);
    memcpy(dst, nv21_data_ptr, nv21_size);  // 拷进去

    MPP_Encode_One_Frame();                 // 后面流程不变
    
    // 编码
    int ret = MPP_Encode_One_Frame();
    if (ret > 0) 
    {
        printf("MPP编码成功，编码大小：%d\n", ret);
    } 
    else 
    {
        printf("MPP编码失败\n");
    }
}