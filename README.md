서버

```
gcc -o server server.c -pthread

./server <port>
```

클라이언트

```
gcc -o client client.c -pthread

./client <IP> <port>
```
