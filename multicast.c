#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#ifndef __USE_MISC
struct ip_mreq
  {
    /* IP multicast address of group.  */
    struct in_addr imr_multiaddr;

    /* Local IP address of interface.  */
    struct in_addr imr_interface;
};
#endif

int main(int argc, char *argv[])
{

    if(argc < 4){
        printf("wrong argv!\n");
        return 1;
    }

    const char *iaddr = argv[1];
    const char *maddr = argv[2];
    unsigned short port = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if(ret < 0){
        perror("bind");
        return 1;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(maddr);
    mreq.imr_interface.s_addr = inet_addr(iaddr);
    ret = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if(ret < 0){
        perror("IP_ADD_MEMBERSHIP");
        return 1;
    }

    printf("Listening for messages on multicast address %s from %s:%d...\n", 
        maddr, iaddr, port);

    char buf[4096];
    struct sockaddr_in dest;
    int dest_len = sizeof(struct sockaddr_in);
    while(1){
        int size = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&dest, &dest_len);
        if(size < 0){
            perror("recvfrom");
            return 1;
        }

        buf[size] = '\0';
        printf("Received from %s %d : %s\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), buf);

    }
    return 0;
}