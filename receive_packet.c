#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>

int main(int argc, char *argv[])
{

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
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        return 1;
    }

    char buf[1024];

    int len = recvfrom(sfd, buf, 1023, 0, NULL, NULL);
    if(len == -1)
    {
        perror("recvfrom");
        return 1;
    }

    buf[len] = '\0';
    printf("receive msg: %s\n", buf);
    return 0;
}