#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024

void error_handling(char *message);
void *handle_receive(void *arg);

int in_chat_room = 0;           // 현재 채팅방에 접속 여부를 나타내는 플래그 (0: 방 없음, 1: 방 있음)
char current_room[BUF_SIZE] = ""; // 현재 접속 중인 채팅방 이름

int main(int argc, char *argv[]) {
    int sock;                     // 서버와 연결할 소켓
    struct sockaddr_in serv_addr; // 서버 주소 정보 구조체
    char message[BUF_SIZE];       // 메시지 버퍼
    pthread_t recv_thread;        // 수신 전용 스레드

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

    pthread_create(&recv_thread, NULL, handle_receive, (void *)&sock);

    while (1) {
        if (in_chat_room) {
            printf("[%s] Chat> ", current_room);
        } else {
            printf("Enter command (or type 'exit' to quit): ");
        }

        fgets(message, BUF_SIZE, stdin);
        message[strlen(message) - 1] = '\0'; // 개행 제거

        if (strcmp(message, "exit") == 0) {
            printf("Exiting...\n");
            write(sock, "exit\n", 5);
            close(sock);
            exit(0);
        }

        if (message[0] == '/') { // 명령어 처리
            if (strncmp(message, "/join ", 6) == 0) {
                in_chat_room = 1;
                strncpy(current_room, message + 6, BUF_SIZE);
            } else if (strcmp(message, "/leave") == 0) {
                in_chat_room = 0;
                memset(current_room, 0, BUF_SIZE);
            } else if (strncmp(message, "/create ", 8) == 0) {
                strncpy(current_room, message + 8, BUF_SIZE); // 방 이름 저장
                in_chat_room = 1;                             // 방 상태 플래그 설정
            }
        }

        write(sock, message, strlen(message));
    }

    return 0;
}

void *handle_receive(void *arg) {
    int sock = *((int *)arg);
    char message[BUF_SIZE];
    int str_len;

    while ((str_len = read(sock, message, BUF_SIZE - 1)) > 0) {
        message[str_len] = '\0';
        printf("\n%s\n", message);

        if (strstr(message, "Left the room") != NULL) {
            in_chat_room = 0;
            memset(current_room, 0, BUF_SIZE);
        }

        if (in_chat_room) {
            printf("[%s] Chat> ", current_room);
        } else {
            printf("Enter command (or type 'exit' to quit): ");
        }
        fflush(stdout);
    }

    return NULL;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
