#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>

#define BUF_SIZE 1024

const char* IP_ADR = "127.0.0.1";

void receiveMessages(SOCKET serverSocket) 
{
    char buffer[BUF_SIZE];
    while (true) 
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) 
        {
            std::cout << "Server disconnected.\n";
            break;
        }
        std::cout << "Message: " << buffer << std::endl;
    }
}

int main() 
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "Winsock initialization failed.\n";
        return -1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) 
    {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, IP_ADR, &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        std::cerr << "Connection to server failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to the server!\n";
    std::thread receiver(receiveMessages, clientSocket);

    std::string message;
    while (true) 
    {
        std::getline(std::cin, message);
        if (message == "exit") break;
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    closesocket(clientSocket);
    receiver.join();
    WSACleanup();
    return 0;
}
