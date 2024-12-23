gcc -o server server.c -pthread
./server <port>

gcc -o client client.c -pthread
./client <IP> <port>
