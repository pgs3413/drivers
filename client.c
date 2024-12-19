#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#define LOCAL_PORT 9999
#define LOCAL_ADDR "192.168.1.247"
// #define LOCAL_ADDR "127.0.0.1"
#define SERVER_PORT 8080
#define SERVER_ADDR "192.168.1.214"
// #define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in local_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建TCP套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // int opt = 1;
    // if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    //     perror("SO_REUSEADDR");
    //     return 1;
    // }

    // struct linger linger;
    // linger.l_onoff = 1;
    // linger.l_linger = 0;
    // if(setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger))) {
    //     perror("SO_LINGFER");
    //     return 1;
    // }

    // local_addr.sin_family = AF_INET;
    // local_addr.sin_port = htons(LOCAL_PORT);
    // local_addr.sin_addr.s_addr = inet_addr(LOCAL_ADDR);

    // if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) < 0) {
    //     perror("bind");
    //     return 1;
    // }

    //开启TCP保活机制
    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(int));
    //设置空闲时间 10s
    int idle = 10;
    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int));
    //设置探测间隔 1s
    int intvl = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
    //设置探测次数 9
    int cnt = 9;
    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(int));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    // 连接服务器
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to server.\n");

    // char buf[100];
    // scanf("%s", buf);

    // write(sockfd, buf, strlen(buf));

    //暂停
    printf("pause... ");
    getchar();

    printf("start to close...\n");
    int ret = close(sockfd);
    if(ret < 0) {
        perror("close");
        return 1;
    }
    printf("closed.\n");
    return 0;
}
