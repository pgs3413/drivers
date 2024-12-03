#include "net.h"

int main()
{

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(sockfd < 0)
    {
        perror("socket raw");
        return 1;
    }

    char *packet = malloc(4096);
    memset(packet, 0, 4096);
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

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ip->daddr;

    int size = sendto(sockfd, packet, sizeof(struct iphdr) + strlen(data), 0, 
        (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if(size < 0)
    {
        perror("sendto");
        return 0;
    }

    printf("ip packet sent successfully(%d bytes)!\n", size);
}