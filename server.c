#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10    // 한 방에 허용되는 최대 클라이언트 수
#define MAX_ROOMS 5       // 최대 방 개수
#define MAX_ROOM_NAME 50  // 방 이름 최대 길이

// 클라이언트 정보를 저장하는 구조체
typedef struct {
    int sock;               // 클라이언트 소켓
    char room[MAX_ROOM_NAME]; // 클라이언트가 접속 중인 방 이름
} ClientInfo;

// 채팅방 정보를 저장하는 구조체
typedef struct {
    char name[MAX_ROOM_NAME];   // 방 이름
    ClientInfo* clients[MAX_CLIENTS]; // 방에 있는 클라이언트 리스트
    int client_count;           // 현재 방에 있는 클라이언트 수
} ChatRoom;

ChatRoom rooms[MAX_ROOMS]; // 전체 방 목록
int room_count = 0;        // 현재 방 개수
pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER; // 동기화를 위한 뮤텍스

// 함수 선언
void error_handling(char* message);
void* handle_client(void* arg);
void broadcast_message(char* message, ClientInfo* sender);
int create_room(char* room_name);
int join_room(ClientInfo* client, char* room_name);
void leave_room(ClientInfo* client);

int main(int argc, char* argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    pthread_t thread_id;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // 소켓 바인딩
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    // 클라이언트 연결 대기
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Chat server started. Waiting for connections...\n");

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        // 클라이언트 연결 수락
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");

        // 새 클라이언트 정보를 저장
        ClientInfo* client = (ClientInfo*)malloc(sizeof(ClientInfo));
        client->sock = clnt_sock;
        memset(client->room, 0, MAX_ROOM_NAME);

        // 클라이언트를 처리할 스레드 생성
        pthread_create(&thread_id, NULL, handle_client, (void*)client);
        pthread_detach(thread_id); // 스레드 리소스를 자동으로 해제
    }

    close(serv_sock);
    return 0;
}

// 클라이언트 요청 처리 함수
void* handle_client(void* arg) {
    ClientInfo* client = (ClientInfo*)arg;
    char message[BUF_SIZE];
    int str_len;

    // 클라이언트에 환영 메시지 전송
    char welcome[] = "Welcome to the chat server!\nCommands:\n/create <room> - Create a new room and join it\n/join <room> - Join a room\n/leave - Leave current room\n/list - List all rooms\n";
    write(client->sock, welcome, strlen(welcome));

    while ((str_len = read(client->sock, message, BUF_SIZE - 1)) != 0) {
        message[str_len] = '\0';

        if (message[0] == '/') { // 명령어 처리
            char cmd[20], param[MAX_ROOM_NAME];
            sscanf(message, "%s %s", cmd, param);

            if (strcmp(cmd, "/create") == 0) {
                if (create_room(param)) {
                    sprintf(message, "Room '%s' created successfully.\n", param);
                    write(client->sock, message, strlen(message));
                    join_room(client, param); // 방 생성 후 자동 접속
                } else {
                    sprintf(message, "Failed to create room '%s'.\n", param);
                    write(client->sock, message, strlen(message));
                }
            } else if (strcmp(cmd, "/join") == 0) {
                if (join_room(client, param)) {
                    sprintf(message, "Joined room '%s'.\n", param);
                    write(client->sock, message, strlen(message));
                } else {
                    sprintf(message, "Failed to join room '%s'.\n", param);
                    write(client->sock, message, strlen(message));
                }
            } else if (strcmp(cmd, "/leave") == 0) {
                leave_room(client);
                write(client->sock, "Left the room.\n", 15);
            } else if (strcmp(cmd, "/list") == 0) {
                pthread_mutex_lock(&room_mutex);
                sprintf(message, "Available rooms:\n");
                for (int i = 0; i < room_count; i++) {
                    strcat(message, "- ");
                    strcat(message, rooms[i].name);
                    strcat(message, "\n");
                }
                pthread_mutex_unlock(&room_mutex);
                write(client->sock, message, strlen(message));
            }
        } else if (strlen(client->room) > 0) {
            broadcast_message(message, client);
        } else {
            write(client->sock, "Please join a room first.\n", 25);
        }
    }

    leave_room(client);
    close(client->sock);
    free(client);
    return NULL;
}


// 방 내 모든 클라이언트에게 메시지 브로드캐스트
void broadcast_message(char* message, ClientInfo* sender) {
    pthread_mutex_lock(&room_mutex);
    for (int i = 0; i < room_count; i++) {
        if (strcmp(rooms[i].name, sender->room) == 0) {
            char full_message[BUF_SIZE + 50];
            sprintf(full_message, "User %d: %s", sender->sock, message);
            
            for (int j = 0; j < rooms[i].client_count; j++) {
                if (rooms[i].clients[j]->sock != sender->sock) {
                    write(rooms[i].clients[j]->sock, full_message, strlen(full_message));
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&room_mutex);
}

// 새로운 방 생성
int create_room(char* room_name) {
    pthread_mutex_lock(&room_mutex);
    if (room_count >= MAX_ROOMS) {
        pthread_mutex_unlock(&room_mutex);
        return 0;
    }
    strcpy(rooms[room_count].name, room_name);
    rooms[room_count].client_count = 0;
    room_count++;
    pthread_mutex_unlock(&room_mutex);
    return 1;
}

// 클라이언트를 방에 추가
int join_room(ClientInfo* client, char* room_name) {
    pthread_mutex_lock(&room_mutex);
    for (int i = 0; i < room_count; i++) {
        if (strcmp(rooms[i].name, room_name) == 0) {
            if (rooms[i].client_count >= MAX_CLIENTS) {
                pthread_mutex_unlock(&room_mutex);
                return 0;
            }
            if (strlen(client->room) > 0) {
                leave_room(client);
            }
            strcpy(client->room, room_name);
            rooms[i].clients[rooms[i].client_count++] = client;
            pthread_mutex_unlock(&room_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return 0;
}

// 클라이언트를 방에서 제거
void leave_room(ClientInfo* client) {
    if (strlen(client->room) == 0) return;
    pthread_mutex_lock(&room_mutex);
    for (int i = 0; i < room_count; i++) {
        if (strcmp(rooms[i].name, client->room) == 0) {
            for (int j = 0; j < rooms[i].client_count; j++) {
                if (rooms[i].clients[j]->sock == client->sock) {
                    for (int k = j; k < rooms[i].client_count - 1; k++) {
                        rooms[i].clients[k] = rooms[i].clients[k + 1];
                    }
                    rooms[i].client_count--;
                    break;
                }
            }
            break;
        }
    }
    memset(client->room, 0, MAX_ROOM_NAME);
    pthread_mutex_unlock(&room_mutex);
}

// 에러 처리 함수
void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
