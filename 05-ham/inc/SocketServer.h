#ifndef __SOCKET_SERVER_H
#define __SOCKET_SERVER_H

#include <stdio.h>     
#include <stdlib.h>   
#include <string.h>    
#include <sys/types.h>  
#include <sys/socket.h> 
#include <sys/un.h>   
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <errno.h>   
#include <fcntl.h>

#define GUI_SOCKET_PATH      "/tmp/gui.sock"          // GUI服务器
#define FUSA_SOCKET_PATH     "/tmp/fusa.sock"         // fusa进程
#define V4L2_SOCKET_PATH     "/tmp/v4l2.sock"         // 采集进程
#define TRANSFER_SOCKET_PATH "/tmp/transfer.sock"     // 中转进程  
#define DISPLAY_SOCKET_PATH  "/tmp/display.sock"      // 显示进程
#define ENCODER_SOCKET_PATH  "/tmp/encoder.sock"      // 编码推流进程
#define HAM_SOCKET_PATH      "/tmp/ham.sock"          // ham监控进程
#define FUSA_SOCKET_PATH     "/tmp/fusa.sock"         // fusa监控进程

int SocketServer_Init();

#endif


