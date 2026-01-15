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
#include </home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release/external/mpp/osal/inc/mpp_mem.h>
#include <rockchip/rk_mpi.h>


typedef struct MpiEncTestArgs_t {
    char                *file_input;
    char                *file_output;
    char                *file_cfg;

    MppCodingType       type;
    MppCodingType       type_src;       /* for file source input */
    MppFrameFormat      format;
    RK_S32              frame_num;
    RK_S32              loop_cnt;
    RK_S32              nthreads;

    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    /* -rc */
    RK_S32              rc_mode;

    /* -bps */
    RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;

    /* -fps */
    RK_S32              fps_in_flex;
    RK_S32              fps_in_num;
    RK_S32              fps_in_den;
    RK_S32              fps_out_flex;
    RK_S32              fps_out_num;
    RK_S32              fps_out_den;

    /* -qc */
    RK_S32              qp_init;
    RK_S32              qp_min;
    RK_S32              qp_max;
    RK_S32              qp_min_i;
    RK_S32              qp_max_i;

    /* -g gop mode */
    RK_S32              gop_mode;
    RK_S32              gop_len;
    RK_S32              vi_len;

    /* -v q runtime log disable flag */
    RK_U32              quiet;
    /* -v f runtime fps log flag */
    RK_U32              trace_fps;
    RK_U32              psnr_en;
    RK_U32              ssim_en;
    char                *file_slt;
} MpiEncTestArgs;

typedef RK_S32 (*OptParser)(void *ctx, const char *next);

typedef struct MppOptInfo_t {
    const char*     name;
    const char*     full_name;
    const char*     help;
    OptParser       proc;
} MppOptInfo;

typedef void* MppOpt;

void MPP_Enc_Wrapper_Init(size_t nv21_size);
void MPP_Enc_Wrapper_Loop(size_t nv21_size);