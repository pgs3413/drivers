#include "net.h"

int ping(char *packet, struct sockaddr_in *dest)
{
    input_ip("dest ip: ", (unsigned char *)&(dest->sin_addr.s_addr));

    struct icmphdr *icmp = (struct icmphdr *)packet;
    char *data = packet + sizeof(struct icmphdr);

    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->un.echo.id = htons((uint32_t)getpid());
    icmp->un.echo.sequence = htons(1);

    printf("icmp data: ");
    fflush(stdout);
    scanf("%100s", data);
    int datasize = strlen(data);

    icmp->checksum = calculate_checksum(packet, sizeof(struct icmphdr) + datasize);

    return sizeof(struct icmphdr) + datasize;
}

int user_ip(char *packet, struct sockaddr_in *dest)
{
    struct iphdr *ip = (struct iphdr *)packet;
    char *data = packet + sizeof(struct iphdr);

    input_ip("src ip: ", (unsigned char *)&ip->saddr);
    input_ip("dest ip: ", (unsigned char *)&ip->daddr);
    ip->protocol = input_ip_proto("ip payload protocol: ");

    printf("ip payload: ");
    fflush(stdout);
    scanf("%1000s", data);

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(strlen(data) + 20);
    ip->id = htons(0xabcd);
    ip->frag_off = htons(0x4000);
    ip->ttl = 64;
    ip->check = 0;
    ip->check = calculate_checksum(ip, 20);

    dest->sin_addr.s_addr = ip->daddr; 

    return sizeof(struct iphdr) + strlen(data);
}

int udp_ip (char *packet, struct sockaddr_in *dest)
{
    input_ip("dest ip: ", (unsigned char *)&(dest->sin_addr.s_addr));

    struct udphdr *udp = (struct udphdr *)packet;
    char *data = packet + sizeof(struct udphdr);

    udp->source = input_port("src port: ");
    udp->dest = input_port("dest port: ");
    udp->check = 0;

    printf("data: ");
    fflush(stdout);
    scanf("%1000s", data);
    int datasize = strlen(data);
    int len = sizeof(struct udphdr) + datasize;

    udp->len = htons(len);
    return len;
}

int main(int argc, char *argv[])
{

    printf("choose mode: 1.user 2.ICMP(ping) 3.UDP ");
    fflush(stdout);
    int mode;
    scanf("%d", &mode);
    getchar();
    if(mode < 1 || mode > 3)
    {
        printf("wrong mode!\n");
        return 1;
    }

    int sockfd, size;
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    char *packet = malloc(4096);
    memset(packet, 0, 4096);

    if(mode == 1) {
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        size = user_ip(packet, &dest);
    } else if(mode == 2) {
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        size = ping(packet, &dest);
    } else if(mode == 3) {
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
        size = udp_ip(packet, &dest);
    }

    if(sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    if(argc >= 2 && strcmp(argv[1], "b") == 0)
    {
        int enable = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
    }

    size = sendto(sockfd, packet, size, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if(size < 0)
    {
        perror("sendto");
        exit(1);
    }

    printf("sent successfully(%d bytes)!\n", size);
}