#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace Nova {
    struct Request {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    struct Response {
        int status = 200;
        std::string contentType = "text/html";
        std::string body;
    };

    class Server {
    private:
        int port;
        SOCKET server_fd;
    public:
        Server(int p) : port(p), server_fd(INVALID_SOCKET) {}
        void start();
    };
}
