#include "net.h"

int ping()
{
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd < 0)
    {
        perror("socket raw");
        exit(1);
    }

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    input_ip("dest ip: ", (unsigned char *)&dest.sin_addr.s_addr);

    char *packet = malloc(4096);
    memset(packet, 0, 4096);
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

    int size = sendto(sockfd, packet, sizeof(struct icmphdr) + datasize, 0, 
        (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if(size < 0)
    {
        perror("sendto");
        exit(1);
    }

    return size;
}

int user_ip()
{

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(sockfd < 0)
    {
        perror("socket raw");
        exit(1);
    }

    char *packet = malloc(4096);
    memset(packet, 0, 4096);
    struct iphdr *ip = (struct iphdr *)packet;
    char *data = packet + sizeof(struct iphdr);
    struct sockaddr_in dest;

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

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ip->daddr; 

    int size = sendto(sockfd, packet, sizeof(struct iphdr) + strlen(data), 0, 
        (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if(size < 0)
    {
        perror("sendto");
        exit(1);
    }

    return size;
}

int main()
{

    printf("choose mode: 1.user 2.ICMP(ping) ");
    fflush(stdout);
    int mode;
    scanf("%d", &mode);
    getchar();
    if(mode < 1 || mode > 2)
    {
        printf("wrong mode!\n");
        return 1;
    }

    int size;
    if(mode == 1){
        size = user_ip();
    } else if(mode == 2) {
        size = ping();
    }

    printf("sent successfully(%d bytes)!\n", size);
}