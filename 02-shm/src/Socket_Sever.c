#include "Socket_Sever.h"
#include "shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define SOCKET_SERVER_PATH "/tmp/unix_socket_server.sock"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5

// 全局变量
static int server_fd = -1;
static int client_fd = -1;
static struct sockaddr_un server_addr, client_addr;
static struct shared_memory *g_shm_ptr = NULL;  // 共享内存指针

void cleanup_socket(const char *path) 
{
    if (unlink(path) == -1 && errno != ENOENT) 
    {
        perror("unlink");
    }
}

void signal_handler(int sig) 
{
    if (sig == SIGINT || sig == SIGTERM) 
    {
        printf("\n收到终止信号，清理资源...\n");
        
        if (client_fd != -1) {
            close(client_fd);
        }
        
        if (server_fd != -1) {
            close(server_fd);
        }
        
        cleanup_socket(SOCKET_SERVER_PATH);
        exit(0);
    }
}

// 设置共享内存指针
void Socket_SetShmPtr(void *shm_ptr)
{
    g_shm_ptr = (struct shared_memory *)shm_ptr;
}

// 保存 BGRA 数据为 PPM 文件（简单可靠，无需 libjpeg）
// PPM 格式：P6\n宽度 高度\n255\nRGB数据...
static int save_bgra_to_ppm(const char *filename, uint8_t *bgra_data, int width, int height)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("打开文件失败");
        return -1;
    }

    // PPM 头
    fprintf(fp, "P6\n%d %d\n255\n", width, height);

    // BGRA → RGB（去掉 A 通道，交换 B 和 R）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            uint8_t b = bgra_data[idx + 0];
            uint8_t g = bgra_data[idx + 1];
            uint8_t r = bgra_data[idx + 2];
            // uint8_t a = bgra_data[idx + 3]; // 忽略 Alpha
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    fclose(fp);
    return 0;
}

// 执行拍照
static int take_photo()
{
    if (!g_shm_ptr) {
        fprintf(stderr, "[拍照] 共享内存未初始化\n");
        return -1;
    }

    // 从共享内存读取当前 BGRA 帧（已旋转为 720×1280）
    uint8_t *bgra_data = Get_Frame_Data_Offset(g_shm_ptr, BGRA_TYPE, g_shm_ptr->sem.BGRA_Avail_Buf);
    if (!bgra_data) {
        fprintf(stderr, "[拍照] 获取 BGRA 数据失败\n");
        return -1;
    }

    // 生成文件名：/mnt/photo_YYYYMMDD_HHMMSS.ppm
    char filename[128];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "/mnt/photo_%Y%m%d_%H%M%S.ppm", tm_info);

    // 旋转后的尺寸：width=HEIGHT=720, height=WIDTH=1280
    int ret = save_bgra_to_ppm(filename, bgra_data, HEIGHT, WIDTH);
    if (ret == 0) {
        printf("[拍照] 成功保存: %s (%dx%d)\n", filename, HEIGHT, WIDTH);
    } else {
        fprintf(stderr, "[拍照] 保存失败\n");
    }

    return ret;
}

// 初始化服务器
int Socket_Server_Init() 
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    cleanup_socket(SOCKET_SERVER_PATH);
    
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket创建失败");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_SERVER_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("绑定失败");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    
    chmod(SOCKET_SERVER_PATH, 0777);
    
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        perror("监听失败");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    
    printf("Unix域Socket服务器已启动，监听路径: %s\n", SOCKET_SERVER_PATH);
    printf("按Ctrl+C停止服务器\n");
    
    return server_fd;
}

// 监听并接受客户端连接
int Socket_Server_Listen() 
{
    socklen_t client_len;
    
    if (server_fd == -1) 
    {
        fprintf(stderr, "服务器未初始化\n");
        return -1;
    }
    
    printf("等待客户端连接...\n");
    
    client_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    
    if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) 
    {
        perror("接受连接失败");
        return -1;
    }
    
    printf("客户端已连接\n");
    return client_fd;
}

// 处理客户端消息（基于 ActionCode 协议）
int Socket_Server_ProcessClient()
{
    char buffer[BUFFER_SIZE];
    
    if (client_fd == -1) 
    {
        fprintf(stderr, "没有活动的客户端连接\n");
        return -1;
    }
    
    ssize_t bytes_received;
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) 
    {
        buffer[bytes_received] = '\0';
        printf("收到消息: %s\n", buffer);
        
        int action_code = atoi(buffer);
        
        switch (action_code) 
        {
            case ACTION_PHOTO:  // 1001: 拍照
            {
                printf("[Action] 拍照请求\n");
                int ret = take_photo();
                
                // 响应: "1002:1" 成功, "1002:0" 失败
                char response[32];
                snprintf(response, sizeof(response), "%d:%d", ACTION_PHOTO_RESULT, (ret == 0) ? 1 : 0);
                send(client_fd, response, strlen(response), 0);
                printf("[Action] 拍照响应: %s\n", response);
                break;
            }

            case ACTION_RECORD_START:  // 2001: 开始录像（预留）
                printf("[Action] 开始录像请求（预留，未实现）\n");
                break;

            case ACTION_RECORD_STOP:   // 2002: 停止录像（预留）
                printf("[Action] 停止录像请求（预留，未实现）\n");
                break;

            default:
                printf("[Action] 未知 ActionCode: %d\n", action_code);
                break;
        }
    }
    
    if (bytes_received == -1) 
    {
        perror("接收失败");
    }
    
    printf("客户端断开连接\n");
    return 0;
}

void Socket_Server_CloseClient()
{
    if (client_fd != -1) 
    {
        close(client_fd);
        client_fd = -1;
        printf("客户端连接已关闭\n");
    }
}

void Socket_Server_Shutdown()
{
    printf("正在关闭服务器...\n");
    Socket_Server_CloseClient();
    
    if (server_fd != -1) 
    {
        close(server_fd);
        server_fd = -1;
        printf("服务器已关闭\n");
    }
    
    cleanup_socket(SOCKET_SERVER_PATH);
}
