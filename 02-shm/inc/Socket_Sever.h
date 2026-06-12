#ifndef __SOCKET_SERVER_H_
#define __SOCKET_SERVER_H_

#include <stdint.h>

// ActionCode 定义（与 Qt 端保持一致）
#define ACTION_PHOTO        1001    // Qt → 后端：触发拍照
#define ACTION_PHOTO_RESULT 1002    // 后端 → Qt：拍照完成通知
#define ACTION_RECORD_START 2001    // Qt → 后端：开始录像（预留）
#define ACTION_RECORD_STOP  2002    // Qt → 后端：停止录像（预留）

// 设置共享内存指针（由 main.c 调用）
void Socket_SetShmPtr(void *shm_ptr);

int Socket_Server_Init();
int Socket_Server_Listen();
int Socket_Server_ProcessClient();
void Socket_Server_CloseClient();
void Socket_Server_Shutdown();

#endif
