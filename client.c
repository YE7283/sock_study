#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024

void error_handling(char *message);
void *handle_receive(void *arg);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUF_SIZE];
    pthread_t recv_thread;

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("Connected to server.\n");

    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, handle_receive, (void *)&sock);

    // 송신 루프
    while (1) {
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "exit\n")) {
            printf("Closing connection...\n");
            write(sock, "exit\n", 5); // 서버에 종료 메시지 전송
            close(sock);
            exit(0);
        }
        write(sock, message, strlen(message));
    }

    return 0;
}

void *handle_receive(void *arg)
{
    int sock = *((int *)arg);
    char message[BUF_SIZE];
    int str_len;

    while ((str_len = read(sock, message, BUF_SIZE - 1)) != 0) {
        message[str_len] = '\0';
        if (!strcmp(message, "exit\n")) {
            printf("Server requested to close the connection.\n");
            close(sock);
            pthread_exit(NULL);
        }
        printf("Server: %s", message);
    }

    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
