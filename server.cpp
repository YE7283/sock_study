#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> // inet_ntop를 사용하기 위해 추가
#include <process.h>
#include <vector>
#include <mutex>
#include <cstring> // memset 사용을 위해 추가

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024
#define MAX_CLNT 2

std::vector<SOCKET> clientSockets;
std::mutex mtx;

unsigned WINAPI HandleClient(void* arg);
void SendMsgToAll(const char* msg, int len);

int main() {
    WSADATA wsaData;
    SOCKET servSock, clntSock;
    SOCKADDR_IN servAddr, clntAddr;
    int clntAddrSize;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() error!" << std::endl;
        return -1;
    }

    servSock = socket(PF_INET, SOCK_STREAM, 0);
    if (servSock == INVALID_SOCKET) {
        std::cerr << "Socket creation error!" << std::endl;
        WSACleanup();
        return -1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(12345);

    if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind error!" << std::endl;
        closesocket(servSock);
        WSACleanup();
        return -1;
    }

    if (listen(servSock, MAX_CLNT) == SOCKET_ERROR) {
        std::cerr << "Listen error!" << std::endl;
        closesocket(servSock);
        WSACleanup();
        return -1;
    }

    std::cout << "Server is running..." << std::endl;

    while (true) { // 무한 루프를 사용하여 계속 클라이언트 연결을 수락
        clntAddrSize = sizeof(clntAddr);
        clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
        if (clntSock == INVALID_SOCKET) {
            std::cerr << "Accept error!" << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (clientSockets.size() >= MAX_CLNT) { // 클라이언트가 초과되었을 경우 연결 거부
                std::cerr << "Too many clients connected. Connection refused." << std::endl;
                closesocket(clntSock);
                continue;
            }

            clientSockets.push_back(clntSock);
        }

        char ipAddress[INET_ADDRSTRLEN]; // IPv4 주소를 저장할 버퍼
        inet_ntop(AF_INET, &clntAddr.sin_addr, ipAddress, INET_ADDRSTRLEN);
        std::cout << "Client connected: " << ipAddress << std::endl;

        _beginthreadex(NULL, 0, HandleClient, (void*)&clntSock, 0, NULL);
    }

    closesocket(servSock);
    WSACleanup();
    return 0;
}

unsigned WINAPI HandleClient(void* arg) {
    SOCKET clntSock = *((SOCKET*)arg);
    char msg[BUF_SIZE];
    int strLen;

    while ((strLen = recv(clntSock, msg, sizeof(msg) - 1, 0)) > 0) {
        msg[strLen] = '\0';
        SendMsgToAll(msg, strLen);
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clntSock), clientSockets.end());
    }

    closesocket(clntSock);
    std::cout << "Client disconnected." << std::endl;
    return 0;
}

void SendMsgToAll(const char* msg, int len) {
    std::lock_guard<std::mutex> lock(mtx);
    for (SOCKET sock : clientSockets) {
        send(sock, msg, len, 0);
    }
}
