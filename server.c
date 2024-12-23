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
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char message[BUF_SIZE];
    pthread_t recv_thread;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 1) == -1)
        error_handling("listen() error");

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
        error_handling("accept() error");

    printf("Client connected.\n");

    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, handle_receive, (void *)&clnt_sock);

    // 송신 루프
    while (1) {
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "exit\n")) {
            printf("Closing connection...\n");
            write(clnt_sock, "exit\n", 5); // 클라이언트에 종료 메시지 전송
            close(clnt_sock);
            close(serv_sock);
            exit(0);
        }
        write(clnt_sock, message, strlen(message));
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
            printf("Client requested to close the connection.\n");
            close(sock);
            pthread_exit(NULL);
        }
        printf("Client: %s", message);
    }

    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
