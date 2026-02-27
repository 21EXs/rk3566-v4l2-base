#include "SocketServer.h"

#define MAX_CLIENTS 10

static int client_fds[MAX_CLIENTS] = {0};
static int client_count = 0;
static int server_sockfd = -1;

int SocketServer_Init() 
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        fprintf(stderr, "socket创建失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 设置SO_REUSEADDR
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // 删除已存在的socket文件
    unlink(HAM_SOCKET_PATH);
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, HAM_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        fprintf(stderr, "bind失败: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 5) < 0) 
    {
        fprintf(stderr, "listen失败: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    printf("服务器监听在Unix域套接字: %s\n", HAM_SOCKET_PATH);
    server_sockfd = sockfd;
    
    // 设置socket为非阻塞
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    
    while(1) 
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        
        int max_fd = sockfd;
        
        // 添加所有客户端fd到select集合
        for (int i = 0; i < client_count; i++) 
        {
            if (client_fds[i] > 0) 
            {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) 
                {
                    max_fd = client_fds[i];
                }
            }
        }
        
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0) 
        {
            perror("select error");
            continue;
        }
        
        // 检查新连接
        if (FD_ISSET(sockfd, &read_fds)) 
        {
            int client_fd = accept(sockfd, NULL, NULL);
            if (client_fd >= 0) 
            {
                // 设置客户端socket为非阻塞
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                
                // 添加到客户端列表
                if (client_count < MAX_CLIENTS) 
                {
                    client_fds[client_count] = client_fd;
                    client_count++;
                    printf("新客户端连接: fd=%d, 总客户端数: %d\n", client_fd, client_count);
                } 
                else 
                {
                    fprintf(stderr, "客户端数量已达上限，拒绝连接\n");
                    close(client_fd);
                }
            }
        }
        
        // 检查所有客户端的数据
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && FD_ISSET(client_fd, &read_fds)) 
            {
                char buffer[1024];
                int n = read(client_fd, buffer, sizeof(buffer) - 1);
                
                if (n > 0) 
                {
                    buffer[n] = '\0';
                    printf("收到客户端[%d]消息: %s\n", client_fd, buffer);
                    
                    // 处理命令并广播
                    Process_Client_Command(buffer, n, client_fd);
                } 
                else if (n == 0) 
                {
                    // 客户端断开连接
                    printf("客户端[%d]断开连接\n", client_fd);
                    close(client_fd);
                    
                    // 从数组中移除
                    for (int j = i; j < client_count - 1; j++) 
                    {
                        client_fds[j] = client_fds[j + 1];
                    }
                    client_count--;
                    client_fds[client_count] = 0;
                    i--;  // 调整索引
                } 
                else 
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) 
                    {
                        perror("read error");
                    }
                }
            }
        }
    }
    
    close(sockfd);
    unlink(HAM_SOCKET_PATH);
    return 0;
}

int Process_Client_Command(const char *receivedData, int dataLen, int sender_fd)
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // 安全拷贝
    int copy_len = dataLen < sizeof(buffer) - 1 ? dataLen : sizeof(buffer) - 1;
    strncpy(buffer, receivedData, copy_len);
    buffer[copy_len] = '\0';
    
    // 去除换行符
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';
    
    printf("处理命令: %s (来自客户端: %d)\n", buffer, sender_fd);
    
    if (strcmp(buffer, "photo") == 0) 
    {
        printf("广播photo命令给其他客户端...\n");
        
        // 广播给所有其他客户端
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && client_fd != sender_fd) 
            {
                printf("  发送photo到客户端[%d]\n", client_fd);
                int written = write(client_fd, "photo", 5);
                if (written <= 0) 
                {
                    printf("  发送失败到客户端[%d]\n", client_fd);
                }
            }
        }
        printf("广播完成\n");
    }
    else if(strcmp(buffer, "start") == 0) 
    {
        printf("广播start命令给其他客户端...\n");
        
        // 广播给所有其他客户端
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && client_fd != sender_fd) 
            {
                printf("  发送start到客户端[%d]\n", client_fd);
                int written = write(client_fd, "start", 5);
                if (written <= 0) 
                {
                    printf("  发送失败到客户端[%d]\n", client_fd);
                }
            }
        }
        printf("广播完成\n");
    }
    else if(strcmp(buffer, "stop") == 0) 
    {
        printf("广播stop命令给其他客户端...\n");
        
        // 广播给所有其他客户端
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && client_fd != sender_fd) 
            {
                printf("  发送stop到客户端[%d]\n", client_fd);
                int written = write(client_fd, "stop", 5);
                if (written <= 0) 
                {
                    printf("  发送失败到客户端[%d]\n", client_fd);
                }
            }
        }
        printf("广播完成\n");
    }
    else if(strcmp(buffer, "Streaming") == 0) 
    {
        printf("广播Streaming命令给其他客户端...\n");
        
        // 广播给所有其他客户端
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && client_fd != sender_fd) 
            {
                printf("  发送Streaming到客户端[%d]\n", client_fd);
                int written = write(client_fd, "Streaming", 5);
                if (written <= 0) 
                {
                    printf("  发送失败到客户端[%d]\n", client_fd);
                }
            }
        }
        printf("广播完成\n");
    }
    else if(strcmp(buffer, "UnStream") == 0) 
    {
        printf("广播UnStream命令给其他客户端...\n");
        
        // 广播给所有其他客户端
        for (int i = 0; i < client_count; i++) 
        {
            int client_fd = client_fds[i];
            if (client_fd > 0 && client_fd != sender_fd) 
            {
                printf("  发送UnStream到客户端[%d]\n", client_fd);
                int written = write(client_fd, "UnStream", 5);
                if (written <= 0) 
                {
                    printf("  发送失败到客户端[%d]\n", client_fd);
                }
            }
        }
        printf("广播完成\n");
    }
    else 
    {
        printf("未知命令: %s\n", buffer);
    }
    
    return 0;
}