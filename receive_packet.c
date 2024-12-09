#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

int main(int argc, char *argv[])
{

    int sfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if(sfd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        return 1;
    }

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(3070);
    addr.sin6_addr = in6addr_any;

    int ret = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        return 1;
    }

    char buf[1024];
    struct sockaddr *dest = malloc(100);
    memset(dest, 0, 100);
    socklen_t dest_len;
    char ip_str[INET6_ADDRSTRLEN];
    void *ip;

    while (1)
    {
        dest_len = 100;
        int len = recvfrom(sfd, buf, 1023, 0, dest, &dest_len);
        if(len == -1)
        {
            perror("recvfrom");
            return 1;
        }

        buf[len] = '\0';

        if(dest->sa_family == AF_INET) 
        {
            ip = &((struct sockaddr_in *)dest)->sin_addr;
        } else {
            ip = &((struct sockaddr_in6 *)dest)->sin6_addr;
        }

        inet_ntop(dest->sa_family, ip, ip_str, INET6_ADDRSTRLEN);

        printf("receive msg from %s : %s\n", ip_str, buf);
    }
    
    return 0;
}