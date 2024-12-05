#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char *argv[])
{

    if(argc < 4)
    {
        printf("wrong argv.\n");
        return 1;
    }

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sfd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    int ret = inet_pton(AF_INET, argv[1], &addr.sin_addr);
    if(ret != 1)
    {
        printf("wrong ip.\n");
        return 1;
    }

    ret = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("connect");
        return 1;
    }

    int msgLen = strlen(argv[3]);
    int len = write(sfd, argv[3], msgLen);
    if(len != msgLen)
    {
        perror("sendto");
        return 1;
    }

    printf("send successful.\n");
    return 0;
}