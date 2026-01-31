#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "main.h"
#include <shm.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_buffer.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>

#include "MPP_Wrapper.h"

size_t nv21_size = WIDTH * HEIGHT * 3 / 2;
static int64_t start_time = 0;

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
    // enc->bitrate = width * height * 7.5;//码率
    enc->bitrate =300000;//码率

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
    uint8_t *h264_data = (uint8_t *)encoded_data;
    if (enc->output_fd >= 0 && encoded_size > 0) 
    {
        int is_keyframe = 0;
        if (encoded_size >= 5 && (h264_data[4] & 0x1F) == 5)
        {
            is_keyframe = 1;
        }

                if (is_keyframe && !enc->got_sps_pps) 
        {
            MppPacket header_packet = NULL;
            ret = enc->mpi->control(enc->ctx, MPP_ENC_GET_EXTRA_INFO, &header_packet);
            if (ret == MPP_OK && header_packet) 
            {
                void *sps_pps = mpp_packet_get_pos(header_packet);
                size_t sps_pps_size = mpp_packet_get_length(header_packet);
                
                mpp_packet_deinit(&header_packet);
                enc->got_sps_pps = 1;
            }
        }
        
        // 发送H.264数据
        // Push_H264_Stream(h264_data, encoded_size);

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
  
