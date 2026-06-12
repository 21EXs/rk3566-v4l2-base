#include "Socket_Client.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <memory>

#define SOCKET_SERVER_PATH "/tmp/unix_socket_server.sock"

int SocketClient::Socket_Client_Init()
{
    if ((client_gui_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        std::cout << "socket创建失败" << std::endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_SERVER_PATH, sizeof(server_addr.sun_path));

    if(connect(client_gui_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << "连接失败" << std::endl;
        close(client_gui_fd);
        return -1;
    }

    return client_gui_fd;
}

ssize_t SocketClient::SendData(const char* data)
{
    if (client_gui_fd < 0) {
        std::cout << "错误：socket未连接" << std::endl;
        return -1;
    }

    if (data == nullptr) {
        std::cout << "错误：发送数据为空" << std::endl;
        return -1;
    }

    size_t length = strlen(data);
    ssize_t bytes_sent = write(client_gui_fd, data, length);

    if (bytes_sent < 0) {
        std::cout << "发送失败: " << strerror(errno) << std::endl;
    } else if (bytes_sent == 0) {
        std::cout << "警告：发送了0字节" << std::endl;
    } else {
        std::cout << "发送成功，字节数: " << bytes_sent << std::endl;
    }

    return bytes_sent;
}

ssize_t SocketClient::RecvData(char* buffer, size_t bufsize)
{
    if (client_gui_fd < 0) {
        std::cout << "错误：socket未连接" << std::endl;
        return -1;
    }

    if (buffer == nullptr || bufsize == 0) {
        std::cout << "错误：接收缓冲区无效" << std::endl;
        return -1;
    }

    memset(buffer, 0, bufsize);
    ssize_t bytes_received = read(client_gui_fd, buffer, bufsize - 1);

    if (bytes_received < 0) {
        std::cout << "接收失败: " << strerror(errno) << std::endl;
    } else if (bytes_received == 0) {
        std::cout << "连接已关闭" << std::endl;
    } else {
        buffer[bytes_received] = '\0';
        std::cout << "收到响应: " << buffer << std::endl;
    }

    return bytes_received;
}
