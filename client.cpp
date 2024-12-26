#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> // inet_pton 사용을 위한 헤더
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

unsigned WINAPI RecvMsg(void* arg);
void ErrorHandling(const char* msg);

int main() {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN servAddr;
    char msg[BUF_SIZE];
    HANDLE hRecvThread;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ErrorHandling("WSAStartup() error!");
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        ErrorHandling("Socket creation error!");
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(12345);

    // inet_addr 대신 inet_pton 사용
    if (inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr) <= 0) {
        ErrorHandling("Invalid address or address not supported!");
    }

    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        ErrorHandling("Connect error!");
    }

    std::cout << "Connected to server." << std::endl;

    hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&sock, 0, NULL);
    while (true) {
        std::cin.getline(msg, BUF_SIZE);
        if (!strcmp(msg, "q") || !strcmp(msg, "Q")) {
            shutdown(sock, SD_BOTH);
            closesocket(sock);
            break;
        }
        send(sock, msg, strlen(msg), 0);
    }

    WaitForSingleObject(hRecvThread, INFINITE);
    WSACleanup();
    return 0;
}

unsigned WINAPI RecvMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg);
    char msg[BUF_SIZE];
    int strLen;

    while ((strLen = recv(sock, msg, BUF_SIZE - 1, 0)) > 0) {
        msg[strLen] = '\0';
        std::cout << "Message: " << msg << std::endl;
    }
    return 0;
}

void ErrorHandling(const char* msg) {
    std::cerr << msg << std::endl;
    exit(1);

}
