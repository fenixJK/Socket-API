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
#include <string>
#include <chrono>
#include <vector>

class ConnectionHandler {
public:
    ConnectionHandler(SOCKET* sock, const std::string& host, int port) : isClient(true) {
        this->sock=*sock;
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
            perror("getaddrinfo");
            return;
        }
        if (connect(*sock, res->ai_addr, res->ai_addrlen) < 0) {
            perror("Unable to connect");
            return;
        }
        freeaddrinfo(res);
        connectionStartTime = std::chrono::system_clock::now();
    }
    ConnectionHandler(SOCKET* sock) : isClient(false) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client = accept(*sock, (struct sockaddr*)&client_addr, &client_len);
        if (client == INVALID_SOCKET) {
            perror("Unable to accept connection");
            return;
        }
        this->sock=client;
    }
    ~ConnectionHandler() {
        if (sock != INVALID_SOCKET) {
            #ifdef _WIN32
                if (!isClient) {
                    if (shutdown(sock, SD_BOTH) == SOCKET_ERROR) {
                        perror("Shutdown failed");
                        return;
                    }
                } else { closeSocket(sock); }
            #else
                if (!isClient) {
                    if (shutdown(sock, SHUT_RDWR) < 0) {
                        perror("Shutdown failed");
                        return;
                    }
                } else { closeSocket(sock); }
            #endif
        }
    }
    bool messageWaiting() {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        int retval = select(sock + 1, &read_fds, NULL, NULL, &tv);
        if (retval == -1) { perror("select failed"); return false; }
        else if (retval > 0) { return true; }
        return false;
    }
    std::string recieveMessage() {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) { return ""; }
        return std::string(buffer);
    }
    bool isDisconnected() {
        if (isClient) { return false; }
        char buffer[1];
        int bytesReceived = recv(sock, buffer, sizeof(buffer), MSG_PEEK);
        if (bytesReceived <= 0) { return true; }
        return false;
    }
    std::chrono::system_clock::time_point getStartTime() { return connectionStartTime; }
private:
    SOCKET sock;
    bool isClient;
    std::chrono::system_clock::time_point connectionStartTime;
    void closeSocket(SOCKET s) {
        #ifdef _WIN32
            closesocket(s);
        #else
            ::close(s);
        #endif
    }
};

class User {
public:
    User(const std::string name) {
        this->name = name;
        users.push_back(this);
    }
    ~User() {
        for (std::pair<ConnectionHandler*, std::chrono::system_clock::time_point> s : connections) { delete s.first; }
        for (auto it = users.begin(); it != users.end(); ) {
            *it == this ? it = users.erase(it) : ++it;
        }
    }
    void setPassword(const std::string& password) { this->password = password; }
    void setPasswordTimeLimit(std::chrono::minutes PasswordTimeLimit) { this->passwordTimeLimit = PasswordTimeLimit; }
    void setConnectionTimeLimit(std::chrono::minutes ConnectionTimeLimit) { this->connectionTimeLimit = ConnectionTimeLimit; }
    void changeUserName(const std::string newName) { this->name = newName; }
    void addConnection(ConnectionHandler* connection) { connections.push_back({ connection, std::chrono::system_clock::now() }); }
    void removeConnection(ConnectionHandler* connection) {
        for (auto it = connections.begin(); it != connections.end(); ) {
            (*it).first == connection ? it = connections.erase(it) : ++it;
        }
    }
    std::string getPassword() { return password; }
    std::string getName() { return name; }
    std::chrono::minutes getPasswordTimeLimit() { return passwordTimeLimit; }
    std::chrono::minutes getConnectionTimeLimit() { return connectionTimeLimit; }
    bool checkPassword(ConnectionHandler* connection) {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        while (std::chrono::system_clock::now() - start < passwordTimeLimit) {
            if (connection->messageWaiting()) {
                std::string receivedPassword = connection->recieveMessage();
                if (receivedPassword == password) return true;
            }
        }
        return false;
    }
    std::vector<std::pair<ConnectionHandler*, std::chrono::system_clock::time_point>> connections;
    static std::vector<User*> users;
private:
    std::string name;
    std::string password;
    std::chrono::minutes passwordTimeLimit;
    std::chrono::minutes connectionTimeLimit;
};
std::vector<User*> User::users;