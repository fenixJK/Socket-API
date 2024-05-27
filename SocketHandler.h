#ifndef MULTIPLAT_INCLUDE
#define MULTIPLAT_INCLUDE
#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#endif
#endif

class SocketHandler {
public:
    SocketHandler(int port) {
        this->port=port;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            perror("Unable to create socket");
            return;
        }
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            perror("Unable to bind");
            return;
        }
        if (listen(sock, 1) == SOCKET_ERROR) {
            perror("Unable to listen");
            return;
        }
    }
    ~SocketHandler() { closeSocket(sock); }
    SOCKET* getSocket() { return &sock; }
    int* getPort() { return &port; }
private:
    SOCKET sock;
    int port;
    void closeSocket(SOCKET s) {
        #ifdef _WIN32
            closesocket(s);
        #else
            ::close(s);
        #endif
    }
};