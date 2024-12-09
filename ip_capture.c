#include"net.h"

void icmp(struct icmphdr *icmp, int size)
{
    printf("  type: %d\n", icmp->type);
    printf("  code: %d\n", icmp->code);
    if(icmp->type == 0 || icmp->type == 8)
    {
        printf(" identifier: %d", ntohs(icmp->un.echo.id));
        printf(" seq: %d\n", ntohs(icmp->un.echo.sequence));
    }
    char *data = (char *)icmp + sizeof(struct icmphdr);
    size = size - sizeof(struct icmphdr);
    data[size] = '\0';
    printf("  data: %s\n", data);
}

void udp(struct udphdr *udp, int size)
{
    printf("  src port: %d\n", ntohs(udp->source));
    printf("  dest port: %d\n", ntohs(udp->dest));
    printf("  len: %d\n", ntohs(udp->len));

    char *data = (char *)udp + sizeof(struct udphdr);
    size = size - sizeof(struct udphdr);
    data[size] = '\0';
     printf("  data: %s\n", data);
}

int main(){

    uint32_t target;
    input_ip("src/dest ip: ", (unsigned char *)&target);

    printf("choose mode: 1.ICMP 2.UDP 3.TCP ");
    fflush(stdout);
    int mode;
    scanf("%d", &mode);
    getchar();
    if(mode < 1 || mode > 2)
    {
        printf("wrong mode!\n");
        return 1;
    }

    int sockfd;

    if(mode == 1) {
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    } else if(mode == 2) {
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    }

    if(sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    char *packet = malloc(4096);
    struct iphdr *ip = (struct iphdr *)packet;
    char* data = packet + sizeof(struct iphdr);

    while(1)
    {
        int size = recvfrom(sockfd, packet, 4096, 0, NULL, NULL);
        if(size < 0)
        {
            perror("recvfrom");
            return 1;
        }

        if(ip->saddr != target && ip->daddr != target)
            continue;

        printf("IP packet:\n");

        char ipbuf[INET_ADDRSTRLEN];
        convert_ip(ipbuf, ( unsigned char *)&ip->saddr);
        printf("  src ip: %s\n", ipbuf);
        convert_ip(ipbuf, ( unsigned char *)&ip->daddr);
        printf("  dest ip: %s\n", ipbuf);

        if(mode == 1) {
            icmp((struct icmphdr *)data, size - sizeof(struct iphdr));
        } else if(mode == 2) {
            udp((struct udphdr *)data, size - sizeof(struct iphdr));
        }

    }

    return 0;
}