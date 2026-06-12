#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <stdexcept>

class SocketClient
{
public:
    int Socket_Client_Init();
    ssize_t SendData(const char* data);
    ssize_t RecvData(char* buffer, size_t bufsize);

private:
    int client_gui_fd;
    struct sockaddr_un server_addr;
};

#endif // SOCKET_CLIENT_H
