#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUF_SIZE 1024

struct rw_custom
{
	SOCKET socket;
	char* msg;
};

unsigned WINAPI recv_msg(void* arg);
unsigned WINAPI send_msg(void* arg);
void ErrorHandling(const char* message);

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSocket;
	char wbuffer[BUF_SIZE];
	char rbuffer[BUF_SIZE];

	int strLen;
	SOCKADDR_IN servAdr;

	HANDLE hRecv;
	HANDLE hSend;
	unsigned threadID;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(argv[1]);
	servAdr.sin_port = htons(atoi(argv[2]));


	if (connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error!");
	else
		puts("Connected...........");


	struct rw_custom rcustom = { hSocket, rbuffer };
	struct rw_custom wcustom = { hSocket, wbuffer };

	hRecv = (HANDLE)_beginthreadex(NULL, 0, recv_msg, (void*)&rcustom, 0, &threadID);
	if (hRecv == 0) {
		puts("_beginthreadex() error");
		return -1;
	}
	hSend = (HANDLE)_beginthreadex(NULL, 0, send_msg, (void*)&wcustom, 0, &threadID);
	if (hSend == 0) {
		puts("_beginthreadex() error");
		return -1;
	}

	WaitForSingleObject(hRecv, INFINITE);
	WaitForSingleObject(hSend, INFINITE);

	closesocket(hSocket);
	WSACleanup();
	return 0;
}

void ErrorHandling(const char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


unsigned WINAPI recv_msg(void* arg)
{
	struct rw_custom rwcustom = *((struct rw_custom*)arg);
	int strLen;
	// 클라이언트 메시지 수신
	while (1)
	{
		strLen = recv(rwcustom.socket, rwcustom.msg, BUF_SIZE, 0);
		if (strLen <= 0) {
			printf("Connection closed by client");
			return -1;
		}
		rwcustom.msg[strLen] = 0;
		printf("Client : %s", rwcustom.msg);
	}

	return 0;
}

unsigned WINAPI send_msg(void* arg)
{
	struct rw_custom rwcustom = *((struct rw_custom*)arg);

	while (1)
	{
		fputs("Server(Q to quit): ", stdout);
		fgets(rwcustom.msg, BUF_SIZE, stdin);
		if (!strcmp(rwcustom.msg, "q\n") || !strcmp(rwcustom.msg, "Q\n"))
			return -1;

		send(rwcustom.socket, rwcustom.msg, strlen(rwcustom.msg), 0);
	}

	return 0;
}