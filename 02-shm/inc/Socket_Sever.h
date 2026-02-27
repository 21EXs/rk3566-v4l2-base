#ifndef __SOCKET_SERVER_H_
#define __SOCKET_SERVER_H_

int Socket_Server_Init() ;
int Socket_Server_Listen() ;
int Socket_Server_ProcessClient();
void Socket_Server_CloseClient();
void Socket_Server_Shutdown();

#endif