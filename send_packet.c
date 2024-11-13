#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>

int main(int argc, char *argv[])
{

    if(argc < 2)
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
    addr.sin_port = htons(3070);
    inet_pton(AF_INET, "172.25.0.2", &addr.sin_addr);

    int msgLen = strlen(argv[1]);
    int len = sendto(sfd, argv[1], msgLen, 0, (struct sockaddr *)&addr, sizeof(addr));
    if(len != msgLen)
    {
        perror("sendto");
        return 1;
    }

    printf("send successful.\n");
    return 0;
}