#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 创建TCP套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定套接字到地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // int recv_buf_size = 5000;
    // if(setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) < 0) {
    //     perror("SET SO_RCVBUF");
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 暂停
    // printf("listen... ");
    // getchar();

    while (1)
    {
    
    printf("Waiting for connections on port %d...\n", PORT);

    // 接受客户端连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char ipbuf[40];
    printf("Connection established with client %s:%d\n", 
        inet_ntop(AF_INET, &address.sin_addr.s_addr, ipbuf, 40),
        ntohs(address.sin_port));

    char buf[1000];
    while (1)
    {
        // 暂停
        printf("read? ");
        getchar();

        int size = read(new_socket, buf, 1000);
        if(size <= 0)
            break;
        printf("read %d bytes.\n", size);   
    }

    close(new_socket);
    printf("Connection closed.\n");
    }

    close(server_fd);
    return 0;
}
