#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <thread>

#define BUF_SIZE 1024

void handleClient(SOCKET clientSocket, SOCKET otherClientSocket) 
{
    char buffer[BUF_SIZE];
    while (true) 
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "A client disconnected.\n";
            break;
        }
        std::cout << "Received: " << buffer << "\n";
        send(otherClientSocket, buffer, bytesReceived, 0);
    }
    closesocket(clientSocket);
}

int main() 
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cout << "Winsock initialization failed.\n";
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) 
    {
        std::cout << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    // 서버 정보
    sockaddr_in serverAddr{};   
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // 바인딩
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        std::cout << "Binding failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // listen
    if (listen(serverSocket, 2) == SOCKET_ERROR) 
    {
        std::cout << "Listening failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Waiting for two clients to connect...\n";
    sockaddr_in clientAddr1{}, clientAddr2{};
    int clientLen1 = sizeof(clientAddr1);
    int clientLen2 = sizeof(clientAddr2);

    SOCKET clientSocket1 = accept(serverSocket, (sockaddr*)&clientAddr1, &clientLen1);
    if (clientSocket1 == INVALID_SOCKET) 
    {
        std::cout << "Client 1 connection failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    std::cout << "Client 1 connected.\n";

    SOCKET clientSocket2 = accept(serverSocket, (sockaddr*)&clientAddr2, &clientLen2);
    if (clientSocket2 == INVALID_SOCKET) 
    {
        std::cout << "Client 2 connection failed.\n";
        closesocket(clientSocket1);
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    std::cout << "Client 2 connected.\n";

    std::thread t1(handleClient, clientSocket1, clientSocket2);
    std::thread t2(handleClient, clientSocket2, clientSocket1);

    t1.join();
    t2.join();

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
