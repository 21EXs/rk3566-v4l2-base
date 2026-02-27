#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_SERVER_PATH "/tmp/unix_socket_server.sock"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5

// 全局变量（在实际项目中可以考虑使用结构体封装）
static int server_fd = -1;
static int client_fd = -1;
static struct sockaddr_un server_addr, client_addr;

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

// 初始化服务器，返回服务器文件描述符
int Socket_Server_Init() 
{
    // 设置信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 清理可能的旧socket文件
    cleanup_socket(SOCKET_SERVER_PATH);
    
    // 创建Unix域socket
    /* AF_UNIX: 指定Unix域socket
       SOCK_STREAM: 流式套接字
       0: 默认协议*/
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket创建失败");
        return -1;
    }
    
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_SERVER_PATH, sizeof(server_addr.sun_path) - 1);
    
    // 绑定socket到地址
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("绑定失败");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    
    // 设置socket文件权限（可选）
    chmod(SOCKET_SERVER_PATH, 0777);
    
    // 监听连接
    if (listen(server_fd, MAX_CLIENTS) == -1) //将主动套接字转为被动模式：让服务器准备好接受客户端的连接请求；设置等待队列：MAX_CLIENTS指定了等待处理连接的最大队列长度
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

// 监听并处理客户端连接（阻塞式）
int Socket_Server_Listen() 
{
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    
    if (server_fd == -1) 
    {
        fprintf(stderr, "服务器未初始化或初始化失败\n");
        return -1;
    }
    
    printf("等待客户端连接...\n");
    
    // 接受客户端连接
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

// 处理客户端消息
int Socket_Server_ProcessClient()
{
    char buffer[BUFFER_SIZE];
    
    if (client_fd == -1) 
    {
        fprintf(stderr, "没有活动的客户端连接\n");
        return -1;
    }
    
    // 处理客户端请求
    ssize_t bytes_received;
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) 
    {
        buffer[bytes_received] = '\0';
        printf("收到消息: %s\n", buffer);
        
        // 简单回显
        if (send(client_fd, buffer, bytes_received, 0) == -1) 
        {
            perror("发送失败");
            break;
        }
        
        if (strcmp(buffer, "photo") == 0) 
        {

            printf("客户端请求拍照\n");
            break;
        }
        if (strcmp(buffer, "stream") == 0) 
        {

            printf("客户端请求编码\n");
            break;
        }
        if (strcmp(buffer, "push") == 0) 
        {

            printf("客户端请求退流\n");
            break;
        }

        // 如果是退出命令
        if (strcmp(buffer, "exit") == 0) 
        {
            printf("客户端请求断开连接\n");
            break;
        }
        
        if (strcmp(buffer, "shutdown") == 0) 
        {
            printf("收到关机命令\n");
            close(client_fd);
            close(server_fd);
            cleanup_socket(SOCKET_SERVER_PATH);
            exit(0);
        }
    }
    
    if (bytes_received == -1) 
    {
        perror("接收失败");
    }
    
    return 0;
}

// 关闭客户端连接
void Socket_Server_CloseClient()
{
    if (client_fd != -1) 
    {
        close(client_fd);
        client_fd = -1;
        printf("客户端连接已关闭\n");
    }
}

// 关闭服务器
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

void PhotoCallBack()
{

}

void StreamCallBack()
{
    
}

void PushCallBack()
{
    
}